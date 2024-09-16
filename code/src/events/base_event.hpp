#pragma once
#include <vector>

// Abstract event class. See Event for actual info.
class BaseEvent {


protected:
	// Internal helper for FlushEventQueue()
	virtual void Flush() = 0;

	BaseEvent();
	virtual ~BaseEvent();

	static std::vector<BaseEvent*>& EventQueue();

	//bool inQueue = false;
public:

	// Calls all the functions connected to the events in the queue and empties the queue.
	static void FlushEventQueue();

private:
	
};

