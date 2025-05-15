#include "base_event.hpp"
#include <debug/log.hpp>
#include <debug/assert.hpp>

int depthShouldBe = 0;

void BaseEvent::FlushEventQueue(int depth) {
	Assert(depth >= depthShouldBe); // checks if this function was called by a function connected to an event (would recurse infinitely, bad)
	Assert(depth <= 8192); // We don't let events fire more events endlessly.
	depthShouldBe++;

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
	auto& q = EventInvocationQueue();
	//DebugLogInfo("Handling ", q.size());
	//if (q.size() > 1) {
		//std::cout << "";
	//}
	auto priorSize = q.size();
	for (auto& invoc : q) {
		invoc->RunConnections();
	}
	//q.clear();
	q.erase(q.begin(), q.begin() + priorSize); // we can't just clear the queue since invoking events could fire more events.
	// If the fired events we just handled fired more events, we should handle those immediately.
	if (!q.empty()) {
		FlushEventQueue(depth + 1);

	}
	else {
		depthShouldBe = 0;
	}
}

void BaseEvent::Cleanup() {
	while (!events.empty()) {
		events.back()->CleanupConnections();
		events.pop_back();
	}
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
	events.push_back(this);
}

BaseEvent::~BaseEvent() { 
	// might not be in events if Cleanup() has already been called
	for (auto it = events.begin(); it != events.end(); it++) {
		if (*it == this) {
			events.erase(it);
			return;
		}
	}
}