#include "network.hpp"
#include "debug/assert.hpp"

// PROTOCOL listed here.
// Type of packets the engine sends.
enum class PacketType : uint8_t {
    // connection handshake types
    ConnectionRequest = 0, // format: 8 bit packet type followed by the client's 16 bit port. Called by client, to which server should respond with ConnectionRequestResponse
    ConnectionRequestResponse = 1, // format: 8 bit packet type, 8 bit bool (true if connection accepted, false if not), null-terminated cstring if connection was not expected. Called by server in response to ConnectionRequest. Client will acknowledge this with CompleteConnectionHandshake to complete the connection.
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
        PacketType type;
        uint16_t port;
    };
    struct ConnectionRequestResponse {
        PacketType type;
        uint8_t connectionAccepted;
        char rejectionReason[];
    };
    struct CompleteConnectionHandshake {
        PacketType type;

    };
};


NetworkingEngine& NetworkingEngine::Get()
{
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
    //serverSocket.emplace(Socket(std::nullopt, ))
}

void NetworkingEngine::Connect(std::string ipAddress, int port, float timeout)
{
    Assert(status == NetworkStatus::Offline);
    status = NetworkStatus::Client;


}

void NetworkingEngine::Update()
{
    
}

NetworkingEngine::NetworkingEngine():
    onConnectionAttemptComplete(Event<ConnectionAttemptResult>::New()),
    onInitialSyncComplete(Event<>::New())
{
    status = NetworkStatus::Offline;
    serverSocket = std::nullopt;
}