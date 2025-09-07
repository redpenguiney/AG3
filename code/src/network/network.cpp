#include "network.hpp"
#include "debug/assert.hpp"
#include <utility/utility.hpp>

// PROTOCOL listed here.
// Type of packets the engine sends.
enum class PacketType : uint8_t {
    // connection types
    ConnectionRequest = 0, // format: 8 bit packet type followed by the client's 16 bit port. Called by client, to which server should respond with ConnectionRequestResponse
    ConnectionRequestResponse = 1, // format: 8 bit packet type, 8 bit bool (true if connection accepted, false if not), NOT-null-terminated string if connection was not accepted. Called by server in response to ConnectionRequest. Client will acknowledge this with CompleteConnectionHandshake to complete the connection.
    CompleteConnectionHandshake = 2, //  format: 8 bit packet type. Called by client in response to ConnectionRequestHandshakeResponse.
    //TerminateConnection = 3, // format: 8 bit packet type, then optional non-null terminated string with reason. Used by server to tell a client they have been kicked/dropped, or for a client to request being kicked/dropped.

    // data transmission types
    SendShort = 4, // format: 8 bit packet type followed by the data. Reciever of the data will not acknowledge. Data must fit within 1 packet.
    SendShortAck = 5, // format: 8 bit packet type, then 16 bit packet id, followed by the data. Reciever of the data SHOULD acknowledge - this packet may be repeatedly sent until the sender recieves acknowledgement. Data must fit within 1 packet.
    LongMessage = 6, // format: 8 bit packet type, then 16 bit packet id, then 16 bit packet id of first packet in long message, then 16 bit num of packets in long message. Reciever should acknowledge.

    // ack types
    AckArray = 7, // format : 8 bit packet type, then array of 16 bit packet ids that have been confirmed/acknowledged to be recieved. 
 
};

namespace PacketStructs {
#pragma pack(push, 1)
    struct ConnectionRequest {
        PacketType type = PacketType::ConnectionRequest;
    };
    struct ConnectionRequestResponse {
        PacketType type;
        uint8_t connectionAccepted;
        //uint16_t messageLen;
        char rejectionReason[];

        ConnectionRequestResponse() = delete;
    };
    struct CompleteConnectionHandshake {
        PacketType type = PacketType::CompleteConnectionHandshake;
    };

    struct AckArray {
        PacketType type = PacketType::AckArray;
        AckId packets[];

        AckArray() = delete;
    };

    struct ShortMessage {
        PacketType type = PacketType::SendShort;
        uint8_t data[];

        ShortMessage() = delete;
    };

    struct ShortMessageReliable {
        PacketType type = PacketType::SendShortAck;
        AckId ackId;
        uint8_t data[];

        ShortMessageReliable() = delete;
    };

    struct LongMessage {
        PacketType type = PacketType::LongMessage;
        uint16_t offset; // firstPacketAckId + offset = ackId of this packet
        AckId firstPacketAckId;
        uint16_t numPackets;
        uint8_t data[];

        LongMessage() = delete;
    };
#pragma pack(pop)
};


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

    // Handle connection requests
    if (status == NetworkStatus::Server) {
        auto packets = serverSocket->Recieve();

        for (auto& p : packets) {
            Assert(p.data.size() > 0);
            auto packetType = (PacketType*)p.data.data();

            if (*packetType == PacketType::ConnectionRequest) {
                // Make sure client isn't already connected
                if (GetClient(p.originAddress, p.originPort)) continue;

                auto [accepted, reason] = connectionRequestHandler(p.originAddress, p.originPort);

                if (accepted) {
                    size_t size = sizeof(PacketStructs::ConnectionRequestResponse);
                    auto response = static_cast<PacketStructs::ConnectionRequestResponse*>(malloc(size));

                    response->type = PacketType::ConnectionRequestResponse;
                    response->connectionAccepted = true;

                    serverSocket->Send(p.originAddress, p.originPort, response, size);


                    DebugLogInfo("Accepted new client (awaiting handshake).");

                    auto newClient = Client::New(false, false, p.originPort, p.originAddress);
                    newClient->connection = ConnectionInfo{
                        .lastMessageTime = p.timestamp,
                        .completedHandshake = false
                    };
                }
                else {
                    std::string rejectionReason = reason.value_or("");

                    size_t size = sizeof(PacketStructs::ConnectionRequestResponse) + rejectionReason.length();
                    auto response = static_cast<PacketStructs::ConnectionRequestResponse*>(malloc(size));
                
                    response->type = PacketType::ConnectionRequestResponse;
                    response->connectionAccepted = false;
                    memcpy(response + 1, rejectionReason.c_str(), rejectionReason.length());

                    serverSocket->Send(p.originAddress, p.originPort, response, size);
                }
            }
            else if (*packetType == PacketType::CompleteConnectionHandshake) {

                auto client = GetClient(p.originAddress, p.originPort);

                if (!client) continue; 

                DebugLogInfo("Completed connection handshake.");

                client->connection->lastMessageTime = p.timestamp;

            }
        }

        ResendUnackedMessages();
    }
    else if (status == NetworkStatus::ClientConnecting) {
        auto packets = serverSocket->Recieve();
        for (auto& p : packets) {
            Assert(p.data.size() > 0);

            if (p.originAddress != targetConnectionIp) continue;

            auto packetType = (PacketType*)p.data.data();
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

                    connection = ConnectionInfo{
                        .lastMessageTime = p.timestamp,
                        .completedHandshake = true,
                    };

                    clients.push_back(Client::New(true, false, p.originPort, p.originAddress));
                }
            }
        }

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
    else if (status == NetworkStatus::Client) {
        auto packets = serverSocket->Recieve();
        for (auto& p : packets) {
            Assert(p.data.size() > 0);

            if (p.originAddress != targetConnectionIp) continue;

            auto packetType = (PacketType*)p.data.data();
            if (*packetType == PacketType::ConnectionRequestResponse) {
                // If this happens, then the server already said we can join but then didn't recieve our last CompleteConnectionHandshake packet, so we'll send it again.
                PacketStructs::CompleteConnectionHandshake handshake;
                serverSocket->Send(&handshake, sizeof(handshake));
            }
           
        }

        ResendUnackedMessages();
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

void NetworkingEngine::SendDataReliable(void* data, size_t nBytes, Client& destination)
{
    Assert(!destination.isLocalMachine);
    if (status == NetworkStatus::Client) {
        Assert(destination.isServer);
    }
    else Assert(status == NetworkStatus::Server);
    Assert(destination.connection);
    if (nBytes <= 500) {
        PacketStructs::ShortMessageReliable* packet = (PacketStructs::ShortMessageReliable*)malloc(nBytes + sizeof(PacketStructs::ShortMessageReliable));
        packet->type = PacketType::SendShortAck;
        packet->ackId = destination.connection->GetAvailableAckId();

        void* payload = malloc(nBytes);
        memcpy(payload, data, nBytes);

        destination.connection->pendingAcks.emplace_back(packet->ackId, Time(), payload, nBytes);
        memcpy(packet + sizeof(packet), data, nBytes);
        serverSocket->Send(packet, nBytes + sizeof(PacketStructs::ShortMessageReliable));
    }
    else {
        uint16_t i = 0;
        AckId firstAck = destination.connection->GetAvailableAckId();
        auto t = Time();
        while (nBytes > 0) {
            unsigned amtToSend = nBytes > 500 ? 500 : nBytes;
            nBytes -= amtToSend;
            
            PacketStructs::LongMessage* packet = (PacketStructs::LongMessage*)malloc(amtToSend + sizeof(PacketStructs::LongMessage));
            
            void* payload = malloc(amtToSend);
            memcpy(payload, data, amtToSend);

            destination.connection->pendingAcks.emplace_back(firstAck + i, t, payload, amtToSend);

            packet->type = PacketType::LongMessage;
            packet->offset = i++;
            packet->firstPacketAckId = firstAck;
           
            memcpy(packet + sizeof(packet), data, amtToSend);
            data = (uint8_t*)data + amtToSend;
            serverSocket->Send(packet, amtToSend + sizeof(PacketStructs::LongMessage));
        }
    }
}

void NetworkingEngine::SendData(void* data, size_t nBytes, Client& destination) {
    Assert(nBytes <= 500);
    Assert(!destination.isLocalMachine);
    if (status == NetworkStatus::Client) {
        Assert(destination.isServer);
    }
    else Assert(status == NetworkStatus::Server);

    PacketStructs::ShortMessage* packet = (PacketStructs::ShortMessage*)malloc(nBytes + sizeof(PacketStructs::ShortMessage));
    packet->type = PacketType::SendShort;
    memcpy(packet + sizeof(packet->type), data, nBytes);
    serverSocket->Send(packet, nBytes + sizeof(PacketStructs::ShortMessage));
}

void NetworkingEngine::HandleAck(Socket::Packet& p) {
    auto client = GetClient(p.originAddress, p.originPort);
    if (!client || !client->connection) return;
    client->connection->lastMessageTime = p.timestamp;

    Assert(p.data.size() > sizeof(PacketStructs::AckArray));
    auto packetType = (PacketType*)p.data.data();
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
    for (auto it = clients.begin(); it != clients.end(); it++) {
        auto& c = *it;
        if (!c->connection) continue;

        for (auto& ack : c->connection->pendingAcks) {
            if (ack.sentTimestamp + RESEND_PACKET_TIME < t) {
                
                ack.sentTimestamp = t;
            }
        }

        if (c->connection->lastMessageTime + TIMEOUT_TIME < t) {
            // Drop connection
            if (c->isServer) {
                Disconnect();
            }
            else {
                // TODO: do more?
                it = clients.erase(it);
            }
        }

    }
}

NetworkingEngine::NetworkingEngine():
    onConnectionAttemptComplete(Event<ConnectionAttemptResult>::New()),
    onInitialSyncComplete(Event<>::New())
{
    status = NetworkStatus::Offline;
    serverSocket = std::nullopt;
}

std::pair<bool, std::optional<std::string>> DefaultConnectionRequestHandler(std::string ipAddress, int port) {
    DebugLogInfo("Default connection request handler recieved request from ", ipAddress, ":", port);
    return { true, std::nullopt };
}