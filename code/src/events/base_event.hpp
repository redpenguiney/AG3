#pragma once
#include <vector>
#include <memory>

// Abstract event class. See Event for actual info.
class BaseEvent: public std::enable_shared_from_this<BaseEvent> {
protected:
	class BaseEventInvocation {
	public:
		virtual void RunConnections() = 0;
		virtual ~BaseEventInvocation() = default;
	};

	// Internal helper for FlushEventQueue()
	//virtual void Flush() = 0;

	BaseEvent();
	virtual ~BaseEvent();

	//static std::vector<std::weak_ptr<BaseEvent>>& EventList();
	static std::vector<std::unique_ptr<BaseEventInvocation>>& EventInvocationQueue();
	

	//bool inQueue = false;
public:

	// Calls all the functions connected to the events in the queue and empties the queue.
	static void FlushEventQueue();

private:
	friend class std::shared_ptr<BaseEvent>;
};

