#pragma once
#include <memory>
#include "gameobjects/gameobject.hpp"


// Base class for anything in the game that needs to move/can't be expressed as a tile.
// Entities are active if they are currently being updated (because they're in range of a ChunkLoader).
// When an entity exits the range of a ChunkLoader and becomes inactive, OnSleep() is called on that entity. Then:
	// Some entities are persistent (depending on the individual, not the class), meaning that they will be stored in inactiveEntities when they become inactive.
		// Of those inactive but persistent entities, some will have take actions in the less frequently called InactiveThink().
		// That way, things like non-friendly NPCs can still perform (low resolution) tasks & movement offscreen without gobbling up performance.
	// Non-persistent entities will not be stored in inactiveEntities and will be destroyed (unless other references remain)
		// However, their OnSleep() method might store those entities in a different form (for example, a wildlife with a notable encounter with an NPC might be saved to be spawned in again at a later date)
class Entity {
public:
	static constexpr int ENTITY_COLLISION_LAYER = 2;

	// If true, then after OnSleep() is called the entity will be added to inactiveEntities. Either way it will be removed from activeEntities.
	bool persistent = true;

	std::shared_ptr<GameObject> gameObject;

	virtual ~Entity(); // virtual destructor required to ensure correct deletion by shared_ptr of subclasses

	// Loads and unloads entities based on whether they're within the bounds of a loaded chunk, then calls Think() on all the active entities
	// dt is delta simulation time in seconds.
	static void UpdateAll(float dt);
	
	// entity from gameobject; returns nullptr if gameobject does not belong to an entity
	static std::shared_ptr<Entity> FromGameObject(GameObject* obj);

	bool IsActive();

protected:

	Entity(const std::shared_ptr<Mesh>& mesh, const GameobjectCreateParams& goParams);

	// Called after an entity becomes active.
	virtual void OnWakeup();

	// Called before an entity becomes inactive. 
	virtual void OnSleep();

	// Makes the entity update its AI and do stuff and all that. (dt in simulation seconds)
	virtual void Think(float dt);

	// Like Think() but for inactive entities
	virtual void InactiveThink(float dt);

	// not private; subclasses factory methods need to append created entities to this vector
	// order is not preserved
	static std::vector<std::shared_ptr<Entity>>& Entities();

	// not private; subclasses factory methods need to append created entities to this vector
	static std::unordered_map<GameObject*, std::shared_ptr<Entity>>& GameObjectsToEntities();

private:
	double sleepTime = -1; // -1 if the object is active, otherwise the simulation time in seconds that the object became inactive at (relative to game start)	

	bool stillActive = false;

	// order is not preserved
	//static std::vector<std::shared_ptr<Entity>>& ActiveEntities();
	static inline double gameTime = 0.0; // simulation time in seconds that the game has gone on for

	
};