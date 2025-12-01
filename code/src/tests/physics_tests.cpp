#include "gameobject_tests.hpp"
#include <graphics/gengine.hpp>
#include <conglomerates/basic_renderer.hpp>
#include "physics_test.hpp"
#include <physics/raycast.hpp>

void TestPhysics() {
	GraphicsEngine::Get().debugFreecamEnabled = true;

	BasicRenderer::Setup();
	TestSkybox();

	MakeFPSTracker();

	//TestGrassFloor()->RawGet<TransformComponent>()->SetPos({ 5, -0.5, 0 });

	TestStationaryPointlight();

	//TestBrickWall();
	
	TestPit();

	//TestCubeArray({ 1, 1, 1 }, { 0, 5, 0 }, { 1, 1, 1 }, true);
	TestCubeArray({ 2, 2, 2 }, { 0, 4, 0 }, { 4, 11, 1 }, true);

	//TestSphere(1, 2.95, 1, true);
		
	for (int x = 1; x < 5; x+= 2) {
		for (int y = 4; y < 18; y+=2) {
			for (int z = 1; z < 4; z+=2) {
				//TestSphere(x, y, z, true);
			}
		}
	}
}

void MousePhysicsGrab() {
	GraphicsEngine::Get().window.inputDown->Connect([](InputObject input) {
		if (input.input == InputObject::LMB) {
			auto lookVector = LookVector(glm::radians(GraphicsEngine::Get().debugFreecamPitch), glm::radians(GraphicsEngine::Get().debugFreecamYaw));
			auto castResult = Raycast(GraphicsEngine::Get().GetDebugFreecamCamera().position, lookVector);
			//DebugPlacePointOnPosition(GraphicsEngine::Get().GetDebugFreecamCamera().position + lookVector * 2.0);
			if (castResult.hitObject != nullptr) {
				//DebugPlacePointOnPosition(castResult.hitPoint);
				if (castResult.hitObject != nullptr && castResult.hitObject->MaybeRawGet<RigidbodyComponent>()) {
					//DebugLogInfo(castResult.hitNormal);
					castResult.hitObject->RawGet<RigidbodyComponent>()->velocity += castResult.hitNormal * 2.0;
					//castResult.hitObject->Get<TransformComponent>().SetPos(castResult.hitObject->Get<TransformComponent>().position + castResult.hitNormal * 0.02);
				}
			}

		}
	});
}
