#pragma once

// PROTOCOL listed here.

// (We don't need to worry about serialized size since the types used are always the same size.)

// Type of packets the engine sends.
// Uppermost bit of this will set set to 1 if this packet is userdata, 0 if enginedata.
namespace PacketType {
    enum PacketType : uint8_t {
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
}

using AckId = uint32_t;
using NetworkTickId = uint16_t;
using SyncId = uint32_t;

constexpr uint8_t TRANSFORM_SYNC_PACKET_IDENTIFIER = 201;
constexpr uint8_t RIGIDBODY_SYNC_PACKET_IDENTIFIER = 202;
constexpr uint8_t CHANGE_OWNER_PACKET_IDENTIFIER = 202;

struct TransformSync {
    glm::dvec3 position;
    glm::quat rotation;
};

struct RigidbodySync {
    // note that rigidbodies actually use a dvec3 for velocity. 
    // TODO: If we cared, we would quantize all synced rigidbodies to prevent rounding from creating a server-client discrepancy.
    glm::vec3 velocity; 
    glm::vec3 angularVelocity;
};

namespace PacketStructs {
#pragma pack(push, 1)
    struct ConnectionRequest {
        uint8_t type = PacketType::ConnectionRequest;
    };
    struct ConnectionRequestResponse {
        uint8_t type;
        uint8_t connectionAccepted;
        char rejectionReason[];

        ConnectionRequestResponse() = delete;
    };
    struct CompleteConnectionHandshake {
        uint8_t type = PacketType::CompleteConnectionHandshake;
    };

    struct AckArray {
        uint8_t type = PacketType::AckArray;
        AckId packets[];

        AckArray() = delete;
    };

    struct ShortMessage {
        uint8_t type = PacketType::SendShort;
        uint8_t data[];

        ShortMessage() = delete;
    };

    struct ShortMessageReliable {
        uint8_t type = PacketType::SendShortAck;
        AckId ackId;
        uint8_t data[];

        ShortMessageReliable() = delete;
    };

    struct LongMessage {
        uint8_t type = PacketType::LongMessage;
        uint16_t offset; // firstPacketAckId + offset = ackId of this packet
        AckId firstPacketAckId;
        uint16_t numPackets;
        uint8_t data[];

        LongMessage() = delete;
    };

    struct TransformSyncSnapshot {
        TransformSync transform;
        SyncId syncId;
    };

    struct TransformSyncPacket {
        uint8_t identifier;
        
        // It's necessary to let recievers know which order the snapshots were sent in, so that they apply the most recent snapshot they have (packets can arrive out of order).
        NetworkTickId tick;
        TransformSyncSnapshot snapshots[];

        TransformSyncPacket() = delete;
    };

    struct RigidbodySyncSnapshot {

        TransformSync transform;
        RigidbodySync rigidbody;
        SyncId syncId;
    };

    struct RigidbodySyncPacket {
        uint8_t identifier;

        // It's necessary to let recievers know which order the snapshots were sent in, so that they apply the most recent snapshot they have (packets can arrive out of order).
        NetworkTickId tick;
        RigidbodySyncSnapshot snapshots[];

        RigidbodySyncPacket() = delete;
    };

#pragma pack(pop)
};

