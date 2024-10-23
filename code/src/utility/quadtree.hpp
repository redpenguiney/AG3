#pragma once
#include <vector>
#include <functional>

// Only holds indices to outside data.
// All nodes, not just leaves, can hold data.
class Quadtree {
public:
	Quadtree();

	struct Node;

	// executes the function for every node of the specified depth
	void ForEach(int nodeDepth, std::function<void(Node&)>);

	// executes the function for every node
	void ForEach(std::function<void(Node&));

	// Splits the given node, giving it 4 children. The data it refers to is unaffected.
	void Split(Node& node);

	// "Removes" the node's 4 children.
	void Collapse();

private:
	const static inline int rootNodeIndex = 0;

	struct Node {
		int childIndex = -1; // (-1 if no children) index to first of 4 children
		int data = -1; // (-1 if no data) index to arbitrary user data stored outside the quadtree. It is expected that this index will be updated if the data it points to moves.
	};

	struct MaybeNode { // used for free list of nodes
		union {
			Node node;
			Node* nextFree = nullptr; // nonowning pointer to the next set of 4 free nodes. 
		};
		
	};

	// free list of nodes. Guaranteed to have free nodes in contiguous sets of 4 so that a node's children are contiguous.
	std::vector<MaybeNode> nodes;
	int firstFreeIndex; // first free node in nodes
};