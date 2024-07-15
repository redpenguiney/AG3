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
	for (unsigned int i = 0; i < _eventQueue.size(); i++) {
		if (_eventQueue[i] == ptr) {
			_eventQueue.erase(_eventQueue.begin() + i);
		}
	}
}