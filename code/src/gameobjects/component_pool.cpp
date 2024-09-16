#include "component_pool.hpp"

//namespace {
//	constexpr std::array<short, N_COMPONENT_TYPES> BaseOffsetArray() {
//		std::array<short, N_COMPONENT_TYPES> offsets;
//		offsets.fill(-1);
//		return offsets;
//	}
//
//	// not stolen from stack overflow, surprisingly
//	// i'll be a real template metaprogrammer some day!
//	template <std::derived_from<BaseComponent> ... Components>
//	std::vector<short> ComponentOffsets() {
//
//		std::vector<short> offsets;
//
//		short offset = 0;
//
//		using I = short[];
//		(void)(I{ 0u, (offsets.push_back(offset), offset += sizeof(Components))... });
//		return offsets;
//	}
//}

std::pair<unsigned int, std::vector<ComponentPool::ComponentMemoryInfo>> GetArchetypeLayoutAndSize(const std::bitset<N_COMPONENT_TYPES>& components) {

	unsigned int currentOffset = sizeof(void*);
	std::vector<ComponentPool::ComponentMemoryInfo> layout;

	// NOTE: VERY IMPORTANT THAT THIS IS SORTED BY BIT INDEX. (TODO might not actually matter lol)
	if (components[ComponentBitIndex::Transform]) {
		layout.push_back(ComponentPool::ComponentMemoryInfo{ .size = sizeof(TransformComponent), .offset = currentOffset, .componentId = ComponentBitIndex::Transform });
		currentOffset += sizeof(TransformComponent);
	}
	if (components[ComponentBitIndex::Render]) {
		layout.push_back(ComponentPool::ComponentMemoryInfo{ .size = sizeof(RenderComponent), .offset = currentOffset, .componentId = ComponentBitIndex::Render });
		currentOffset += sizeof(TransformComponent);
	}
	if (components[ComponentBitIndex::Collider]) {
		layout.push_back(ComponentPool::ComponentMemoryInfo{ .size = sizeof(ColliderComponent), .offset = currentOffset, .componentId = ComponentBitIndex::Collider });
		currentOffset += sizeof(TransformComponent);
	}
	if (components[ComponentBitIndex::Rigidbody]) {
		layout.push_back(ComponentPool::ComponentMemoryInfo{ .size = sizeof(RigidbodyComponent), .offset = currentOffset, .componentId = ComponentBitIndex::Rigidbody });
		currentOffset += sizeof(TransformComponent);
	}
	if (components[ComponentBitIndex::Pointlight]) {
		layout.push_back(ComponentPool::ComponentMemoryInfo{ .size = sizeof(PointLightComponent), .offset = currentOffset, .componentId = ComponentBitIndex::Pointlight });
		currentOffset += sizeof(TransformComponent);
	}
	if (components[ComponentBitIndex::RenderNoFO]) {
		layout.push_back(ComponentPool::ComponentMemoryInfo{ .size = sizeof(RenderComponentNoFO), .offset = currentOffset, .componentId = ComponentBitIndex::RenderNoFO });
		currentOffset += sizeof(RenderComponentNoFO);
	}
	if (components[ComponentBitIndex::AudioPlayer]) {
		layout.push_back(ComponentPool::ComponentMemoryInfo{ .size = sizeof(AudioPlayerComponent), .offset = currentOffset, .componentId = ComponentBitIndex::AudioPlayer });
		currentOffset += sizeof(AudioPlayerComponent);
	}
	if (components[ComponentBitIndex::Animation]) {
		layout.push_back(ComponentPool::ComponentMemoryInfo{ .size = sizeof(AnimationComponent), .offset = currentOffset, .componentId = ComponentBitIndex::Animation });
		currentOffset += sizeof(AnimationComponent);
	}
	if (components[ComponentBitIndex::Spotlight]) {
		layout.push_back(ComponentPool::ComponentMemoryInfo{ .size = sizeof(SpotLightComponent), .offset = currentOffset, .componentId = ComponentBitIndex::Spotlight });
		currentOffset += sizeof(SpotLightComponent);
	}

	return { currentOffset, layout };
}

unsigned int GetArchetypeSize(const std::bitset<N_COMPONENT_TYPES>& components) {
	return GetArchetypeLayoutAndSize(components).first;
}

std::vector<ComponentPool::ComponentMemoryInfo> GetArchetypeLayout(const std::bitset<N_COMPONENT_TYPES>& components) {
	return GetArchetypeLayoutAndSize(components).second;
}

ComponentPool::ComponentPool(const std::bitset<N_COMPONENT_TYPES>& components):
	componentLayout(GetArchetypeLayout(components)),
	objectSize(GetArchetypeSize(components))
{
	AddPage();
}

ComponentPool::~ComponentPool() {
	for (auto & ptr: pages) {
		delete ptr;
	}
}

std::tuple<void*, int, int> ComponentPool::GetObject() {
	uint8_t* foundComponents = nullptr;
	unsigned int pageI = 0;

	// We use something called a "free list" to find a component set. If a component is not in use, the start of its memory is a pointer to the next unallocated component.
	for (pageI = 0; pageI < firstFree.size(); pageI++) {
		auto ptr = firstFree[pageI];
		if (ptr != nullptr) {
			// then this component will work
			foundComponents = ptr;

			// this ptr is pointing to a ptr to the next component set on this page (or LAST_COMPONENT if none); update firstAvailable which that
			firstFree[pageI] = *(uint8_t**)foundComponents;

			break;
		}
	}

	// if none of the pages have any space, make a new one
	if (!foundComponents) {
		AddPage();
		pageI = static_cast<unsigned int>(pages.size() - 1);
		foundComponents = firstFree.back();
		firstFree.back() = *(uint8_t**)foundComponents;
	}

	// mark the ptr we got as in use
	*(void**)foundComponents = nullptr;

	unsigned int index = static_cast<int>(((char*)foundComponents - (char*)pages[pageI]) / objectSize);
	Assert(foundComponents != nullptr);

	return std::make_tuple(foundComponents, index, pageI);
}

void ComponentPool::ReturnObject(int pageIndex, int objectIndex) {
	// call object destructors
	uint8_t* components = pages.at(pageIndex) + (objectIndex * objectSize);

	// make the components the first node in the free list and have it point to what was previously the first node
	*(uint8_t**)components = firstFree.at(pageIndex);
	firstFree.at(pageIndex) = components;
}

void ComponentPool::AddPage() {

	unsigned int index = static_cast<int>(pages.size());
	uint8_t* newPage = new uint8_t[COMPONENTS_PER_PAGE * objectSize];

	firstFree.push_back(newPage);
	pages.push_back(newPage);

	// for free list, we have to, for each location a set of components would be written, write a pointer to the next such location
	for (unsigned int i = 0; i < COMPONENTS_PER_PAGE - 1; i++) {

		*((uint8_t**)(newPage + (objectSize * i))) = ((uint8_t*)newPage + (objectSize * (i + 1)));
	}
	// last location just has LAST_COMPONENT/end of free list
	*((uint8_t**)(newPage + (objectSize * (COMPONENTS_PER_PAGE - 1)))) = LAST_COMPONENT;
}