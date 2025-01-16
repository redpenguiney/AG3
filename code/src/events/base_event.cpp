#include "base_event.hpp"
#include <debug/log.hpp>



void BaseEvent::FlushEventQueue() {
	//return;
	// we have to copy the queue because Flush() can result in the destruction of an event thus invalidating the iterator if we held onto the reference
	//auto q = EventQueue();
	auto& q = EventQueue();

	for (unsigned int i = 0; i < q.size(); i++) { 
		auto& event = q[i];
		if (event.expired()) {
			q[i] = q.back();
			q.pop_back();
			i--;
		}
		else {
			//DebugLogInfo("Flushing ", even);
			event.lock()->Flush();
		}
		
	}
}

std::vector<std::weak_ptr<BaseEvent>>& BaseEvent::EventQueue()
{
	static std::vector<std::weak_ptr<BaseEvent >> eventQueue;
	return eventQueue;
}
;

BaseEvent::BaseEvent() {
	// CANNOT do this here because a shared_ptr hasn't been made yet.
	//EventQueue().push_back(enable_shared_from_this<BaseEvent>::weak_from_this(this));
}

BaseEvent::~BaseEvent() { // todo: unoptimized for frequent removal 
	//if (!inQueue) { return; }
	/*BaseEvent* ptr = this;
	for (unsigned int i = 0; i < EventQueue().size(); i++) {
		if (EventQueue()[i] = ptr) {
			EventQueue()[i] = EventQueue().back();
			EventQueue().pop_back();
			return;
		}
	}*/
}