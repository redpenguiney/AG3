#pragma once
#include <vector>

// Abstract event class. See Event for actual info.
class BaseEvent {
	// Internal helper for FlushEventQueue()
	virtual void Flush() = 0;

	protected:
	BaseEvent();
	~BaseEvent();

	public:

	// Calls all the functions connected to the events in the queue and empties the queue.
	static void FlushEventQueue();
};



