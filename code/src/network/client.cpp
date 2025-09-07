#include "client.hpp"
#include <debug/assert.hpp>

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
