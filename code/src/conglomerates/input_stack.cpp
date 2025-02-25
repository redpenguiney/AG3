#include "input_stack.hpp"
#include "graphics/gengine.hpp"

InputStack& InputStack::Get()
{
	static InputStack i;
	return i;
}

void InputStack::PushBegin(InputObject::InputType input, int name, InputCallback callback)
{
	for (auto it = beginStack[input].begin(); it != beginStack[input].end(); it++) {
		if (it->name == name) {
			beginStack[input].erase(it);
			break;
		}
	}

	auto& ref = beginStack[input].emplace_back(name, callback);
}

void InputStack::PushEnd(InputObject::InputType input, int name, InputCallback callback)
{
	for (auto it = endStack[input].begin(); it != endStack[input].end(); it++) {
		if (it->name == name) {
			endStack[input].erase(it);
			break;
		}
	}

	auto& ref = endStack[input].emplace_back(name, callback);
}

void InputStack::PopBegin(InputObject::InputType input, int name)
{
	for (auto it = beginStack[input].begin(); it != beginStack[input].end(); it++)
		if (it->name == name) {
			beginStack[input].erase(it);
			return;
		}
	//Assert(false); // INVALID NAME
}

void InputStack::PopEnd(InputObject::InputType input, int name)
{
	for (auto it = endStack[input].begin(); it != endStack[input].end(); it++)
		if (it->name == name) {
			endStack[input].erase(it);
			return;
		}
	//Assert(false); // INVALID NAME
}

InputStack::InputStack() {
	windowInputBeginConnection = GraphicsEngine::Get().window.inputDown->ConnectTemporary([this](InputObject i) {
		if (!beginStack[i.input].empty())
			beginStack[i.input].back().callback(i);
	});
	windowInputEndConnection = GraphicsEngine::Get().window.inputUp->ConnectTemporary([this](InputObject i) {
		if (!endStack[i.input].empty())
			endStack[i.input].back().callback(i);
	});
}