#pragma once
#include <vector>
#include <functional>
#include "base_event.hpp"
#include "debug/log.hpp"

// Events of various kinds are triggered by various parts of the AG3 engine (input events, collision events, etc.).
// You can connect functions to those events to make code run when an event is fired, as well as fire custom events.
// Connected functions are not called immediately for performance reasons, but are stored in a queue then fired all at once.
template <typename ... eventArgs>
class Event: BaseEvent {
	public:

	using ConnectableFunctionArgs = std::tuple<eventArgs...>;
	using ConnectableFunction = std::function<void(eventArgs...)>;

	Event(): BaseEvent() {}

	Event(const Event&) = delete;
	~Event() {}

	// Adds the event to the queue, meaning that next time FlushQueue() is called, all functions connected to this event will be called.
	inline void Fire(eventArgs ... args) {
		ConnectableFunctionArgs tupledArgs = std::make_tuple(args...);
		eventInvocations.push_back(tupledArgs); 
	}

	// Connects the given function to the event, so that it will be called every time the event is fired.
	// WARNING: if the function is a lambda which captures a shared_ptr, then that shared_ptr gets stored in this event.
	// TODO: disconnecting events
	// TODO: might not work with templated functions? do we care?
	void Connect(ConnectableFunction function) {
		connectedFunctions.push_back(function);
	}

	// returns true if anything is connected to this event.
	bool HasConnections() {
		return connectedFunctions.size() > 0;
	};

	private:

	// Internal helper for FlushEventQueue()
	inline void Flush() {
		for (auto& cfa : eventInvocations) {
			for (auto& f : connectedFunctions) {
				std::apply(f, cfa); // std::apply basically calls f using the tuple cfa as a variaidic for us.
			}
		}
		
		eventInvocations.clear();
	}
	

	std::vector<ConnectableFunctionArgs> eventInvocations;
	std::vector<ConnectableFunction> connectedFunctions;
	
};