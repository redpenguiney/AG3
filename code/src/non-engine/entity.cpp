#include "entity.hpp"
#include "world.hpp"

Entity::~Entity() {
	gameObject->Destroy();
}

void Entity::UpdateAll(float dt) {
	gameTime += dt;

	auto& SAS = SpatialAccelerationStructure::Get();
	CollisionLayerSet entityLayerSet = 0b0;
	entityLayerSet[ENTITY_COLLISION_LAYER] = true;

	for (auto& loader : World::Loaded()->chunkLoaders) {
		AABB loaderBounds(glm::dvec3(loader.centerPosition - loader.radius, -16384.0), glm::dvec3(loader.centerPosition + loader.radius, 16384.0));

		auto entityObjects = SAS.Query(loaderBounds, entityLayerSet);
		for (auto& obj : entityObjects) {
			auto entity = FromGameObject(obj->gameobject);
			Assert(entity);

			if (!entity->IsActive()) {
				entity->sleepTime = -1; // make entity active
				entity->OnWakeup();
			}
			
			entity->stillActive = true;
		}
	}

	for (auto& entity : Entities()) {
		if (entity->IsActive()) {
			if (!entity->stillActive) { // then entity became inactive this frame
				entity->OnSleep();
				entity->sleepTime = gameTime; // make entity inactive
			}
			else {
				entity->Think(dt);
			}	
		}
		else {
			//entity->InactiveThink() TODO
		}
		entity->stillActive = false;
	}
}

std::shared_ptr<Entity> Entity::FromGameObject(GameObject* obj) {
	if (GameObjectsToEntities().contains(obj)) {
		return GameObjectsToEntities()[obj];
	}
	else {
		return nullptr;
	}
}

bool Entity::IsActive() {
	return sleepTime == -1;
}

std::shared_ptr<GameObject> MakeGameObject(const GameobjectCreateParams& params) {
	Assert(params.HasComponent(ComponentBitIndex::Collider));

	auto obj = GameObject::New(params);
	obj->RawGet<ColliderComponent>()->SetCollisionLayer(Entity::ENTITY_COLLISION_LAYER);

	return obj;
}

Entity::Entity(const std::shared_ptr<Mesh>& mesh, const GameobjectCreateParams& goParams):
	gameObject(MakeGameObject(goParams))
{

}

void Entity::OnWakeup() {
	float timeAsleep = gameTime - sleepTime;
}

void Entity::OnSleep() {
	
}

void Entity::Think(float dt) {

}

void Entity::InactiveThink(float dt) {

}

std::vector<std::shared_ptr<Entity>>& Entity::Entities()
{
	static std::vector<std::shared_ptr<Entity>> v;
	return v;
}

//std::vector<std::shared_ptr<Entity>>& Entity::ActiveEntities()
//{
//	static std::vector<std::shared_ptr<Entity>> av;
//	return av;
//}

std::unordered_map<GameObject*, std::shared_ptr<Entity>>& Entity::GameObjectsToEntities()
{
	static std::unordered_map<GameObject*, std::shared_ptr<Entity>> e;
	return e;
}
