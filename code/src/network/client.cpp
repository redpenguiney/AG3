#include "client.hpp"
#include "packet_types.hpp"
#include <debug/assert.hpp>
#include <utility/utility.hpp>

std::shared_ptr<Client> Client::New(bool server, bool local, int port, std::string address)
{
    return std::shared_ptr<Client>(new Client(server, local, port, address));
}

Client::Client(bool server, bool local, int port, std::string address) :
    isServer(server),
    isLocalMachine(local),
    port(port),
    address(address)
{

}

AckId ConnectionInfo::GetAvailableAckId()
{
    for (auto& ack : pendingAcks) {
        Assert(ack.ackId != nextAckId);
    }
    return nextAckId++; // overflow is fine.
    
}

void ConnectionInfo::AckData(AckId ackId) {
    Assert(!AlreadyRecievedPacket(ackId));
    recievedPackets[ackId] = Time();
    acksToSend.push_back(ackId);
}

std::vector<std::pair<void*, size_t>> ConnectionInfo::FlushAcksToSend() {
    std::vector<std::pair<void*, size_t>> packets;
    while (!acksToSend.empty()) {
        unsigned nAcks = std::min(acksToSend.size(), 500 / sizeof(AckId));
        auto acks = (PacketStructs::AckArray*)malloc(sizeof(PacketStructs::AckArray) + nAcks * 4);

        acks->type = PacketType::AckArray;
        memcpy(acks->packets, acksToSend.data() + acksToSend.size() - nAcks, nAcks * sizeof(AckId));
        for (unsigned i = 0; i < nAcks; i++) acksToSend.pop_back();

        packets.push_back(std::make_pair(acks, sizeof(PacketStructs::AckArray) + nAcks * 4));
    }

    return packets;
}

void ConnectionInfo::ExpireRecievedPackets() {
    auto t = Time();
    for (auto it = recievedPackets.begin(); it != recievedPackets.end(); it++) {
        if (it->second + PACKET_EXPIRATION_TIME < t)
            it = recievedPackets.erase(it);
    }
}

bool ConnectionInfo::AlreadyRecievedPacket(AckId ackId)
{
    return recievedPackets.contains(ackId);
}
