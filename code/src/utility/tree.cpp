#include "tree.hpp"

template <unsigned int NodeChildrenCount>
Tree<NodeChildrenCount>::Tree() {
	nodes.emplace_back();
}

template <unsigned int NodeChildrenCount>
void Tree<NodeChildrenCount>::ForEach(int nodeDepth, std::function<void(int)> func)
{
	VisitChildren(rootNodeIndex, 0, nodeDepth, func);
}

template <unsigned int NodeChildrenCount>
void Tree<NodeChildrenCount>::ForEach(std::function<void(int, int)> func)
{

	VisitChildren(rootNodeIndex, 0, func);
}

template <unsigned int NodeChildrenCount>
void Tree<NodeChildrenCount>::Split(int nodeIndex) {
	if (firstFreeIndex == -1) {
		firstFreeIndex = nodes.size();

		for (unsigned int i = 0; i < NodeChildrenCount; i++) {
			nodes.emplace_back();
		}
	}

	nodes[nodeIndex].node.childIndex = firstFreeIndex;
	firstFreeIndex = nodes[firstFreeIndex].nextFree;
	for (unsigned int i = 0; i < NodeChildrenCount; i++) {
		nodes[nodes[nodeIndex].node.childIndex + i].node.childIndex = -1;
		nodes[nodes[nodeIndex].node.childIndex + i].node.data = -1;
	}
}

template <unsigned int NodeChildrenCount>
void Tree<NodeChildrenCount>::Collapse(int nodeIndex) {
	for (unsigned int i = 0; i < NodeChildrenCount; i++) {
		nodes[nodes[nodeIndex].node.childIndex + i].nextFree = firstFreeIndex;
	}
	
	firstFreeIndex = nodes[nodeIndex].node.childIndex;
}

template <unsigned int NodeChildrenCount>
void Tree<NodeChildrenCount>::VisitChildren(int nodeIndex, int currentNodeDepth, std::function<void(int, int)> func)
{
	func(nodeIndex, currentNodeDepth);

	if (nodes[nodeIndex].node.childIndex != -1) {
		for (unsigned int i = 0; i < NodeChildrenCount; i++) {
			VisitChildren(nodes[nodeIndex].node.childIndex + i, currentNodeDepth + 1, func);
		}
	}
}

template <unsigned int NodeChildrenCount>
Tree<NodeChildrenCount>::Node& Tree<NodeChildrenCount>::GetNode(int index) {
	return nodes[index].node;
}

template <unsigned int NodeChildrenCount>
void Tree<NodeChildrenCount>::VisitChildren(int nodeIndex, int currentNodeDepth, int targetNodeDepth, std::function<void(int)> func)
{
	if (currentNodeDepth > targetNodeDepth) { return; }
	

	if (currentNodeDepth == targetNodeDepth) {
		func(nodeIndex);
	}

	if (nodes[nodeIndex].node.childIndex != -1) {
		for (unsigned int i = 0; i < NodeChildrenCount; i++) {
			VisitChildren(nodes[nodeIndex].node.childIndex + i, currentNodeDepth + 1, targetNodeDepth, func);
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