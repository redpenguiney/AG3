#pragma once

// Component pool needs components to have a few fields, so this makes sure it does.
// TODO: how to make this compatible with free list memory optimization?
class BaseComponent {
    public:
    // not private bc i couldn't make it work
    bool live; // indicates that the component is in use
    void* next; // pointer to next available component in pool, void* so it works for all types
    unsigned int componentPoolId; // index into pools vector
};