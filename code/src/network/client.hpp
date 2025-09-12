#pragma once
#include <optional>
#include <string>
#include <memory>
#include <vector>
#include "protocol.hpp"

using AckId = uint32_t;

struct PendingAck {
    AckId ackId;
    double sentTimestamp;
    void* payload; // FREED ON PendingAck DESTRUCTION
    unsigned payloadNBytes;

    // takes ownership of payload
    PendingAck(const AckId id, double timestamp, void* data, unsigned dataSize) : ackId(id), sentTimestamp(timestamp), payload(data), payloadNBytes(dataSize) {}
    PendingAck(const PendingAck&) = delete;
    PendingAck(PendingAck&& old) noexcept: 
        ackId(old.ackId), 
        sentTimestamp(old.sentTimestamp), 
        payload(old.payload), 
        payloadNBytes(old.payloadNBytes) 
    {
        old.ackId = UINT32_MAX;
        old.payload = nullptr;
    }
    PendingAck& operator=(PendingAck&& rhs) {
        ackId = rhs.ackId;
        rhs.ackId = UINT32_MAX;
        sentTimestamp = rhs.sentTimestamp;
        payload = rhs.payload;
        rhs.payload = nullptr;
        payloadNBytes = rhs.payloadNBytes;
        return *this;
    }
    ~PendingAck() { free(payload); }
};

struct LongMessageReconstruction {
    std::vector<std::vector<uint8_t>> packets;
    unsigned numPackets;
    AckId firstPacketId;
};

class ConnectionInfo {
public:
    double lastMessageTime; // since epoch
    bool completedHandshake = false;

    std::vector<PendingAck> pendingAcks;

    // where long messages that have not been fully transmitted to the local machine are stored.
    // key is ackId of first packet
    std::unordered_map<AckId, LongMessageReconstruction> wipLongMessages;

    AckId nextAckId = 0;
    AckId GetAvailableAckId();

    void AckData(AckId ackId);
    // Returns the data to send. Free the given void*'s when you're finished.
    std::vector<std::pair<void*, size_t>> FlushAcksToSend();
    void ExpireRecievedPackets();
    bool AlreadyRecievedPacket(AckId ackId);

private:
    std::vector<AckId> acksToSend;

    // If someone we're connected to doesn't get our ack, they'll resend the data which is bad.
    // Value is the time of recieving
    std::unordered_map<AckId, double> recievedPackets;

    // How long we remember ackIds for recieved packets 
    static constexpr double PACKET_EXPIRATION_TIME = 300.0f;
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