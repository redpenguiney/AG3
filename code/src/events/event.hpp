#pragma once
#include <vector>
#include <functional>
#include "base_event.hpp"
#include "debug/log.hpp"

// Events of various kinds are triggered by various parts of the AG3 engine (input events, collision events, etc.).
// You can connect functions to those events to make code run when an event is fired, as well as fire custom events.
// Connected functions are not called immediately for performance reasons, but are instead stored in a queue then fired all at once.
template <typename ... eventArgs>
class Event: BaseEvent {
	public:

	using ConnectableFunctionArgs = std::tuple<eventArgs...>;
	using ConnectableFunction = std::function<void(eventArgs...)>;

	Event(): BaseEvent() {
		eventInvocations = std::make_shared< std::vector<ConnectableFunctionArgs>>();
		connectedFunctions = std::make_shared< std::vector<ConnectableFunction>>();
	}

	Event(const Event&) = delete;
	~Event() {}

	// Adds the event to the queue, meaning that next time FlushQueue() is called, all functions connected to this event will be called.
	void Fire(eventArgs ... args) {
		
		//if (!inQueue) {
			//DebugLogInfo("Firing ", this);
			//EventQueue().push_back(this);
			//inQueue = true;
		//}
		ConnectableFunctionArgs tupledArgs = std::make_tuple(args...);
		eventInvocations->push_back(tupledArgs); 
	}

	// Connects the given function to the event, so that it will be called every time the event is fired.
	// WARNING: if the function is a lambda which captures a shared_ptr, then that shared_ptr gets stored in this event.
	// TODO: disconnecting events
	void Connect(ConnectableFunction function) {
		connectedFunctions->push_back(function);
	}

	// returns true if anything is connected to this event.
	bool HasConnections() {
		return connectedFunctions->size() > 0;
	};

	private:

	// Internal helper for FlushEventQueue()
	inline void Flush() {
		// annoyingly, because a function can destroy the event which called it (FOR EXAMPLE (AHEM), if a gui DESTROYS itself when clicked), we have to create shared_ptrs in here
		std::shared_ptr<std::vector<ConnectableFunctionArgs>> eventInvocationsLock = eventInvocations;
		std::shared_ptr<std::vector<ConnectableFunction>> connectedFunctionsLock = connectedFunctions;

		for (auto& cfa : *eventInvocationsLock) {
			for (auto& f : *connectedFunctionsLock) {
				std::apply(f, cfa); // std::apply basically calls f using the tuple cfa as a variaidic for us.
			}
		}

		eventInvocationsLock->clear();
	}
	
	// shared_ptrs needed so that Flush() can keep working without segfaults, even if the event is destroyed by calling a connected function.
	std::shared_ptr<std::vector<ConnectableFunctionArgs>> eventInvocations;
	std::shared_ptr<std::vector<ConnectableFunction>> connectedFunctions;
	
};