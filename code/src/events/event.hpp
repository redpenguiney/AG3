#pragma once
#include <vector>
#include <functional>
#include "base_event.hpp"
#include "debug/log.hpp"
#include <memory>

// Events of various kinds are triggered by various parts of the AG3 engine (input events, collision events, etc.).
// You can connect functions to those events to make code run when an event is fired, as well as fire custom events.
// Connected functions are not called immediately for performance reasons, but are instead stored in a queue then fired all at once.
template <typename ... eventArgs>
class Event: public BaseEvent {
public:
	friend class Connection;

	// Represents a function's connection to a specific event. 
	// When this goes out of scope, it disconnects the function from the event.
	class Connection {
	public:
		Connection(unsigned int id, std::weak_ptr<Event> even): 
			connectedFunctionId(id), event(even) 
		{
		
		}

		~Connection() {
			if (!event.expired()) {
				auto lockedEvent = event.lock();
				for (auto it = lockedEvent->connectedFunctions->begin(); it != lockedEvent->connectedFunctions->end(); it++) {
					if (it->first == connectedFunctionId) {
						lockedEvent->connectedFunctions->erase(it);
						return;
					}
				}
			}
		}
	private:
		unsigned int connectedFunctionId;
		std::weak_ptr<Event> event;
	};

	using ConnectableFunctionArgs = std::tuple<eventArgs...>;
	using ConnectableFunction = std::function<void(eventArgs...)>;

private:
	class EventInvocation : public BaseEventInvocation {
	public:
		ConnectableFunctionArgs invocationArgs;
		std::weak_ptr<Event> event;

		EventInvocation(ConnectableFunctionArgs args, decltype(event) e): invocationArgs(args), event(e) {}
		~EventInvocation() = default;

		virtual void RunConnections() override {
			if (event.expired()) return;

			auto eventLock = event.lock();
			std::shared_ptr<std::vector<std::pair<unsigned int, ConnectableFunction>>> connectedFunctionsLock = eventLock->connectedFunctions;

			//for (auto& cfa : *eventInvocationsLock) {
			for (auto& f : *connectedFunctionsLock) {
				std::apply(f.second, invocationArgs); // std::apply basically calls f using the tuple cfa as a variaidic for us.
			}
			//}

			//eventInvocationsLock->clear();
		}
	};
public:

	// sadly, we have to use shared_ptr for events to handle the situation where firing an event results in the destruction of that fired event.
	// this means no default constructing event either :(
	static std::shared_ptr<Event> New() {
		auto ptr = std::shared_ptr< Event>(new Event());
		//EventList().push_back(std::weak_ptr<BaseEvent>(std::static_pointer_cast<BaseEvent>(ptr)));
		return ptr;
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

		//if (typeid(decltype(*this)).hash_code() != typeid(Event<>).hash_code() )
			//DebugLogInfo("Adding ", typeid(decltype(this)).name(), "to queue");

		ConnectableFunctionArgs tupledArgs = std::make_tuple(args...);
		//eventInvocations->push_back(tupledArgs); 
		std::weak_ptr<Event> e = dynamic_pointer_cast<Event>(weak_from_this().lock());
		std::unique_ptr<BaseEventInvocation> p = std::unique_ptr<BaseEventInvocation>((BaseEventInvocation*)(new EventInvocation(tupledArgs, e)));
		auto& q = EventInvocationQueue();
		q.push_back(std::move(p));
	}

	// Connects the given function to the event, so that it will be called every time the event is fired.
	// WARNING: if the function is a lambda which captures a shared_ptr, then that shared_ptr gets stored in this event.
	// This connection is permanent and lasts until the event is destroyed. For a temporary connection use ConnectTemporary().
	void Connect(ConnectableFunction function) {
		connectedFunctions->push_back(std::make_pair(LAST_CONNECTION_ID++, function));
	}

	// Connects the given function to the event, so that it will be called every time the event is fired, until the destruction of the returned Connection object.
	std::unique_ptr<Connection> ConnectTemporary(ConnectableFunction function) {
		unsigned int id = LAST_CONNECTION_ID++;
		connectedFunctions->push_back(std::make_pair(id, function));
		std::weak_ptr<Event> wthis = std::dynamic_pointer_cast<Event>(shared_from_this()); // sadly we have to use shared_ptr, cast it, then turn it to a weak_ptr because no dynamic pointer cast for weak_ptr. stupid.
		return std::move(std::unique_ptr<Connection>(new Connection(id, wthis)));
	}

	// returns true if anything is connected to this event.
	bool HasConnections() {
		return connectedFunctions->size() > 0;
	};

private:
	static inline unsigned int LAST_CONNECTION_ID = 0;

	// private to enforce use of factory constructor
	Event() : BaseEvent() {
		//eventInvocations = std::make_shared< std::vector<ConnectableFunctionArgs>>();
		connectedFunctions = std::make_shared< std::vector<std::pair<unsigned int, ConnectableFunction>>>();
	}

	private:
	//template <typename T>
	friend class std::shared_ptr<Event>;

	// Internal helper for FlushEventQueue()
	//inline void Flush() {
		// annoyingly, because a function can destroy the event which called it (FOR EXAMPLE (AHEM), if a gui DESTROYS itself when clicked), we have to create shared_ptrs in here
		//std::shared_ptr<std::vector<ConnectableFunctionArgs>> eventInvocationsLock = eventInvocations;
		//std::shared_ptr<std::vector<std::pair<unsigned int ,ConnectableFunction>>> connectedFunctionsLock = connectedFunctions;

		//for (auto& cfa : *eventInvocationsLock) {
		//	for (auto& f : *connectedFunctionsLock) {
		//		std::apply(f.second, cfa); // std::apply basically calls f using the tuple cfa as a variaidic for us.
		//	}
		//}

		//eventInvocationsLock->clear();
	//}
	
	// shared_ptrs needed so that Flush() can keep working without segfaults, even if the event is destroyed by calling a connected function.
	//std::shared_ptr<std::vector<ConnectableFunctionArgs>> eventInvocations;
	friend class EventInvocation;

	// unsigned int is ID of the function, because std::function can't be compared and we need to be able to differentiate between different functions to disconnect them.
	std::shared_ptr<std::vector<std::pair<unsigned int, ConnectableFunction>>> connectedFunctions; 
	
};