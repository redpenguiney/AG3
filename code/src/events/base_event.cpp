#include "base_event.hpp"

inline std::vector<BaseEvent*> _eventQueue;

void BaseEvent::FlushEventQueue() {
	for (auto& event : _eventQueue) {
		event->Flush();
	}
};

BaseEvent::BaseEvent() {
	_eventQueue.push_back(this);
}

BaseEvent::~BaseEvent() { // todo: unoptimized for frequent removal 
	BaseEvent* ptr = this;
	for (auto it = _eventQueue.begin(); it != _eventQueue.end(); it++) {
		if (*it == ptr) {
			it = _eventQueue.erase(it);
		}
	}
}