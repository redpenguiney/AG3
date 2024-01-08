#include "lifetime.hpp"
#include "../utility/utility.hpp"
#include <utility>
#include <vector>

// stores pairs of the gameobject and the time at which it should be destroyed.
std::vector<std::pair<std::shared_ptr<GameObject>, double>> OBJECT_LIFETIMES;

void NewObjectLifetime(std::shared_ptr<GameObject>& object, double secondsToLive) {
    OBJECT_LIFETIMES.emplace_back(std::make_pair(object, Time() + secondsToLive));
}

void UpdateLifetimes() {
    auto currentTime = Time();
    for (auto & [object, destructionTime] : OBJECT_LIFETIMES) {
        if (currentTime >= destructionTime) {
            object->Destroy();
        }
    }
}