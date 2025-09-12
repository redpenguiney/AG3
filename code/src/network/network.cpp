#include "network.hpp"
#include "debug/assert.hpp"
#include <utility/utility.hpp>
#include "packet_types.hpp"

const std::vector<std::shared_ptr<Client>>& NetworkingEngine::GetClientList()
{
    return clients;
}

NetworkingEngine& NetworkingEngine::Get()
{
    InitSocketGlobals();
    static NetworkingEngine ne;
    return ne;
}

NetworkStatus NetworkingEngine::GetStatus()
{
    return status;
}

void NetworkingEngine::Host(int port) {
    Assert(status == NetworkStatus::Offline);
    status = NetworkStatus::Server;
    
    // open socket and let anyone use it
    serverSocket.emplace(port);

    clients.push_back(Client::New(true, true, port, "127.0.0.1"));
}

void NetworkingEngine::Unhost(){
    Assert(status == NetworkStatus::Server);

    for (int i = clients.size() - 1; i >= 0; i--) {
        Kick(clients[i], "Default server shutdown message.");
    }

    serverSocket = std::nullopt;

    status = NetworkStatus::Offline;
}

void NetworkingEngine::Kick(std::shared_ptr<Client>& client, std::string reason) {
    for (auto it = clients.begin(); it != clients.end(); it++) {
        if (*it == client) {
            clients.erase(it);
            return;
        }
    }
}

void NetworkingEngine::Disconnect() {
    Assert(status == NetworkStatus::Client || status == NetworkStatus::ClientConnecting);

    serverSocket = std::nullopt;
    clients = {};

    status = NetworkStatus::Offline;
}

void NetworkingEngine::Connect(std::string ipAddress, int destPort, int localPort, float timeout, unsigned numTries) {
    Assert(timeout > 0 && numTries > 0);
    Assert(status == NetworkStatus::Offline);
    status = NetworkStatus::ClientConnecting;

    clients.push_back(Client::New(false, true, localPort, "127.0.0.1"));

    connectionAttemptTimeout = timeout;
    timeUntilConnectionAttemptTimeout = connectionAttemptTimeout;
    connectionAttemptsRemaining = numTries;
    targetConnectionIp = ipAddress;

    serverSocket.emplace(ipAddress, destPort, localPort);

    PacketStructs::ConnectionRequest request;
    serverSocket->Send(&request, sizeof(request));
    connectionAttemptsRemaining--;
}

void NetworkingEngine::Update(float dt){

    if (status == NetworkStatus::Offline)
        return;

    auto time = Time();

    // Handle connection requests
    if (status == NetworkStatus::Server) {
        auto packets = serverSocket->Recieve();

        for (auto& p : packets) {
            Assert(p.data.size() > 0);
            auto packetType = (uint8_t*)p.data.data();
            bool isUserdata = (*packetType & 0b10000000) != 0;
            *packetType &= 0b01111111;
            if (*packetType == PacketType::ConnectionRequest) {

                // Make sure client isn't already connected; 
                // in this case they didn't hear that they were accepted so we just want to resend the message
                if (GetClient(p.originAddress, p.originPort)) {
                    size_t size = sizeof(PacketStructs::ConnectionRequestResponse);
                    auto response = static_cast<PacketStructs::ConnectionRequestResponse*>(malloc(size));

                    response->type = PacketType::ConnectionRequestResponse;
                    response->connectionAccepted = true;

                    serverSocket->Send(p.originAddress, p.originPort, response, size);
                    free(response);
                }
                else {
                    auto [accepted, reason] = connectionRequestHandler(p.originAddress, p.originPort);

                    if (accepted) {
                        size_t size = sizeof(PacketStructs::ConnectionRequestResponse);
                        auto response = static_cast<PacketStructs::ConnectionRequestResponse*>(malloc(size));

                        response->type = PacketType::ConnectionRequestResponse;
                        response->connectionAccepted = true;

                        serverSocket->Send(p.originAddress, p.originPort, response, size);


                        DebugLogInfo("Accepted new client (awaiting handshake).");

                        auto newClient = Client::New(false, false, p.originPort, p.originAddress);
                        newClient->connection = ConnectionInfo();
                        newClient->connection->lastMessageTime = p.timestamp;
                        newClient->connection->completedHandshake = false;
                        //newClient->connection->pendingAcks.push_back(PendingAck(newClient->connection->GetAvailableAckId(), time, response, size));
                        clients.push_back(newClient);
                        free(response);
                    }
                    else {
                        std::string rejectionReason = reason.value_or("");

                        size_t size = sizeof(PacketStructs::ConnectionRequestResponse) + rejectionReason.length();
                        auto response = static_cast<PacketStructs::ConnectionRequestResponse*>(malloc(size));

                        response->type = PacketType::ConnectionRequestResponse;
                        response->connectionAccepted = false;
                        memcpy(response + 1, rejectionReason.c_str(), rejectionReason.length());

                        serverSocket->Send(p.originAddress, p.originPort, response, size);
                        free(response);
                    }
                }
                
            }
            else if (*packetType == PacketType::CompleteConnectionHandshake) {

                auto client = GetClient(p.originAddress, p.originPort);

                if (!client) continue; 

                DebugLogInfo("Completed connection handshake.");

                client->connection->lastMessageTime = p.timestamp;

            }
            else if (*packetType == PacketType::AckArray) {
                auto client = GetClient(p.originAddress, p.originPort);
                ProcessAckArray(client, std::bit_cast<AckId*>(packetType + 1), (p.data.size() - 1) / 4);
            }
            else if (*packetType == PacketType::SendShort) {
                auto client = GetClient(p.originAddress, p.originPort);
                ProcessShortMessage(client, packetType + 1, p.data.size() - 1, isUserdata);
            }
            else if (*packetType == PacketType::SendShortAck) {
                auto client = GetClient(p.originAddress, p.originPort);
                auto msg = (PacketStructs::ShortMessageReliable*)p.data.data();
                ProcessShortMessageReliable(client, msg->ackId, msg->data, p.data.size() - sizeof(PacketStructs::ShortMessageReliable), isUserdata);
            }
            else if (*packetType == PacketType::LongMessage) {
                auto client = GetClient(p.originAddress, p.originPort);
                auto msg = (PacketStructs::LongMessage*)p.data.data();
                ProcessLongMessageFragment(client, msg->firstPacketAckId, msg->offset, msg->numPackets, msg->data, p.data.size() - sizeof(PacketStructs::LongMessage), isUserdata);
            }
        }

        ResendUnackedMessages();
        AckMessages();
    }
    else if (status == NetworkStatus::ClientConnecting) {
        auto packets = serverSocket->Recieve();
        for (auto& p : packets) {
            Assert(p.data.size() > 0);

            if (p.originAddress != targetConnectionIp) continue;

            auto packetType = (uint8_t*)p.data.data();
            bool isUserdata = (*packetType & 0b10000000) != 0;
            *packetType &= 0b01111111;
            if (*packetType == PacketType::ConnectionRequestResponse) {
                if (p.data.size() < 2) continue; 
                auto connectionAccepted = *(bool*)(p.data.data() + 1);

                if (!connectionAccepted) {
                    std::string reason = "";
                    for (int i = 2; i < p.data.size(); i++) {
                        char c = (char)(p.data[i]);
                        if (c == '\0') break;
                        reason += c;
                    }

                    ConnectionAttemptResult result{
                        .successful = false,
                        .failureReason = ConnectionFailureReason::ServerRejected,
                        .failureMessage = reason
                    };
                    onConnectionAttemptComplete->Fire(result);

                    status = NetworkStatus::Offline;
                        
                    timeUntilConnectionAttemptTimeout = -1.0f;
                    connectionAttemptTimeout = -1.0f;
                    connectionAttemptsRemaining = -1;
                        
                }
                else {

                    // complete handshake
                    //timeUntilConnectionAttemptTimeout = connectionAttemptTimeout;
                    timeUntilConnectionAttemptTimeout = -1.0f;
                    connectionAttemptTimeout = -1.0f;
                    connectionAttemptsRemaining = -1;

                    PacketStructs::CompleteConnectionHandshake handshake;
                    serverSocket->Send(&handshake, sizeof(handshake));

                    DebugLogInfo("Client connection to server successful.");

                    ConnectionAttemptResult result{
                        .successful = true,
                        .failureReason = std::nullopt,
                        .failureMessage = std::nullopt
                    };
                    status = NetworkStatus::Client;
                    onConnectionAttemptComplete->Fire(result);

                    clients.push_back(Client::New(true, false, p.originPort, p.originAddress));
                    clients.back()->connection = ConnectionInfo();
                    clients.back()->connection->lastMessageTime = p.timestamp;
                    clients.back()->connection->completedHandshake = true;
                }
            }
        }

        if (timeUntilConnectionAttemptTimeout != -1) {
            timeUntilConnectionAttemptTimeout -= dt;
            if (timeUntilConnectionAttemptTimeout < 0) {
                DebugLogInfo("Connection attempt timed out.");
                connectionAttemptsRemaining--;

                if (connectionAttemptsRemaining < 0) {
                    ConnectionAttemptResult result{
                        .successful = false,
                        .failureReason = ConnectionFailureReason::TimedOut,
                        .failureMessage = std::nullopt
                    };
                    onConnectionAttemptComplete->Fire(result);

                    status = NetworkStatus::Offline;

                    timeUntilConnectionAttemptTimeout = -1.0f;
                    connectionAttemptTimeout = -1.0f;
                    connectionAttemptsRemaining = -1;
                }
                else {
                    timeUntilConnectionAttemptTimeout = connectionAttemptTimeout;

                    PacketStructs::ConnectionRequest request;
                    serverSocket->Send(&request, sizeof(request));
                }
            }
        }
    }
    else if (status == NetworkStatus::Client) {
        auto packets = serverSocket->Recieve();
        for (auto& p : packets) {
            Assert(p.data.size() > 0);

            if (p.originAddress != targetConnectionIp) continue;

            auto packetType = (uint8_t*)p.data.data();
            bool isUserdata = (*packetType & 0b10000000) != 0;
            *packetType &= 0b01111111;
            if (*packetType == PacketType::ConnectionRequestResponse) {
                // If this happens, then the server already said we can join but then didn't recieve our last CompleteConnectionHandshake packet, so we'll send it again.
                PacketStructs::CompleteConnectionHandshake handshake;
                serverSocket->Send(&handshake, sizeof(handshake));
            }
            else if (*packetType == PacketType::AckArray) {
                auto client = GetClient(p.originAddress, p.originPort);
                ProcessAckArray(client, std::bit_cast<AckId*>(packetType + 1), (p.data.size() - 1) / 4);
            }
            else if (*packetType == PacketType::SendShort) {
                auto client = GetClient(p.originAddress, p.originPort);
                ProcessShortMessage(client, packetType + 1, p.data.size() - 1, isUserdata);
            }
            else if (*packetType == PacketType::SendShortAck) {
                auto client = GetClient(p.originAddress, p.originPort);
                ProcessShortMessageReliable(client, *std::bit_cast<AckId*>(packetType + 1), p.data.data() + 5, p.data.size() - 5, isUserdata);
            }
            else if (*packetType == PacketType::LongMessage) {
                auto client = GetClient(p.originAddress, p.originPort);
                auto msg = (PacketStructs::LongMessage*)p.data.data();
                ProcessLongMessageFragment(client, msg->firstPacketAckId, msg->offset, msg->numPackets, msg->data, p.data.size() - sizeof(PacketStructs::LongMessage), isUserdata);
            }
        }

        ResendUnackedMessages();
        AckMessages();
    }
}

std::shared_ptr<Client> NetworkingEngine::GetClient(std::string address, int port) {
    for (auto& c : clients) {
        if (c->address == address && c->port == port) {
            return c;
        }
    }
    return nullptr;
}

void NetworkingEngine::SendDataReliable(void* data, size_t nBytes, std::shared_ptr<Client>& destination) {
    ImplSendDataReliable(data, nBytes, destination, true);
}

void NetworkingEngine::SendDataReliable(void* data, size_t nBytes) {
    Assert(status == NetworkStatus::Client);
    for (auto& c : clients) {
        if (c->isServer) {
            SendDataReliable(data, nBytes, c);
            return;
        }
    }
}

void NetworkingEngine::SendData(void* data, size_t nBytes, std::shared_ptr<Client>& destination){
    ImplSendData(data, nBytes, destination, true);
}

void NetworkingEngine::SendData(void* data, size_t nBytes) {
    Assert(status == NetworkStatus::Client);
    for (auto& c : clients) {
        if (c->isServer) {
            SendData(data, nBytes, c);
            return;
        }
    }
}

void NetworkingEngine::ImplSendDataReliable(void* data, size_t nBytes, std::shared_ptr<Client>& destination, bool isUserdata)
{
    Assert(!destination->isLocalMachine);
    if (status == NetworkStatus::Client) {
        Assert(destination->isServer);
    }
    else Assert(status == NetworkStatus::Server);
    Assert(destination->connection);
    if (nBytes <= 500) {
        PacketStructs::ShortMessageReliable* packet = (PacketStructs::ShortMessageReliable*)malloc(nBytes + sizeof(PacketStructs::ShortMessageReliable));
        packet->type = PacketType::SendShortAck;
        if (isUserdata)
            packet->type |= 0b10000000;
        packet->ackId = destination->connection->GetAvailableAckId();

        void* payload = malloc(nBytes);
        memcpy(payload, data, nBytes);

        destination->connection->pendingAcks.emplace_back(packet->ackId, Time(), payload, nBytes);
        memcpy((uint8_t*)packet + sizeof(PacketStructs::ShortMessageReliable), data, nBytes);
        serverSocket->Send(packet, nBytes + sizeof(PacketStructs::ShortMessageReliable));

        free(packet);
        // don't free payload
    }
    else {
        uint16_t i = 0;
        AckId firstAck = UINT32_MAX;
        uint16_t numPackets = (nBytes + 499) / 500;
        auto t = Time();
        while (nBytes > 0) {
            unsigned amtToSend = nBytes > 500 ? 500 : nBytes;
            nBytes -= amtToSend;
            
            PacketStructs::LongMessage* packet = (PacketStructs::LongMessage*)malloc(amtToSend + sizeof(PacketStructs::LongMessage));
            
            void* payload = malloc(amtToSend);
            memcpy(payload, data, amtToSend);

            auto ackId = destination->connection->GetAvailableAckId();
            if (firstAck == UINT32_MAX) firstAck = ackId;
            destination->connection->pendingAcks.emplace_back(ackId, t, payload, amtToSend);
            //DebugLogInfo("PendingAck ", ackId);

            packet->type = PacketType::LongMessage;
            if (isUserdata)
                packet->type |= 0b10000000;
            packet->offset = i++;
            packet->firstPacketAckId = firstAck;
            packet->numPackets = numPackets;
           
            memcpy((uint8_t*)packet + sizeof(PacketStructs::LongMessage), data, amtToSend);
            data = (uint8_t*)data + amtToSend;
            serverSocket->Send(packet, amtToSend + sizeof(PacketStructs::LongMessage));

            free(packet);
        }

        Assert(i == numPackets);
        //DebugLogInfo("LONGDATA SENT IN ", i, " PACKET");
    }
}

void NetworkingEngine::ImplSendData(void* data, size_t nBytes, std::shared_ptr<Client>& destination, bool isUserdata) {
    Assert(nBytes <= 500);
    Assert(!destination->isLocalMachine);
    if (status == NetworkStatus::Client) {
        Assert(destination->isServer);
    }
    else Assert(status == NetworkStatus::Server);

    PacketStructs::ShortMessage* packet = (PacketStructs::ShortMessage*)malloc(nBytes + sizeof(PacketStructs::ShortMessage));
    packet->type = PacketType::SendShort;
    if (isUserdata)
        packet->type |= 0b10000000;
    memcpy(packet + sizeof(packet->type), data, nBytes);
    serverSocket->Send(packet, nBytes + sizeof(PacketStructs::ShortMessage));

    free(packet);
}

void NetworkingEngine::HandleAck(Socket::Packet& p) {
    auto client = GetClient(p.originAddress, p.originPort);
    if (!client || !client->connection) return;
    client->connection->lastMessageTime = p.timestamp;

    Assert(p.data.size() > sizeof(PacketStructs::AckArray));
    auto packetType = (uint8_t*)p.data.data();
    *packetType &= 0b01111111;
    Assert(*packetType = PacketType::AckArray);
    unsigned nPackets = p.data.size() - sizeof(PacketStructs::AckArray);
  
    AckId* ack = reinterpret_cast<AckId*>(p.data.data() + sizeof(PacketStructs::AckArray));
    for (unsigned i = 0; i < nPackets; i++) { // TODO: BAD TIME COMPLEXITY, SHOULD OPTIMIZE
        for (auto it = client->connection->pendingAcks.begin(); it != client->connection->pendingAcks.end(); it++) {
            if (it->ackId == *ack) {
                client->connection->pendingAcks.erase(it);
                break;
            }
        }
        ack++;
    }
}

void NetworkingEngine::ResendUnackedMessages() {
    auto t = Time();
    for (unsigned i = 0; i < clients.size(); i++) {
        auto& c = clients.at(i);
        if (!c->connection) continue;

        for (auto& ack : c->connection->pendingAcks) {
            if (ack.sentTimestamp + RESEND_PACKET_TIME < t) {
                
                ack.sentTimestamp = t;

                DebugLogInfo("Resending unacked ", ack.payloadNBytes, " bytes.");

                // resend message
                if (status == NetworkStatus::Server)
                    serverSocket->Send(c->address, c->port, ack.payload, ack.payloadNBytes);
                else
                    serverSocket->Send(ack.payload, ack.payloadNBytes);
            }
        }

        if (c->connection->lastMessageTime + TIMEOUT_TIME < t) {
            // Drop connection
            if (c->isServer) {
                DebugLogInfo("Server non-responsive. Disconnecting.")
                Disconnect();
                return;
            }
            else {
                DebugLogInfo("Client non-responsive. Removing them.")
                // TODO: do more?
                clients[i] = clients.back();
                clients.pop_back();
                i--;
            }
        }

    }
}

void NetworkingEngine::AckMessages() {
    for (auto& c : clients) {
        if (c->connection) {
            auto packets = c->connection->FlushAcksToSend();
            if (!packets.empty()) DebugLogInfo("There are ", packets.size(), " inbound packets to acknowledge.");
            for (auto& p : packets) {
                if (status == NetworkStatus::Client)
                    serverSocket->Send(p.first, p.second);
                else
                    serverSocket->Send(c->address, c->port, p.first, p.second);
                free(p.first);
            }

            c->connection->ExpireRecievedPackets();
        }
    }
}

void NetworkingEngine::ProcessAckArray(std::shared_ptr<Client>& client, AckId* acks, unsigned nAcks) {
    Assert(client && client->connection);
    DebugLogInfo("Handling ackarray with ", nAcks, " acks");
    //DebugLogInfo("We're currently waiting for: ");
    for (auto it = client->connection->pendingAcks.begin(); it != client->connection->pendingAcks.end(); it++) DebugLogInfo(it->ackId);
    
    for (unsigned i = 0; i < nAcks; i++) {
        for (unsigned int j = 0; j < client->connection->pendingAcks.size(); j++) {
            //DebugLogInfo("Considering ", client->connection->pendingAcks[j].ackId);
            if (client->connection->pendingAcks[j].ackId == acks[i]) {
                //DebugLogInfo("Found ackId ", acks[i], " will be replaced with ", client->connection->pendingAcks.back().ackId);
                client->connection->pendingAcks[j] = std::move(client->connection->pendingAcks.back());
                client->connection->pendingAcks.pop_back();
                Assert(client->connection->pendingAcks[j].ackId != acks[i]);
                goto found;
            }
        }
        /*for (auto it = client->connection->pendingAcks.begin(); it != client->connection->pendingAcks.end(); it++) {
            if (it->ackId == acks[i]) {
                DebugLogInfo("Recieved ack for ", it->ackId);
                client->connection->pendingAcks.erase(it);

                goto found;
            }
        }*/
        DebugLogInfo("Unrecognized ackId from inbound ackarray: ", acks[i]);

        found:;

        for (unsigned j = 0; j < client->connection->pendingAcks.size(); j++) {
            Assert(client->connection->pendingAcks[j].ackId != acks[i]);
        }
    }
}

void NetworkingEngine::ProcessShortMessage(std::shared_ptr<Client>& client, uint8_t* data, unsigned nBytes, bool isUserdata) {
    if (isUserdata) {
        NetworkUserdata userdata {};
        userdata.data.assign(data, data + nBytes);
        userdata.reliable = false;
        onUserdataRecieved->Fire(userdata);
    }
    else {

    }
}

void NetworkingEngine::ProcessShortMessageReliable(std::shared_ptr<Client>& client, AckId ackId, uint8_t* data, unsigned nBytes, bool isUserdata) {
    Assert(client && client->connection);

    if (client->connection->AlreadyRecievedPacket(ackId)) return;

    client->connection->AckData(ackId);

    if (isUserdata) {

        NetworkUserdata userdata{};
        userdata.data.assign(data, data + nBytes);
        userdata.reliable = true;
        onUserdataRecieved->Fire(userdata);

    }
    else {

    }
}

void NetworkingEngine::ProcessLongMessageFragment(std::shared_ptr<Client>& client, AckId firstAckId, uint16_t idOffset, uint16_t nPackets, uint8_t* data, unsigned nBytes, bool isUserdata) {
    Assert(client && client->connection);
    AckId ackId = firstAckId + idOffset;

    //DebugLogInfo("Got long message fragment ", idOffset + 1, "/", nPackets);

    if (client->connection->AlreadyRecievedPacket(ackId)) return;

    client->connection->AckData(ackId);
    
    if (!client->connection->wipLongMessages.contains(firstAckId)) {
        DebugLogInfo("Recieving new long message with ", nPackets, " fragments.");
        client->connection->wipLongMessages[firstAckId].firstPacketId = firstAckId;
        client->connection->wipLongMessages[firstAckId].numPackets = nPackets;   
    }
    std::vector<uint8_t> niceData;
    niceData.assign(data, data + nBytes);
    client->connection->wipLongMessages[firstAckId].packets.push_back(niceData);
    if (client->connection->wipLongMessages[firstAckId].packets.size() == nPackets) {
        if (isUserdata) {
            NetworkUserdata userdata;
            userdata.reliable = true;
            for (auto& pkt : client->connection->wipLongMessages[firstAckId].packets)
                userdata.data.insert(userdata.data.end(), pkt.begin(), pkt.end());
            client->connection->wipLongMessages.erase(firstAckId);
            DebugLogInfo("Long message with ", nPackets, " fragments completed.");
            onUserdataRecieved->Fire(userdata);
        }
        else {

        }
    }
}

NetworkingEngine::NetworkingEngine():
    onConnectionAttemptComplete(Event<ConnectionAttemptResult>::New()),
    onInitialSyncComplete(Event<>::New()),
    onUserdataRecieved(Event<NetworkUserdata>::New())
{
    status = NetworkStatus::Offline;
    serverSocket = std::nullopt;
}

std::pair<bool, std::optional<std::string>> DefaultConnectionRequestHandler(std::string ipAddress, int port) {
    DebugLogInfo("Default connection request handler recieved request from ", ipAddress, ":", port);
    return { true, std::nullopt };
}