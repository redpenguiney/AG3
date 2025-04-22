#include "graphics_test.hpp"
#include "gameobjects/gameobject.hpp"
#include "gameobject_tests.hpp"

// I will find the graphics bugs. Once. And. For all.
void TestGraphics() {
	TestSkybox();

	static std::vector<std::shared_ptr<GameObject>> cubes = {};
	static std::vector<std::shared_ptr<GameObject>> spheres = {};

	GraphicsEngine::Get().SetDebugFreecamEnabled(true);
	GraphicsEngine::Get().SetWireframeEnabled(true);

	static auto cubeM = CubeMesh();
	static auto sphereM = SphereMesh();

	GameobjectCreateParams cubeParams({ ComponentBitIndex::Transform, ComponentBitIndex::Render });
	cubeParams.meshId = cubeM->meshId;
	GameobjectCreateParams sphereParams({ ComponentBitIndex::Transform, ComponentBitIndex::Render });
	sphereParams.meshId = sphereM->meshId;

	GraphicsEngine::Get().GetWindow().inputDown->Connect([cubeParams, sphereParams](InputObject io) {
		if (io.input == InputObject::One) {
			if (cubes.size() > 0) {
				cubes.pop_back();
			}
		}
		else if (io.input == InputObject::Two) {
			cubes.push_back(GameObject::New(cubeParams));
			cubes.back()->Destroy();
			cubes.back()->RawGet<TransformComponent>()->SetPos(glm::dvec3{ cubes.size() * 3, 4, -1 });
			cubes.back()->RawGet<RenderComponent>()->SetColor(glm::dvec4{ 1, 1, 1, 1 });
		}
		else if (io.input == InputObject::Three) {
			if (spheres.size() > 0) {
				spheres.pop_back();
			}
		}
		else if (io.input == InputObject::Four) {
			spheres.push_back(GameObject::New(sphereParams));
			spheres.back()->Destroy();
			spheres.back()->RawGet<TransformComponent>()->SetPos(glm::dvec3{ spheres.size() * 3, 4, 4 });
			spheres.back()->RawGet<RenderComponent>()->SetColor(glm::dvec4{ 1, 1, 1, 1 });
		}
		else if (io.input == InputObject::Five) {
			for (int i = 0; i < 15; i++) {
				cubes.push_back(GameObject::New(cubeParams));
				cubes.back()->Destroy();
				cubes.back()->RawGet<TransformComponent>()->SetPos(glm::dvec3{ cubes.size() * 3, 4, -1 });
				cubes.back()->RawGet<RenderComponent>()->SetColor(glm::dvec4{ 1, 1, 1, 1 });
			}
		}
		else if (io.input == InputObject::Six) {
			for (int i = 0; i < 15; i++) {
				spheres.push_back(GameObject::New(sphereParams));
				spheres.back()->Destroy();
				spheres.back()->RawGet<TransformComponent>()->SetPos(glm::dvec3{ spheres.size() * 3, 4, 4 });
				spheres.back()->RawGet<RenderComponent>()->SetColor(glm::dvec4{ 1, 1, 1, 1 });
			}
		}
	});
}