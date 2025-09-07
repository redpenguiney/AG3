#pragma once
#include <optional>
#include <string>
#include <memory>
#include <vector>

using AckId = uint32_t;

struct PendingAck {
    const AckId ackId;
    double sentTimestamp;
    void* payload; // FREED ON PendingAck DESTRUCTION
    const unsigned payloadNBytes;

    PendingAck(const AckId id, double timestamp, void* data, unsigned dataSize) : ackId(id), sentTimestamp(timestamp), payload(data), payloadNBytes(dataSize) {}
    PendingAck(const PendingAck&) = delete;
    PendingAck(PendingAck&& old) noexcept: 
        ackId(old.ackId), 
        sentTimestamp(old.sentTimestamp), 
        payload(old.payload), 
        payloadNBytes(old.payloadNBytes) 
    {
        old.payload = nullptr;
    }
    ~PendingAck() { free(payload); }
};

struct ConnectionInfo {
    double lastMessageTime; // since epoch
    bool completedHandshake = false;

    std::vector<PendingAck> pendingAcks;

    AckId nextAckId = 0;
    AckId GetAvailableAckId();
};

class Client {
public:

    static std::shared_ptr<Client> New(bool server, bool local, int port, std::string address);

    const bool isLocalMachine;
    const bool isServer;
    const int port;
    const std::string address;

private:
    friend class NetworkingEngine;
    
    Client(bool server, bool local, int port, std::string address);

    // nullopt if isLocalMachine or the local machine is not directly connected to this client
    std::optional<ConnectionInfo> connection;
};