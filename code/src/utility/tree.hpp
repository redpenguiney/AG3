#pragma once
#include <vector>
#include <functional>

// Only holds indices to outside data.
// All nodes, not just leaves, can hold data.
// Can be an octree, quadtree, etc. depending on NodeChildrenCount
template <unsigned int NodeChildrenCount>
class Tree {
public:
	Tree();

	struct Node;

	// executes the function for every node of the specified depth (where the root is depth = 0)
	void ForEach(int nodeDepth, std::function<void(Node&)>);

	// executes the function for every node
	void ForEach(std::function<void(Node&)>);

	// Splits the given node, giving it 4 children. The data it refers to is unaffected.
	void Split(Node& node);

	// "Removes" the node's 4 children.
	void Collapse(Node& node);

private:
	const static inline int rootNodeIndex = 0;

	struct Node {
		int childIndex = -1; // (-1 if no children) index to first of NodeChildrenCount children
		int data = -1; // (-1 if no data) index to arbitrary user data stored outside the quadtree. It is expected that this index will be updated if the data it points to moves.
	};

	struct MaybeNode { // used for free list of nodes
		union {
			Node node;
			int nextFree; // nonowning pointer to the next set of NodeChildrenCount free nodes. -1 if there is none.
		};
		
		MaybeNode();
	};

	// free list of nodes. Guaranteed to have free nodes in contiguous sets of NodeChildrenCount so that a node's children are contiguous.
	std::vector<MaybeNode> nodes;
	int firstFreeIndex = -1; // first free node in nodes, -1 if none

	void VisitChildren(int nodeIndex, int nodeDepth, std::function<void(Node&)>);

	void VisitChildren(int nodeIndex, std::function<void(Node&)>);
};

using BinaryTree = Tree<2>;
using Quadtree = Tree<4>;
using Octree = Tree<8>;
using Icoseptree = Tree<27>;
using ChunkTree = Tree<256>;

