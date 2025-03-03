#include "entity.hpp"
#include "world.hpp"

glm::ivec2 Entity::Pos() const
{
	const auto p = gameObject->RawGet<TransformComponent>()->Position();
	return { std::round(p.x), std::round(p.z) };
}

Entity::~Entity() {
	// there's no way an entity could be destroyed if it had a shared_ptr in here
	//GameObjectsToEntities().erase(gameObject.get());

	gameObject->Destroy();
}

void Entity::UpdateAll(float dt) {
	gameTime += dt;

	auto& SAS = SpatialAccelerationStructure::Get();
	CollisionLayerSet entityLayerSet = 0b0;
	entityLayerSet[ENTITY_COLLISION_LAYER] = true;

	for (auto& loader : World::Loaded()->chunkLoaders) {
		Assert(loader.radius > 0);
		// +/- 8 to reach chunk boundaries instead of chunk centres
		AABB loaderBounds(glm::dvec3(loader.centerPosition.x - loader.radius - 8, -16384.0, loader.centerPosition.y - loader.radius), glm::dvec3(loader.centerPosition.x + loader.radius, 16384.0, loader.centerPosition.y + loader.radius + 8));

		auto entityObjects = SAS.Query(loaderBounds, entityLayerSet);
		for (auto& obj : entityObjects) {
			auto entity = FromGameObject(obj->gameobject);
			Assert(entity);

			if (!entity->IsActive()) {
				entity->OnWakeup();
				entity->sleepTime = -1; // make entity active			
			}
			
			entity->stillActive = true;
		}
	}

	//DebugLogInfo("Updating ", Entities().size());

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

void Entity::Cleanup() {
	GameObjectsToEntities().clear();
	Entities().clear();
	gameTime = 0; 
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

	auto rigid = obj->MaybeRawGet<RigidbodyComponent>();
	if (rigid) {
		rigid->kinematic = true;
	}

	return obj;
}

Entity::Entity(const std::shared_ptr<Mesh>& mesh, const GameobjectCreateParams& goParams):
	gameObject(MakeGameObject(goParams))
{
	gameObject->RawGet<RenderComponent>()->SetColor({1, 1, 1, 1});
	gameObject->RawGet<RenderComponent>()->SetTextureZ(-1);
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
