#include "base_event.hpp"
#include <debug/log.hpp>



void BaseEvent::FlushEventQueue() {
	//auto& events = EventList();

	////
	//for (unsigned int i = 0; i < events.size(); i++) { 
	//	auto& event = events[i];
	//	if (event.expired()) {
	//		events[i] = events.back();
	//		events.pop_back();
	//		i--;
	//	}
	//}
	auto &q = EventInvocationQueue();
	DebugLogInfo("Handling ", q.size());
	if (q.size() > 1) {
		std::cout << "";
	}
	for (auto& invoc : q) {
		invoc->RunConnections();
	}
	EventInvocationQueue().clear();
}

//std::vector<std::weak_ptr<BaseEvent>>& BaseEvent::EventList()
//{
//	static std::vector<std::weak_ptr<BaseEvent >> events;
//	return events;
//}
std::vector<std::unique_ptr<BaseEvent::BaseEventInvocation>>& BaseEvent::EventInvocationQueue()
{
	static std::vector<std::unique_ptr<BaseEventInvocation>> invocations;
	return invocations;
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