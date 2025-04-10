#include "pch.h"
#include "CppUnitTest.h"
//#include "../code/src/events/event.hpp"
#include "events/event.hpp"
#include "physics/spatial_acceleration_structure.hpp"
#include <gameobjects/gameobject.hpp>

#include <filesystem>
#include <tests/gameobject_tests.hpp>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TG3
{
	TEST_CLASS(TG3)
	{
	public:
		
		TEST_METHOD(TestEvents)
		{
			std::vector<int> expected = { 3, 4, 1, 2};
			std::vector<int> actual = {};
			auto f1 = [&actual]() mutable {
				actual.push_back(1);
			};

			auto f2 = [&actual]() mutable {
				actual.push_back(2);
			};

			auto f3 = [&actual]() mutable {
				actual.push_back(3);
			};

			auto f4 = [&actual]() mutable {
				actual.push_back(4);
			};

			auto eA = Event<>::New();
			auto eB = Event<>::New();

			eA->Connect(f1);
			eA->Connect(f2);
			eB->Connect(f3);
			eB->Connect(f4);

			eB->Fire();
			eA->Fire();
			
			BaseEvent::FlushEventQueue();

			Assert::IsTrue(expected == actual);
		}

		TEST_METHOD(TestEventsWithTemporaryConnections)
		{
			std::vector<int> expected = { 3, 4, 1, 2, 1, 3, 3, 4, 1, 2, 1, 3, 3, 4, 1, 2, 1, 3, 3, 4, 1, 2, 1, 3, 3, 4, 1, 2, 1, 3, 3, 4, 1, 2, 1, 3, 3, 4, 1, 2, 1, 3, 3, 4, 1, 2, 1, 3 };
			std::vector<int> actual = {};
			auto f1 = [&actual]() mutable {
				actual.push_back(1);
				};

			auto f2 = [&actual]() mutable {
				actual.push_back(2);
				};

			auto f3 = [&actual]() mutable {
				actual.push_back(3);
				};

			auto f4 = [&actual]() mutable {
				actual.push_back(4);
			};

			auto f5 = [&actual]() mutable {
				actual.push_back(5);
			};

			auto eA = Event<>::New();
			auto eB = Event<>::New();

			for (int i = 0; i < 8; i++) {
				auto c1 = eA->ConnectTemporary(f1);
				auto c2 = eA->ConnectTemporary(f2);
				auto c3 = eB->ConnectTemporary(f3);
				auto c4 = eB->ConnectTemporary(f4);
				eA->ConnectTemporary(f5);

				eB->Fire();
				eA->Fire();

				BaseEvent::FlushEventQueue();

				c2 = nullptr;
				c4 = nullptr;

				eA->Fire();
				eB->Fire();

				BaseEvent::FlushEventQueue();

			}
			if (expected != actual) {
				for (auto i : actual) {
					DebugLogInfo(i)
				}
				Assert::IsTrue(expected == actual);
			}
			
		}

		TEST_METHOD(TestSAS1) {
			try {
				std::filesystem::current_path(std::filesystem::current_path().concat("\\..\\.."));
			}
			catch (std::exception& e) {
				DebugLogInfo(e.what());
				abort();
			}
			auto& SAS = SpatialAccelerationStructure::Get();

			std::array<std::array<std::array<std::shared_ptr<GameObject>, 8>, 8>, 8> arr;
			auto mesh = CubeMesh();

			for (int x = 0; x < 8; x++)
				for (int y = 0; y < 8; y++)
					for (int z = 0; z < 8; z++) {
						auto params = GameobjectCreateParams({ ComponentBitIndex::Transform, ComponentBitIndex::Collider });
						params.meshId = mesh->meshId;
						auto g = GameObject::New(params);
						g->RawGet<TransformComponent>()->SetPos(glm::dvec3 { x * 4, y * 4, z * 4 });

						arr[x][y][z] = g;
						g->Destroy();
					}

			SAS.Update();
			Assert::IsTrue(SAS.Query(AABB({ 6, 6, 6 }, { 12, 12, 24 })).size() == 2*2*5);
			
			for (int x = 0; x < 8; x++)
				for (int y = 0; y < 8; y++)
					for (int z = 0; z < 8; z++)
						arr[x][y][z] = nullptr;

			SAS.Update();
			Assert::IsTrue(SAS.Query(AABB({ 6, 6, 6 }, { 12, 12, 24 })).size() == 0);
		}
	};
}
