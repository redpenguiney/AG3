#pragma once

// A structure that allows for fast queries of objects by position, which is needed for collision detection that isn't O(n^2).
class SpatialAccelerationStructure {
    public:
    SpatialAccelerationStructure(SpatialAccelerationStructure const&) = delete; // no copying
    SpatialAccelerationStructure& operator=(SpatialAccelerationStructure const&) = delete; // no assigning

    static SpatialAccelerationStructure& Get()
    {
        static SpatialAccelerationStructure sas; // yeah apparently you can have local static variables
        return sas;
    }

    private:
    

    SpatialAccelerationStructure() {

    }

    ~SpatialAccelerationStructure() {

    }
};