#pragma once
#include <bitset>

// AABBs inserted into the SAS will be scaled by this much so that if the object moves a little bit we don't need to update its position in the SAS. (TODO?)
static const inline double AABB_FAT_FACTOR = 1;

using CollisionLayer = uint16_t;

inline constexpr CollisionLayer MAX_COLLISION_LAYERS = 32;

using CollisionLayerSet = std::bitset<MAX_COLLISION_LAYERS>;
static const inline CollisionLayerSet ALL_COLLISION_LAYERS = CollisionLayerSet().set();

// For the broadphase of collisions, we use AABBs since it is easier/cheaper to determine when they are colliding
struct AABB {
    glm::dvec3 min;
    glm::dvec3 max;

    AABB(glm::dvec3 minPoint = {}, glm::dvec3 maxPoint = {}) {
        min = minPoint;
        max = maxPoint;
    }

    // makes this aabb expand to contain other
    void Grow(const AABB& other) {
        min.x = std::min(min.x, other.min.x);
        min.y = std::min(min.y, other.min.y);
        min.z = std::min(min.z, other.min.z);
        max.x = std::max(max.x, other.max.x);
        max.y = std::max(max.y, other.max.y);
        max.z = std::max(max.z, other.max.z);
    }

    // Checks if this AABB fully envelopes the other AABB 
    bool TestEnvelopes(const AABB& other) {
        return (min.x <= other.min.x && min.y <= other.min.y && min.z <= other.min.z && max.x >= other.max.x && max.y >= other.max.y && max.z >= other.max.z);
    }

    // returns true if they touching
    bool TestIntersection(const AABB& other) {
        return (min.x <= other.max.x && max.x >= other.min.x) && (min.y <= other.max.y && max.y >= other.min.y) && (min.z <= other.max.z && max.z >= other.min.z);
    }

    // returns true if they touching
    // uses https://tavianator.com/2011/ray_box.html 
    bool TestIntersection(const glm::dvec3& origin, const glm::dvec3& direction_inverse) {
        //std::printf("Testing AABB going from %f %f %f to %f %f %f\n.", min.x, min.y, min.z, max.x, max.y, max.z);

        double t1 = (min[0] - origin[0]) * direction_inverse[0];
        double t2 = (max[0] - origin[0]) * direction_inverse[0];

        double tmin = std::min(t1, t2);
        double tmax = std::max(t1, t2);

        for (int i = 1; i < 3; ++i) {
            t1 = (min[i] - origin[i]) * direction_inverse[i];
            t2 = (max[i] - origin[i]) * direction_inverse[i];

            tmin = std::min(std::max(t1, tmin), std::max(t2, tmin));
            tmax = std::max(std::min(t1, tmax), std::min(t2, tmax));
        }

        return tmax > std::max(tmin, 0.0);
    }

    // returns average of min and max
    glm::dvec3 Center() const {
        return (min + max) * 0.5;
    }

    // returns AABB's volume
    double Volume() const {
        auto m = max - min;
        return m.x * m.y * m.z;
    }   
};
