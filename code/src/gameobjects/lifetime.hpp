#include "component_registry.hpp"
#include <memory>

// gives the given object the given amount of time to live before they are destroyed.
// at the beginning of this frame objects that this function has been called on will be checked to see if they need to be destroyed.
// if the gameobject is destroyed before its time runs out, that's fine.
void NewObjectLifetime(std::shared_ptr<GameObject>& object, double secondsToLive);

// Destroys objects that NewObjectLifetime() was called on, if it's their time.
void UpdateLifetimes();