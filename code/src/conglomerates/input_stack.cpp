#include "input_stack.hpp"
#include "graphics/gengine.hpp"

InputStack& InputStack::Get()
{
	static InputStack i;
	return i;
}

InputStack::StackId InputStack::PushBegin(InputObject::InputType input, InputCallback callback)
{
	auto& ref = beginStack[input].emplace_back(callback);
	return &ref;
}

InputStack::StackId InputStack::PushEnd(InputObject::InputType input, InputCallback callback)
{
	auto& ref = endStack[input].emplace_back(callback);
	return &ref;
}

void InputStack::PopBegin(InputObject::InputType input, StackId id)
{
	for (auto it = beginStack[input].begin(); it != beginStack[input].end(); it++)
		if (&*it == id) {
			beginStack[input].erase(it);
			return;
		}
	Assert(false); // INVALID STACK ID
}

void InputStack::PopEnd(InputObject::InputType input, StackId id)
{
	for (auto it = endStack[input].begin(); it != endStack[input].end(); it++)
		if (&*it == id) {
			endStack[input].erase(it);
			return;
		}
	Assert(false); // INVALID STACK ID
}

InputStack::InputStack() {
	windowInputBeginConnection = GraphicsEngine::Get().window.inputDown->ConnectTemporary([this](InputObject i) {
		if (!beginStack[i.input].empty())
			beginStack[i.input].back()(i);
	});
	windowInputEndConnection = GraphicsEngine::Get().window.inputUp->ConnectTemporary([this](InputObject i) {
		if (!endStack[i.input].empty())
			endStack[i.input].back()(i);
	});
}