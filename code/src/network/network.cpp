#include "network.hpp"
#include "debug/assert.hpp"
#include <utility/utility.hpp>
#include "packet_types.hpp"
#include "gameobjects/gameobject.hpp"

// TODO: this file has memory leaks in it. I know it does. 
// TODO: we don't serialize anything except formatted data which is a big issue.

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

    currentTick++;

    // Handle connection requests
    if (status == NetworkStatus::Server) {
        SyncGameobjects();

        auto packets = serverSocket->Recieve();

        for (auto& p : packets) {
            Assert(p.data.size() > 0);
            auto packetType = (uint8_t*)p.data.data();
            bool isUserdata = (*packetType & 0b10000000) != 0;
            *packetType &= 0b01111111;

            if (auto client = GetClient(p.originAddress, p.originPort)) {
                client->connection->lastMessageTime = p.timestamp;
            }

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


                        //DebugLogInfo("Accepted new client (awaiting handshake).");

                        auto newClient = Client::New(false, false, p.originPort, p.originAddress);
                        newClient->connection = ConnectionInfo();
                        newClient->connection->lastMessageTime = p.timestamp;
                        newClient->connection->completedHandshake = false;
                        //newClient->connection->pendingAcks.push_back(PendingAck(newClient->connection->GetAvailableAckId(), time, response, size));
                        clients.push_back(newClient);

                        onNewClient->Fire(newClient);

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

                //DebugLogInfo("Completed connection handshake.");

                client->connection->completedHandshake = true;
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

                    //DebugLogInfo("Client connection to server successful.");

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
                //DebugLogInfo("Connection attempt timed out.");
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
        SyncGameobjects();

        auto packets = serverSocket->Recieve();
        for (auto& p : packets) {
            Assert(p.data.size() > 0);

            if (p.originAddress != targetConnectionIp) continue;

            GetServer()->connection->lastMessageTime = p.timestamp;

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

std::shared_ptr<Client> NetworkingEngine::GetLocalMachine()
{
    for (auto& c : clients) {
        if (c->isLocalMachine) return c;
    }
    Assert(false);
    return nullptr;
}

std::shared_ptr<Client> NetworkingEngine::GetServer() {
    //Assert(status == NetworkStatus::Client);
    //if (status == NetworkStatus::Server) return GetLocalMachine();
    for (auto& c : clients) {
        if (c->isServer) return c;
    }
    Assert(false);
    return nullptr;
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

TransformSync GetNetworkTransform(std::shared_ptr<GameObject>& obj) {
    auto transform = obj->RawGet<TransformComponent>();
    return TransformSync{
        .position = transform->Position(),
        .rotation = transform->Rotation()
    };
}

RigidbodySync GetNetworkRigidbody(std::shared_ptr<GameObject>& obj) {
    auto rigidbody = obj->RawGet<RigidbodyComponent>();
    return RigidbodySync {
        .velocity = rigidbody->velocity,
        .angularVelocity = rigidbody->angularVelocity
    };
}

void NetworkingEngine::SyncObjectTransform(std::shared_ptr<GameObject> obj, SyncId name, bool ackSnapshots) {
    //auto client = GetLocalMachine();
    auto owner = GetServer();

    Assert(name != 0); // reserved

    Assert(owner->ownedSyncedTransforms.contains(name) == false);
    owner->ownedSyncedTransforms[name] = TransformSyncInfo{
        .lastSentTransform = GetNetworkTransform(obj),
        .lastSentRigidbody = obj->MaybeRawGet<RigidbodyComponent>() ? std::optional(GetNetworkRigidbody(obj)) : std::nullopt,
        .priorityAccumulator = INFINITY,
        .ackTransformSnapshots = ackSnapshots,
        .gameObject = obj
    };
}

void NetworkingEngine::SetNetworkOwner(std::shared_ptr<GameObject> obj, std::shared_ptr<Client> client) {

    Assert(false); // TODO
}

void NetworkingEngine::ImplSendDataReliable(void* data, size_t nBytes, std::shared_ptr<Client>& destination, bool isUserdata, UserdataFormatName format)
{
    Assert(!destination->isLocalMachine);
    if (status == NetworkStatus::Client || status == NetworkStatus::ClientConnecting) {
        Assert(destination->isServer);
    }
    else Assert(status == NetworkStatus::Server);
    Assert(destination->connection);

    size_t trueNBytes = nBytes;
    if (isUserdata) trueNBytes += sizeof(UserdataFormatName);
    size_t dataOffset = 0;
    if (isUserdata) dataOffset = sizeof(UserdataFormatName);

    if (nBytes <= 500) {
        PacketStructs::ShortMessageReliable* packet = (PacketStructs::ShortMessageReliable*)malloc(trueNBytes + sizeof(PacketStructs::ShortMessageReliable));
        packet->type = PacketType::SendShortAck;
        if (isUserdata)
            packet->type |= 0b10000000;
        packet->ackId = destination->connection->GetAvailableAckId();

        void* payload = malloc(trueNBytes);
        memcpy((uint8_t*)payload + dataOffset, data, nBytes);
        if (isUserdata) memcpy(payload, &format, sizeof(UserdataFormatName));

        destination->connection->pendingAcks.emplace_back(packet->ackId, Time(), payload, trueNBytes);
        memcpy((uint8_t*)packet + sizeof(PacketStructs::ShortMessageReliable), payload, trueNBytes);
        if (status == NetworkStatus::Client || status == NetworkStatus::ClientConnecting) {
            serverSocket->Send(packet, trueNBytes + sizeof(PacketStructs::ShortMessageReliable));
        }
        else {
            serverSocket->Send(destination->address, destination->port, packet, trueNBytes + sizeof(PacketStructs::ShortMessageReliable));
        }
        free(packet);
        // don't free payload
    }
    else {
        uint16_t i = 0;
        AckId firstAck = UINT32_MAX;
        uint16_t numPackets = (nBytes + 499) / 500;
        auto t = Time();
        while (nBytes > 0) {
            unsigned amtToSend = (nBytes) > 500 ? 500 : nBytes;
            nBytes -= amtToSend;
            
            PacketStructs::LongMessage* packet = (PacketStructs::LongMessage*)malloc(amtToSend + sizeof(PacketStructs::LongMessage));
            
            void* payload = malloc(amtToSend);
            memcpy(payload, (uint8_t*)data + dataOffset, amtToSend - dataOffset);
            if (isUserdata) memcpy(payload, &format, sizeof(UserdataFormatName));

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
           
            memcpy((uint8_t*)packet + sizeof(PacketStructs::LongMessage), payload, amtToSend);
            data = (uint8_t*)data + amtToSend - dataOffset;
            if (status == NetworkStatus::Client || status == NetworkStatus::ClientConnecting) {
                serverSocket->Send(packet, amtToSend + sizeof(PacketStructs::LongMessage));
            }
            else {
                serverSocket->Send(destination->address, destination->port, packet, amtToSend + sizeof(PacketStructs::LongMessage));
            }
            free(packet);
        }

        Assert(i == numPackets);
        //DebugLogInfo("LONGDATA SENT IN ", i, " PACKET");
    }
}

void NetworkingEngine::ImplSendData(void* data, size_t nBytes, std::shared_ptr<Client>& destination, bool isUserdata, UserdataFormatName format) {
    Assert(nBytes <= 500);
    Assert(!destination->isLocalMachine);
    if (status == NetworkStatus::Client || status == NetworkStatus::ClientConnecting) {
        Assert(destination->isServer);
    }
    else Assert(status == NetworkStatus::Server);

    /*if (nBytes > 20 && *((uint8_t*)data + 20) == 0xcd) {
        DebugLogInfo("Sending uninitialized.");
    }*/

    size_t trueNBytes = nBytes;
    if (isUserdata) trueNBytes += sizeof(UserdataFormatName);
    size_t dataOffset = 0;
    if (isUserdata) dataOffset = sizeof(UserdataFormatName);

    PacketStructs::ShortMessage* packet = (PacketStructs::ShortMessage*)malloc(trueNBytes + sizeof(PacketStructs::ShortMessage));
    packet->type = PacketType::SendShort;
    if (isUserdata)
        packet->type |= 0b10000000;
    memcpy(packet + sizeof(PacketStructs::ShortMessage), &format, sizeof(UserdataFormatName));
    memcpy(packet + dataOffset + sizeof(PacketStructs::ShortMessage), data, nBytes);

    if (status == NetworkStatus::Client || status == NetworkStatus::ClientConnecting) {
        serverSocket->Send(packet, trueNBytes + sizeof(PacketStructs::ShortMessage));
    }
    else {
        serverSocket->Send(destination->address, destination->port, packet, trueNBytes + sizeof(PacketStructs::ShortMessage));
    }
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

                //DebugLogInfo("Resending unacked ", ack.payloadNBytes, " bytes.");

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
            //if (!packets.empty()) DebugLogInfo("There are ", packets.size(), " inbound packets to acknowledge.");
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
    //DebugLogInfo("Handling ackarray with ", nAcks, " acks");
    //DebugLogInfo("We're currently waiting for: ");
    // for (auto it = client->connection->pendingAcks.begin(); it != client->connection->pendingAcks.end(); it++) {
    //  DebugLogInfo(it->ackId);
    // }
    
    for (unsigned i = 0; i < nAcks; i++) {
        for (unsigned int j = 0; j < client->connection->pendingAcks.size(); j++) {
            //DebugLogInfo("Considering ", client->connection->pendingAcks[j].ackId);
            if (client->connection->pendingAcks[j].ackId == acks[i]) {
                //DebugLogInfo("Found ackId ", acks[i], " will be replaced with ", client->connection->pendingAcks.back().ackId);
                client->connection->pendingAcks[j] = std::move(client->connection->pendingAcks.back());
                client->connection->pendingAcks.pop_back();
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
        //DebugLogInfo("Unrecognized ackId from inbound ackarray: ", acks[i]);

        found:;

        //for (unsigned j = 0; j < client->connection->pendingAcks.size(); j++) {
            //Assert(client->connection->pendingAcks[j].ackId != acks[i]);
        //}
    }
}

void NetworkingEngine::ProcessShortMessage(std::shared_ptr<Client>& client, uint8_t* data, unsigned nBytes, bool isUserdata) {

    if (nBytes > 20 && *((uint8_t*)data + 80) == 0xcd) {
        DebugLogInfo("Recieving uninitialized.");
    }

    if (isUserdata) {
        Assert(nBytes >= sizeof(UserdataFormatName));
        uint16_t formatName = *reinterpret_cast<uint16_t*>(data);
        if (formatName == 0) {
            NetworkUserdata userdata{};
            userdata.sender = client;
            userdata.data.assign(data + sizeof(UserdataFormatName), data + nBytes - sizeof(UserdataFormatName));
            userdata.reliable = false;
            onUserdataRecieved->Fire(userdata);
        }
        else {
            if (!formattedUserdataEvents.contains(formatName))
                DebugLogInfo("Unrecognized userdata format name ", formatName);
            formattedUserdataEvents[formatName](client, data + sizeof(UserdataFormatName), nBytes - sizeof(UserdataFormatName));
        }
    }
    else {
        Assert(nBytes > 0);
        uint8_t type = data[0];

        if (type == TRANSFORM_SYNC_PACKET_IDENTIFIER) { // then it's transform syncs
            HandleTransformSyncPacket(client, reinterpret_cast<PacketStructs::TransformSyncPacket*>(data), (nBytes - sizeof(PacketStructs::TransformSyncPacket)) / sizeof(PacketStructs::TransformSyncSnapshot));

        }
        else if (type == RIGIDBODY_SYNC_PACKET_IDENTIFIER) {
            HandleRigidbodySyncPacket(client, reinterpret_cast<PacketStructs::RigidbodySyncPacket*>(data), (nBytes - sizeof(PacketStructs::RigidbodySyncPacket)) / sizeof(PacketStructs::RigidbodySyncSnapshot));
        }
    }
}

void NetworkingEngine::ProcessShortMessageReliable(std::shared_ptr<Client>& client, AckId ackId, uint8_t* data, unsigned nBytes, bool isUserdata) {
    Assert(client && client->connection);

    //DebugLogInfo("Short reliable recieved.");
    if (client->connection->AlreadyRecievedPacket(ackId)) return;

    client->connection->AckData(ackId);

    if (isUserdata) {
        Assert(nBytes >= sizeof(UserdataFormatName));
        uint16_t formatName = *reinterpret_cast<uint16_t*>(data);
        if (formatName == 0) {
            NetworkUserdata userdata{};
            userdata.sender = client;
            userdata.data.assign(data + sizeof(UserdataFormatName), data + nBytes - sizeof(UserdataFormatName));
            userdata.reliable = true;
            onUserdataRecieved->Fire(userdata);
        }
        else {
            if (!formattedUserdataEvents.contains(formatName))
                DebugLogInfo("Unrecognized userdata format name ", formatName);
            formattedUserdataEvents[formatName](client, data + sizeof(UserdataFormatName), nBytes - sizeof(UserdataFormatName));
        }
    }
    else {
        Assert(nBytes > 0);
        uint8_t type = data[0];
        if (type == TRANSFORM_SYNC_PACKET_IDENTIFIER) { // then it's transform syncs
            HandleTransformSyncPacket(client, reinterpret_cast<PacketStructs::TransformSyncPacket*>(data), (nBytes - sizeof(PacketStructs::TransformSyncPacket)) / sizeof(PacketStructs::TransformSyncSnapshot));
            
        }
        else if (type == RIGIDBODY_SYNC_PACKET_IDENTIFIER) {
            HandleRigidbodySyncPacket(client, reinterpret_cast<PacketStructs::RigidbodySyncPacket*>(data), (nBytes - sizeof(PacketStructs::RigidbodySyncPacket)) / sizeof(PacketStructs::RigidbodySyncSnapshot));
        }
    }
}

void NetworkingEngine::ProcessLongMessageFragment(std::shared_ptr<Client>& client, AckId firstAckId, uint16_t idOffset, uint16_t nPackets, uint8_t* data, unsigned nBytes, bool isUserdata) {
    Assert(client && client->connection);
    AckId ackId = firstAckId + idOffset;

    //DebugLogInfo("Got long message fragment ", idOffset + 1, "/", nPackets);

    if (client->connection->AlreadyRecievedPacket(ackId)) return;

    client->connection->AckData(ackId);
    
    if (!client->connection->wipLongMessages.contains(firstAckId)) {
        //DebugLogInfo("Recieving new long message with ", nPackets, " fragments.");
        client->connection->wipLongMessages[firstAckId].firstPacketId = firstAckId;
        client->connection->wipLongMessages[firstAckId].numPackets = nPackets;   
    }
    std::vector<uint8_t> niceData;
    niceData.assign(data, data + nBytes);
    client->connection->wipLongMessages[firstAckId].packets.push_back(niceData);
    if (client->connection->wipLongMessages[firstAckId].packets.size() == nPackets) {
        if (isUserdata) {
            Assert(nBytes >= sizeof(UserdataFormatName));
            uint16_t formatName = *reinterpret_cast<uint16_t*>(data);
            std::vector<uint8_t> data;
            for (auto& pkt : client->connection->wipLongMessages[firstAckId].packets)
                data.insert(data.end(), pkt.begin(), pkt.end());

            if (formatName == 0) {
                NetworkUserdata userdata{};
                userdata.sender = client;
                userdata.data = data;
                userdata.reliable = true;
                onUserdataRecieved->Fire(userdata);
            }
            else {
                if (!formattedUserdataEvents.contains(formatName))
                    DebugLogInfo("Unrecognized userdata format name ", formatName);
                formattedUserdataEvents[formatName](client, data.data() + sizeof(UserdataFormatName), data.size() - sizeof(UserdataFormatName));
            }   
        }
        else {

        }

        client->connection->wipLongMessages.erase(firstAckId);
    }
}

void NetworkingEngine::SyncGameobjects() {
    auto localMachine = GetLocalMachine();
    
    std::vector<std::shared_ptr<Client>> clientsToSyncWith;
    if (status == NetworkStatus::Client) clientsToSyncWith.push_back(GetServer());
    else {
        for (auto& c : clients) {
            if (!c->isLocalMachine && c->connection && c->connection->completedHandshake) {
                clientsToSyncWith.push_back(c);
            }
        }
    }

    //DebugLogInfo(clientsToSyncWith.size(), " ", localMachine->ownedSyncedTransforms.size());
    if (clientsToSyncWith.empty()) return;

    unsigned maxPacketsPerFrame = 512; // TODO: SET DYNAMICALLY BASED ON INTELLIGENT CRITERIA

    constexpr unsigned TRANSFORM_SYNCS_PER_PACKET = Socket::MAX_PACKET_SIZE / sizeof(PacketStructs::TransformSyncSnapshot);
    constexpr unsigned RIGIDBODY_SYNCS_PER_PACKET = Socket::MAX_PACKET_SIZE / sizeof(PacketStructs::RigidbodySyncSnapshot);
    size_t transformPacketSize = sizeof(PacketStructs::TransformSyncPacket) + TRANSFORM_SYNCS_PER_PACKET * sizeof(PacketStructs::TransformSyncSnapshot);
    size_t rigidbodyPacketSize = sizeof(PacketStructs::RigidbodySyncPacket) + RIGIDBODY_SYNCS_PER_PACKET * sizeof(PacketStructs::RigidbodySyncSnapshot);

    void* ackedTransformPacket = nullptr;
    void* transformPacket = nullptr;
    void* ackedRigidbodyPacket = nullptr;
    void* rigidbodyPacket = nullptr;

    unsigned nAckedTransforms = 0;
    unsigned nTransforms = 0;
    unsigned nAckedRigidbodies = 0;
    unsigned nRigidbodies = 0;

    for (auto& [syncId, syncInfo] : localMachine->ownedSyncedTransforms) {
        if (syncInfo.ackTransformSnapshots && syncInfo.lastSentRigidbody.has_value()) {
            if (!ackedRigidbodyPacket) {
                ackedRigidbodyPacket = malloc(rigidbodyPacketSize);
                Assert(ackedRigidbodyPacket);
                ((PacketStructs::RigidbodySyncPacket*)ackedRigidbodyPacket)->identifier = RIGIDBODY_SYNC_PACKET_IDENTIFIER;
                ((PacketStructs::RigidbodySyncPacket*)ackedRigidbodyPacket)->tick = currentTick;

            }

            PacketStructs::RigidbodySyncSnapshot snapshot{
            .transform = GetNetworkTransform(syncInfo.gameObject),
            .rigidbody = GetNetworkRigidbody(syncInfo.gameObject),
            .syncId = syncId,
            };

            memcpy(reinterpret_cast<uint8_t*>(ackedRigidbodyPacket) + sizeof(PacketStructs::RigidbodySyncPacket) + nAckedRigidbodies * sizeof(PacketStructs::RigidbodySyncSnapshot), &snapshot, sizeof(snapshot));
            nAckedRigidbodies++;

            if (nAckedRigidbodies == RIGIDBODY_SYNCS_PER_PACKET) {
                for (auto& c : clientsToSyncWith) {
                    ImplSendDataReliable(ackedRigidbodyPacket, rigidbodyPacketSize, c, false);
                }

                free(ackedRigidbodyPacket);
                ackedRigidbodyPacket = nullptr;
                nAckedRigidbodies = 0;
            }
        }
        else if (syncInfo.ackTransformSnapshots) {
            if (!ackedTransformPacket) {
                ackedTransformPacket = malloc(transformPacketSize);
                Assert(ackedTransformPacket);
                ((PacketStructs::TransformSyncPacket*)ackedTransformPacket)->identifier = TRANSFORM_SYNC_PACKET_IDENTIFIER;
                ((PacketStructs::TransformSyncPacket*)ackedTransformPacket)->tick = currentTick;
            }

            PacketStructs::TransformSyncSnapshot snapshot{
            .transform = GetNetworkTransform(syncInfo.gameObject),
            .syncId = syncId,
            };

            memcpy(reinterpret_cast<uint8_t*>(ackedTransformPacket) + sizeof(PacketStructs::TransformSyncPacket) + nAckedTransforms * sizeof(PacketStructs::TransformSyncSnapshot), &snapshot, sizeof(snapshot));
            nAckedTransforms++;

            if (nAckedTransforms == TRANSFORM_SYNCS_PER_PACKET) {
                for (auto& c : clientsToSyncWith) {
                    ImplSendDataReliable(ackedTransformPacket, transformPacketSize, c, false);
                }

                free(ackedTransformPacket);
                ackedTransformPacket = nullptr;
                nAckedTransforms = 0;
            }
        }
        else if (syncInfo.lastSentRigidbody.has_value()) {
            if (!rigidbodyPacket) {
                rigidbodyPacket = malloc(rigidbodyPacketSize);
                Assert(rigidbodyPacket);
                ((PacketStructs::RigidbodySyncPacket*)rigidbodyPacket)->identifier = RIGIDBODY_SYNC_PACKET_IDENTIFIER;
                ((PacketStructs::RigidbodySyncPacket*)rigidbodyPacket)->tick = currentTick;
            }

            PacketStructs::RigidbodySyncSnapshot snapshot{
            .transform = GetNetworkTransform(syncInfo.gameObject),
            .rigidbody = GetNetworkRigidbody(syncInfo.gameObject),
            .syncId = syncId,
            };

            memcpy(reinterpret_cast<uint8_t*>(rigidbodyPacket) + sizeof(PacketStructs::RigidbodySyncPacket) + nRigidbodies * sizeof(PacketStructs::RigidbodySyncSnapshot), &snapshot, sizeof(snapshot));
            nRigidbodies++;

            if (nRigidbodies == RIGIDBODY_SYNCS_PER_PACKET) {
                for (auto& c : clientsToSyncWith) {
                    ImplSendData(rigidbodyPacket, rigidbodyPacketSize, c, false);
                }

                free(rigidbodyPacket);
                rigidbodyPacket = nullptr;
                nRigidbodies = 0;
            }
        }
        else {
            if (!transformPacket) {
                transformPacket = malloc(transformPacketSize);
                Assert(transformPacket);
                ((PacketStructs::TransformSyncPacket*)transformPacket)->identifier = TRANSFORM_SYNC_PACKET_IDENTIFIER;
                ((PacketStructs::TransformSyncPacket*)transformPacket)->tick = currentTick;
            }

            PacketStructs::TransformSyncSnapshot snapshot{
            .transform = GetNetworkTransform(syncInfo.gameObject),
            .syncId = syncId,
            };

            memcpy(reinterpret_cast<uint8_t*>(transformPacket) + sizeof(PacketStructs::TransformSyncPacket) * nTransforms * sizeof(PacketStructs::TransformSyncSnapshot), &snapshot, sizeof(snapshot));
            nTransforms++;

            if (nTransforms == TRANSFORM_SYNCS_PER_PACKET) {
                for (auto& c : clientsToSyncWith) {
                    ImplSendData(transformPacket, transformPacketSize, c, false);
                }

                free(transformPacket);
                transformPacket = nullptr;
                nTransforms = 0;
            }
        }

    }

    if (ackedRigidbodyPacket) {
        for (unsigned i = nAckedRigidbodies; i < RIGIDBODY_SYNCS_PER_PACKET; i++) {
            memset(reinterpret_cast<uint8_t*>(ackedRigidbodyPacket) + sizeof(PacketStructs::RigidbodySyncPacket) * nAckedRigidbodies * sizeof(PacketStructs::RigidbodySyncSnapshot), 0, sizeof(PacketStructs::RigidbodySyncSnapshot));
        }

        for (auto& c : clientsToSyncWith) {
            ImplSendDataReliable(ackedRigidbodyPacket, rigidbodyPacketSize, c, false);
        }

        free(ackedRigidbodyPacket);
    }

    if (rigidbodyPacket) {
        for (unsigned i = nRigidbodies; i < RIGIDBODY_SYNCS_PER_PACKET; i++) {
            memset(reinterpret_cast<uint8_t*>(rigidbodyPacket) + sizeof(PacketStructs::RigidbodySyncPacket) * nRigidbodies * sizeof(PacketStructs::RigidbodySyncSnapshot), 0, sizeof(PacketStructs::RigidbodySyncSnapshot));
        }

        for (auto& c : clientsToSyncWith) {
            ImplSendData(rigidbodyPacket, rigidbodyPacketSize, c, false);
        }

        free(rigidbodyPacket);
    }

    if (ackedTransformPacket) {
        for (unsigned i = nAckedTransforms; i < TRANSFORM_SYNCS_PER_PACKET; i++) {
            memset(reinterpret_cast<uint8_t*>(ackedTransformPacket) + sizeof(PacketStructs::TransformSyncPacket) * nAckedTransforms * sizeof(PacketStructs::TransformSyncSnapshot), 0, sizeof(PacketStructs::TransformSyncSnapshot));
        }

        for (auto& c : clientsToSyncWith) {
            ImplSendDataReliable(ackedTransformPacket, transformPacketSize, c, false);
        }

        free(ackedTransformPacket);
    }

    if (transformPacket) {
        for (unsigned i = nAckedRigidbodies; i < TRANSFORM_SYNCS_PER_PACKET; i++) {
            memset(reinterpret_cast<uint8_t*>(transformPacket) + sizeof(PacketStructs::TransformSyncPacket) * nTransforms * sizeof(PacketStructs::TransformSyncSnapshot), 0, sizeof(PacketStructs::TransformSyncSnapshot));
        }

        for (auto& c : clientsToSyncWith) {
            ImplSendData(transformPacket, transformPacketSize, c, false);
        }

        free(transformPacket);
    }
}

bool TickIsNewer(NetworkTickId previous, NetworkTickId maybeLater) {
    return maybeLater - previous < UINT16_MAX / 4;
}

void NetworkingEngine::HandleTransformSyncPacket(std::shared_ptr<Client>& client, PacketStructs::TransformSyncPacket* packet, unsigned nSnapshots) {
    //DebugLogInfo("Transform sync recieved; ", nSnapshots);
    
    for (unsigned i = 0; i < nSnapshots; i++) {
        auto id = packet->snapshots[i].syncId;
        if (client->ownedSyncedTransforms.contains(id)) {
            if (!std::isfinite(packet->snapshots[i].transform.position.x) || !std::isfinite(packet->snapshots[i].transform.position.y) || !std::isfinite(packet->snapshots[i].transform.position.z) || glm::length(packet->snapshots[i].transform.rotation) < 0.001) {
                DebugLogInfo("Recieved invalid network transform ", id);
            }
            else {
                //DebugLogInfo("Applying transform ", id);

                if (!client->ownedSyncedTransforms[id].recievedDataTick || TickIsNewer(*client->ownedSyncedTransforms[id].recievedDataTick, packet->tick)) {
                    client->ownedSyncedTransforms[id].recievedDataTick = packet->tick;
                    client->ownedSyncedTransforms[id].gameObject->RawGet<TransformComponent>()->SetPos(packet->snapshots[i].transform.position);
                    client->ownedSyncedTransforms[id].gameObject->RawGet<TransformComponent>()->SetRot(packet->snapshots[i].transform.rotation);
                }
                else DebugLogInfo(" Transform sync ", id, " was outdated ");
            }
        }
        else if (id != 0) {
            DebugLogInfo("Unrecognized transform sync name ", id);
        }
    }
}

void NetworkingEngine::HandleRigidbodySyncPacket(std::shared_ptr<Client>& client, PacketStructs::RigidbodySyncPacket* packet, unsigned nSnapshots) {
    //DebugLogInfo("Rigidbody sync packet recieved with ", nSnapshots, " snapshots");

    for (unsigned i = 0; i < nSnapshots; i++) {
        auto id = packet->snapshots[i].syncId;
        if (client->ownedSyncedTransforms.contains(id) && client->ownedSyncedTransforms[id].lastSentRigidbody) {
            if (!std::isfinite(packet->snapshots[i].transform.position.x) 
                || !std::isfinite(packet->snapshots[i].transform.position.y)
                || !std::isfinite(packet->snapshots[i].transform.position.z) 
                || !std::isfinite(packet->snapshots[i].rigidbody.velocity.x)
                || !std::isfinite(packet->snapshots[i].rigidbody.velocity.y)
                || !std::isfinite(packet->snapshots[i].rigidbody.velocity.z)
                || !std::isfinite(packet->snapshots[i].rigidbody.angularVelocity.x)
                || !std::isfinite(packet->snapshots[i].rigidbody.angularVelocity.y)
                || !std::isfinite(packet->snapshots[i].rigidbody.angularVelocity.z)
                || glm::length(packet->snapshots[i].transform.rotation) < 0.001) {
                DebugLogInfo("Recieved invalid network rigidbody", id);
            }
            else {
                //DebugLogInfo("Applying rigidbody sync ", id);

                if (!client->ownedSyncedTransforms[id].recievedDataTick || TickIsNewer(*client->ownedSyncedTransforms[id].recievedDataTick, packet->tick)) {
                    client->ownedSyncedTransforms[id].recievedDataTick = packet->tick;
                    client->ownedSyncedTransforms[id].gameObject->RawGet<TransformComponent>()->SetPos(packet->snapshots[i].transform.position);
                    client->ownedSyncedTransforms[id].gameObject->RawGet<TransformComponent>()->SetRot(packet->snapshots[i].transform.rotation);
                    client->ownedSyncedTransforms[id].gameObject->RawGet<RigidbodyComponent>()->velocity = packet->snapshots[i].rigidbody.velocity;
                    client->ownedSyncedTransforms[id].gameObject->RawGet<RigidbodyComponent>()->angularVelocity = packet->snapshots[i].rigidbody.angularVelocity;
                }
                else DebugLogInfo(" Rigidbody sync ", id, " was outdated ");
            }
        }
        else if (id != 0) {
            DebugLogInfo("Unrecognized rigidbody sync name ", id);
        }
    }
}

NetworkingEngine::NetworkingEngine():
    onConnectionAttemptComplete(Event<ConnectionAttemptResult>::New()),
    onInitialSyncComplete(Event<>::New()),
    onUserdataRecieved(Event<NetworkUserdata>::New()),
    onNewClient(Event<std::shared_ptr<Client>>::New())
{
    status = NetworkStatus::Offline;
    serverSocket = std::nullopt;
    currentTick = 0;
}

std::pair<bool, std::optional<std::string>> DefaultConnectionRequestHandler(std::string ipAddress, int port) {
    DebugLogInfo("Default connection request handler recieved request from ", ipAddress, ":", port);
    return { true, std::nullopt };
}