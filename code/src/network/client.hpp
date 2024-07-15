#pragma once
#include <optional>
#include <string>

class Client {
    public:
    // the client running on this machine.
    static std::optional<Client> localClient;

    // std::nullopt if this client IS the host. 
    std::optional<std::string> hostAddress;

    void Update();

};