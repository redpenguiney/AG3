#pragma once
#include <optional>
#include <string>
#include <memory>

class Client {
public:

    static std::shared_ptr<Client> New(bool server, int port, std::string address);

    const bool isServer;
    const int port;
    const std::string address;

private:
    friend class NetworkingEngine;
    
    Client(bool server, int port, std::string address);

    // (SERVER ONLY) true if recieved completehandshake packet from client.
    bool completedHandshake = false;


};