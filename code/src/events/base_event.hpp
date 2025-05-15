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

	// Calls all the functions connected to the events in the queue and empties the queue. Disregard depth, implementation detail for recursion.
	// Do NOT call within an event handler
	static void FlushEventQueue(int depth = 0);

	// neccesary because functional objects connected to events could store things that need singletons to be destructed, yet singletons store events.
	// Disconnects all events.
	static void Cleanup();

private:
	friend class std::shared_ptr<BaseEvent>;

	// helper for Cleanup()
	virtual void CleanupConnections() = 0;
	static inline std::vector<BaseEvent*> events = {};
};

