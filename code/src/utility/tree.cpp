#include "tree.hpp"

template <unsigned int NodeChildrenCount>
Tree<NodeChildrenCount>::Tree() {
	nodes.emplace_back();
}

template <unsigned int NodeChildrenCount>
void Tree<NodeChildrenCount>::ForEach(int nodeDepth, std::function<void(Node&)> func)
{
	VisitChildren(rootNodeIndex, nodeDepth, func);
}

template <unsigned int NodeChildrenCount>
void Tree<NodeChildrenCount>::ForEach(std::function<void(Node&)> func)
{
	VisitChildren(rootNodeIndex, func);
}

template <unsigned int NodeChildrenCount>
void Tree<NodeChildrenCount>::Split(Node& node) {
	if (firstFreeIndex == -1) {
		firstFreeIndex = nodes.size();

		for (unsigned int i = 0; i < NodeChildrenCount; i++) {
			nodes.emplace_back();
		}
	}

	node.childIndex = firstFreeIndex;
	firstFreeIndex = nodes[firstFreeIndex].nextFree;
	for (unsigned int i = 0; i < NodeChildrenCount; i++) {
		nodes[node.childIndex + i].node.childIndex = -1;
		nodes[node.childIndex + i].node.data = -1;
	}
}

template <unsigned int NodeChildrenCount>
void Tree<NodeChildrenCount>::Collapse(Node& node) {
	for (unsigned int i = 0; i < NodeChildrenCount; i++) {
		nodes[node.childIndex + i].nextFree = firstFreeIndex;
	}
	
	firstFreeIndex = node.childIndex;
}

template <unsigned int NodeChildrenCount>
void Tree<NodeChildrenCount>::VisitChildren(int nodeIndex, std::function<void(Node&)> func)
{
	Node& node = nodes[nodeIndex].node;
	func(node);

	if (node.childIndex != -1) {
		for (unsigned int i = 0; i < NodeChildrenCount; i++) {
			VisitChildren(node.childIndex + i, func);
		}
	}
}

template <unsigned int NodeChildrenCount>
void Tree<NodeChildrenCount>::VisitChildren(int nodeIndex, int nodeDepth, std::function<void(Node&)> func)
{
	Node& node = nodes[nodeIndex].node;
	func(node);

	if (node.childIndex != -1) {
		for (unsigned int i = 0; i < NodeChildrenCount; i++) {
			VisitChildren(node.childIndex + i, nodeDepth + 1, func);
		}
	}
}

template<unsigned int NodeChildrenCount>
inline Tree<NodeChildrenCount>::MaybeNode::MaybeNode()
{
	node = { .childIndex = -1, .data = -1 };
}

// explicit template instantiations
template class Tree<2>;
template class Tree<4>;
template class Tree<8>;
template class Tree<27>;
template class Tree<256>;