#include "base_event.hpp"
#include <debug/log.hpp>



void BaseEvent::FlushEventQueue() {
	auto& q = EventQueue();
	for (auto event : q) {
		//DebugLogInfo("Flushing ", event);
		event->Flush();
	}
}

std::vector<BaseEvent*>& BaseEvent::EventQueue()
{
	static std::vector<BaseEvent*> eventQueue;
	return eventQueue;
}
;

BaseEvent::BaseEvent() {
	EventQueue().push_back(this);
}

BaseEvent::~BaseEvent() { // todo: unoptimized for frequent removal 
	//if (!inQueue) { return; }
	BaseEvent* ptr = this;
	for (unsigned int i = 0; i < EventQueue().size(); i++) {
		if (EventQueue()[i] = ptr) {
			EventQueue()[i] = EventQueue().back();
			EventQueue().pop_back();
			return;
		}
	}
}