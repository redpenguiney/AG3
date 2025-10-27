#include "debug/log.hpp"
#include <conglomerates/basic_renderer.hpp>
#include <graphics/gengine.hpp>
#include <network/network.hpp>

//#include "WinSock2.h"
#include "boost/asio.hpp"
#include <tests/graphics_test.hpp>
#include <tests/gameobject_tests.hpp>

#include "gameobjects/gameobject.hpp"
#include <physics/raycast.hpp>

void GameInit(std::vector<std::string> args) {
	DebugLogInfo("SANDBOXING? MORE LIKE SANDBAGGING!");

	bool is_server = true;
	bool headless = false;
	for (auto& arg : args) {
		if (arg == "--headless")
			headless = true;
		else if (arg == "--client")
			is_server = false;
	}

	if (!headless) {
		BasicRenderer::Setup();
		TestStationaryPointlight();
		GraphicsEngine::Get().SetDebugFreecamEnabled(true);
		GraphicsEngine::Get().GetDebugFreecamCamera().position = { 0, 5, 10 };
	}

	//is_server = !is_server;

	auto testObject = TestGrassFloor();

	if (is_server) {
		TestSkybox();

		NetworkingEngine::Get().Host();

		NetworkingEngine::Get().onUserdataRecieved->Connect([](NetworkUserdata userdata) {
			std::string messagestr((const char*)userdata.data.data(), userdata.data.size());
			DebugLogInfo("RECIEVED FROM CLIENT (", userdata.reliable, " ", userdata.data.size(), "): first ", (int)messagestr[0], " "/*, messagestr*/);

		});

		//NetworkingEngine::Get().SyncObjectTransform(testObject, 1);
		//NetworkingEngine::Get().onNewClient->Connect()
		//TestGraphics();

		/*Socket* server = new Socket(49000);


		GraphicsEngine::Get().preRenderEvent->Connect([server](float dt) {
			auto packets = server->Recieve();
			DebugLogInfo("GOT ", packets.size());
		});		*/	

		//GraphicsEngine::Get().preRenderEvent->Connect([testObject](float dt) {
			//double x = sin(GraphicsEngine::Get().shaderTime / 10) * 100;
			//testObject->RawGet<TransformComponent>()->SetPos({x , 0, 0 });
		//});

		TestCubeArray({3, 3, 3 }, { 0, 8, 0 }, { 2, 2, 2 }, true, { 1, 1, 1 }, true);

		//GraphicsEngine::Get().window.inputDown->Connect([](InputObject input) {
	 //   if (input.input == InputObject::LMB) {
		//	auto lookVector = LookVector(glm::radians(GraphicsEngine::Get().debugFreecamPitch), glm::radians(GraphicsEngine::Get().debugFreecamYaw));
	 //       auto castResult = Raycast(GraphicsEngine::Get().GetDebugFreecamCamera().position, lookVector);
		//	//DebugPlacePointOnPosition(GraphicsEngine::Get().GetDebugFreecamCamera().position + lookVector * 2.0);
	 //       if (castResult.hitObject != nullptr) {
		//		//DebugPlacePointOnPosition(castResult.hitPoint);
	 //           if (castResult.hitObject != nullptr && castResult.hitObject->MaybeRawGet<RigidbodyComponent>()) {
		//			//DebugLogInfo(castResult.hitNormal);
	 //               castResult.hitObject->RawGet<RigidbodyComponent>()->velocity += castResult.hitNormal * 2.0;
	 //               //castResult.hitObject->Get<TransformComponent>().SetPos(castResult.hitObject->Get<TransformComponent>().position + castResult.hitNormal * 0.02);
	 //           }
	 //       }

	 //   }
	 //   });
	}
	else {
		NetworkingEngine::Get().Connect("127.0.0.1");
		//NetworkingEngine::Get().Connect("162.226.172.240");

		NetworkingEngine::Get().onConnectionAttemptComplete->Connect([testObject](NetworkingEngine::ConnectionAttemptResult result) {
			Assert(result.successful);

			//NetworkingEngine::Get().SyncObjectTransform(testObject, 1);

			//char str[] = "POOPLESNOOT";

			//char str[] = ""
			//std::ifstream file("../resources/bee_move_script.txt");
			//std::stringstream buffer;
			//buffer << file.rdbuf();
			//std::string str = buffer.str();
			//Assert(str.size() > 0);
			//NetworkingEngine::Get().SendDataReliable(str.data(), str.size());
			//DebugLogInfo("SENT ", str.size());

			TestCubeArray({ 3, 3, 3 }, { 0, 8, 0 }, { 2, 2, 2 }, true, { 1, 1, 1 }, true);

		});

		/*Socket* client = new Socket("127.0.0.1", 49000, 49001);
		GraphicsEngine::Get().preRenderEvent->Connect([client](float dt) {
			char data[3] = "HI";
			client->Send(data, 3);
		});*/


	}
	

}

void GameClose() {}