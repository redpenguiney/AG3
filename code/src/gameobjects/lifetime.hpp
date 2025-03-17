#include "gameobject.hpp"
#include <memory>

// holds onto to the given shared_ptr until secondsToLive runs out, meaning it will destroyed after the given time interval if no other references to the object exist
template <typename T>
void NewObjectLifetime(std::shared_ptr<T>& object, double secondsToLive) {
	struct Lifetime {
		double timeLeft;
		std::unique_ptr<Event<float>::Connection> connection;
		std::shared_ptr<T> object;
	};
	std::shared_ptr<Lifetime> lifetime = std::make_shared<Lifetime>(secondsToLive, nullptr, object);
	auto c = GraphicsEngine::Get().preRenderEvent->ConnectTemporary([lifetime](float dt) {
		lifetime->timeLeft -= dt;
		if (lifetime->timeLeft <= 0) {
			lifetime->object = nullptr;
			lifetime->connection = nullptr;
		}
	});
	lifetime->connection = std::move(c);
}
