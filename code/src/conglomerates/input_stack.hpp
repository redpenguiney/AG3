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
	using StackId = void*;

	const InputStack& Get();

	// returned StackId is used to remove the input when you're done.
	StackId PushBegin(InputObject::InputType, InputCallback);
	// returned StackId is used to remove the input when you're done.
	StackId PushEnd(InputObject::InputType, InputCallback);
	
	void PopBegin(InputObject::InputType, StackId);
	void PopEnd(InputObject::InputType, StackId);

private:
	std::unordered_map<InputObject::InputType, std::list<InputCallback>> beginStack;
	std::unordered_map<InputObject::InputType, std::list<InputCallback>> endStack;

	std::unique_ptr<Event<InputObject>::Connection> windowInputBeginConnection;
	std::unique_ptr<Event<InputObject>::Connection> windowInputEndConnection;

	InputStack();
};

