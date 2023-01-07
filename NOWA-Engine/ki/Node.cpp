#include "NOWAPrecompiled.h"
#include "Node.h"

namespace NOWA
{
	namespace KI
	{
		Node::Node()
		:
		index(-1) // invalid index default
		{

		}

		Node::Node(int index)
		:
		index(index)
		{

		}

		Node::~Node()
		{

		}

		void Node::setIndex(int index)
		{
			this->index = index;
		}

		int Node::getIndex(void) const
		{
			return this->index;
		}

	}; //end namespace KI

}; //end namespace NOWA