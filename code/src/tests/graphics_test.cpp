#include "graphics_test.hpp"
#include "gameobjects/gameobject.hpp"
#include "gameobject_tests.hpp"
#include <conglomerates/basic_renderer.hpp>
#include <conglomerates/highlight.hpp>


void TransparentHandler(Material*, std::shared_ptr<ShaderProgram>) {
	//BasicRenderer::Setup().mainFramebuffer->Bind({ 1, 2 });
}

// I will find the graphics bugs. Once. And. For all.
void TestGraphics() {

	BasicRenderer::Setup();
	TestSkybox();

	TestGrassFloor();
	TestBrickWall();

	TestAnimation();

	//TestUi();

	//TestCubeArray({ 3, 4, 3 }, { 1, 1, 1 }, { 10, 10, 10 }, false);

	//TestStationaryPointlight();

	static std::vector<std::shared_ptr<GameObject>> cubes = {};
	static std::vector<std::shared_ptr<GameObject>> spheres = {};
	static std::vector<std::shared_ptr<GameObject>> unique_cubes = {};

	GraphicsEngine::Get().SetDebugFreecamEnabled(true);
	GraphicsEngine::Get().GetDebugFreecamCamera().position += glm::dvec3 {0, 0, 15};
	//GraphicsEngine::Get().SetWireframeEnabled(true);

	static auto cubeM = CubeMesh();
	static auto sphereM = SphereMesh();

	auto transparentMaterial = BasicRenderer::GetDefaultTransparentMaterial();

	GameobjectCreateParams cubeParams({ ComponentBitIndex::Transform, ComponentBitIndex::Render });
	cubeParams.meshId = cubeM->meshId;
	GameobjectCreateParams sphereParams({ ComponentBitIndex::Transform, ComponentBitIndex::Render });
	sphereParams.meshId = sphereM->meshId;
	GameobjectCreateParams transparentCubeParams({ ComponentBitIndex::Transform, ComponentBitIndex::Render });
	transparentCubeParams.meshId = cubeM->meshId;
	transparentCubeParams.materialId = transparentMaterial->id;
	//transparentCubeParams.materialId = GraphicsEngine::Get().defa

	for (int i = 0; i < 3; i++) {
		auto g = GameObject::New(transparentCubeParams);
		g->RawGet<TransformComponent>()->SetPos(glm::dvec3{ -4, 2, -4 + 3 * i });
		glm::vec4 color{ 0, 0, 0, 0.5 };
		color[i] = 1;
		g->RawGet<RenderComponent>()->SetColor(color);
		g->RawGet<RenderComponent>()->SetTextureZ(-1);

		HighlightHandler::Get().AddHighlight(g, 5, { 1, 1, 1 });
	}

	for (int i = 0; i < 10; i++) {
		auto g = GameObject::New(transparentCubeParams);
		g->RawGet<TransformComponent>()->SetPos(glm::dvec3{ -7, 2, -4 + 3 * i });
		glm::vec4 color{ 0.5, 0.5, 0.5, i/10.0 };
		g->RawGet<RenderComponent>()->SetColor(color);
		g->RawGet<RenderComponent>()->SetTextureZ(-1);
	}

	auto highlightSphere = GameObject::New(sphereParams);
	highlightSphere->RawGet<TransformComponent>()->SetPos({-3, 1, -4 });
	highlightSphere->RawGet<RenderComponent>()->SetColor(glm::dvec4{ 1, 1, 1, 1 });
	HighlightHandler::Get().AddHighlight(highlightSphere, 5, { 1, 1, 1 });

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
		else if (io.input == InputObject::Seven) {
			auto makeMparams = MeshCreateParams{ .textureZ = -1.0, .opacity = 1, .expectedCount = 16384 };
			auto m = Mesh::MultiFromFile("../models/rainbowcube.obj", makeMparams).at(0).mesh;

			GameobjectCreateParams uniqueCubeParams({ ComponentBitIndex::Transform, ComponentBitIndex::Render });
			uniqueCubeParams.meshId = m->meshId;
			unique_cubes.push_back(GameObject::New(uniqueCubeParams));
			unique_cubes.back()->Destroy();
			unique_cubes.back()->RawGet<TransformComponent>()->SetPos(glm::dvec3{ unique_cubes.size() * 3, -4, -1 });
			unique_cubes.back()->RawGet<RenderComponent>()->SetColor(glm::dvec4{ 0, 1, 1, 1 });
		}
		else if (io.input == InputObject::Eight) {
			if (unique_cubes.size() > 0) {
				unique_cubes.pop_back();
			}
		}
	});
}