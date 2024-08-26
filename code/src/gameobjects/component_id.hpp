#pragma once
#include "base_component.hpp"
#include <concepts>

#include "gameobjects/animation_component.hpp"
#include "render_component.hpp"
#include "collider_component.hpp"
#include "transform_component.hpp"
#include "pointlight_component.hpp"
#include "rigidbody_component.hpp"
#include "audio_player_component.hpp"
#include "spotlight_component.hpp"

namespace ComponentBitIndex {
	enum ComponentBitIndex {
		Transform = 0,
		Render = 1,
		Collider = 2,
		Rigidbody = 3,
		Pointlight = 4,
		RenderNoFO = 5,
		AudioPlayer = 6,
		Animation = 7,
		Spotlight = 8
	};
};

// How many different component classes there are. (can be greater just not less)
static inline const unsigned int N_COMPONENT_TYPES = 16;

template<std::derived_from<BaseComponent> T>
static constexpr ComponentBitIndex::ComponentBitIndex ComponentIdFromType();

template<> constexpr inline ComponentBitIndex::ComponentBitIndex ComponentIdFromType<TransformComponent>() {
    return ComponentBitIndex::Transform;
}
template<> constexpr inline ComponentBitIndex::ComponentBitIndex ComponentIdFromType<RenderComponent>() {
    return ComponentBitIndex::Render;
}
template<> constexpr inline ComponentBitIndex::ComponentBitIndex ComponentIdFromType<RenderComponentNoFO>() {
    return ComponentBitIndex::RenderNoFO;
}
template<> constexpr inline ComponentBitIndex::ComponentBitIndex ComponentIdFromType<ColliderComponent>() {
    return ComponentBitIndex::Collider;
}
template<> constexpr inline ComponentBitIndex::ComponentBitIndex ComponentIdFromType<RigidbodyComponent>() {
    return ComponentBitIndex::Rigidbody;
}
template<> constexpr inline ComponentBitIndex::ComponentBitIndex ComponentIdFromType<PointLightComponent>() {
    return ComponentBitIndex::Pointlight;
}
template<> constexpr inline ComponentBitIndex::ComponentBitIndex ComponentIdFromType<AudioPlayerComponent>() {
    return ComponentBitIndex::AudioPlayer;
}
template<> constexpr inline ComponentBitIndex::ComponentBitIndex ComponentIdFromType<AnimationComponent>() {
    return ComponentBitIndex::Animation;
}
template<> constexpr inline ComponentBitIndex::ComponentBitIndex ComponentIdFromType<SpotLightComponent>() {
    return ComponentBitIndex::Spotlight;
}