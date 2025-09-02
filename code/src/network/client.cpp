#include "client.hpp"

std::shared_ptr<Client> Client::New(bool server, int port, std::string address)
{
    return std::shared_ptr<Client>(new Client(server, port, address));
}

Client::Client(bool server, int port, std::string address):
    isServer(server),
    port(port),
    address(address)
{

}
