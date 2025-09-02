#include "network.hpp"
#include "debug/assert.hpp"

// PROTOCOL listed here.
// Type of packets the engine sends.
enum class PacketType : uint8_t {
    // connection handshake types
    ConnectionRequest = 0, // format: 8 bit packet type followed by the client's 16 bit port. Called by client, to which server should respond with ConnectionRequestResponse
    ConnectionRequestResponse = 1, // format: 8 bit packet type, 8 bit bool (true if connection accepted, false if not), NOT-null-terminated string if connection was not accepted. Called by server in response to ConnectionRequest. Client will acknowledge this with CompleteConnectionHandshake to complete the connection.
    CompleteConnectionHandshake = 2, //  format: 8 bit packet type. Called by client in response to ConnectionRequestHandshakeResponse.

    // data transmission types
    SendShort = 3, // format: 8 bit packet type followed by the data. Reciever of the data will not acknowledge. Data must fit within 1 packet.
    SendShortAck = 4, // format: 8 bit packet type, then 16 bit packet id, followed by the data. Reciever of the data SHOULD acknowledge - this packet may be repeatedly sent until the sender recieves acknowledgement. Data must fit within 1 packet.
    SendLongHeader = 5, // format: 8 bit packet type, then 16 bit packet id, then 16 bit header id, and lastly 16 bit nPackets. Reciever should acknowledge.
    SendLongBody = 6, // format: 8 bit packet type, then 16 bit body number, then 16 bit packet id, then 16 bit header id, followed by the data. Reciever should acknowledge.

    // ack types
    AckArray = 7, // format : 8 bit packet type, then array of 16 bit packet ids that have been confirmed/acknowledged to be recieved. 
 
};

namespace PacketStructs {
    struct ConnectionRequest {
        PacketType type = PacketType::ConnectionRequest;
    };
    struct ConnectionRequestResponse {
        PacketType type;
        uint8_t connectionAccepted;
        //uint16_t messageLen;
        char rejectionReason[];
    };
    struct CompleteConnectionHandshake {
        PacketType type = PacketType::CompleteConnectionHandshake;
    };

    struct AckArray {
        PacketType type;
        uint16_t packets[];
    };
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

    clients.push_back(Client::New(true, port, "127.0.0.1"));
}

void NetworkingEngine::Unhost(){
    Assert(status == NetworkStatus::Server);

    for (int i = clients.size() - 1; i >= 0; i--) {
        Kick(clients[i], "Default server shutdown message.");
    }

    status = NetworkStatus::Offline;
}

void NetworkingEngine::Kick(std::shared_ptr<Client>& client, std::string reason) {

}

void NetworkingEngine::Connect(std::string ipAddress, int destPort, int localPort, float timeout, unsigned numTries) {
    Assert(timeout > 0 && numTries > 0);
    Assert(status == NetworkStatus::Offline);
    status = NetworkStatus::ClientConnecting;

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

                // Make sure client isn't already connected
                if (GetClient(p.originAddress, p.originPort)) continue;

                // Make sure we actually agreed to have this client be here
                if (!connectionRequestHandler(p.originAddress, p.originPort).first) continue;

                DebugLogInfo("Accepted new client");


                auto newClient = Client::New(false, p.originPort, p.originAddress);

                clients.push_back(newClient);
            }
        }

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
}

std::shared_ptr<Client> NetworkingEngine::GetClient(std::string address, int port) {
    for (auto& c : clients) {
        if (c->address == address && c->port == port) {
            return c;
        }
    }
    return nullptr;
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