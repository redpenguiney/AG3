#pragma once
#include <unordered_map>
#include <list>
#include "graphics/window.hpp"

// Sometimes connecting to Window's or Gui's input events isn't enough. 
// This singleton models input connections as a stack.
// That allows stuff like (for example) scrolling zoomming in and out until you mouse over a GUI, at which point scrolling will scroll the GUI until you mouse out.
class InputStack {
public:
	using InputCallback = std::function<void(InputObject) >;

	static InputStack& Get();

	// Call PopBegin() with the same name to unbind. If an action with the same name is already on the stack, it silently removes it before pushing the new callback to the top of the stack.
	void PushBegin(InputObject::InputType, int name, InputCallback);
	// Call PopEnd() with the same name to unbind. If an action with the same name is already on the stack, it silently removes it before pushing the new callback to the top of the stack.
	void PushEnd(InputObject::InputType, int name, InputCallback);
	
	// Does nothing if name not in stack.
	void PopBegin(InputObject::InputType, int name);
	// Does nothing if name not in stack.
	void PopEnd(InputObject::InputType, int name);

private:
	struct Binding {
		int name;
		InputCallback callback;
	};

	std::unordered_map<InputObject::InputType, std::list<Binding>> beginStack;
	std::unordered_map<InputObject::InputType, std::list<Binding>> endStack;

	std::unique_ptr<Event<InputObject>::Connection> windowInputBeginConnection;
	std::unique_ptr<Event<InputObject>::Connection> windowInputEndConnection;

	InputStack();
	InputStack(const InputStack&) = delete;
};

