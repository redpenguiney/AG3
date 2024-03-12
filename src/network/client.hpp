#pragma once
#include <optional>
#include <string>

class Client {
    public:

    // std::nullopt if this client IS the host. 
    std::optional<std::string> hostAddress;

    void Update();
};