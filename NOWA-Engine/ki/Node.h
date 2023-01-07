#ifndef NODE_H
#define NODE_H

#include "defines.h"
#include <fstream>

namespace NOWA
{
	namespace KI
	{
		class EXPORTED Node
		{
		public:
			Node();
			Node(int index);
			virtual ~Node();
			void setIndex(int index);
			int getIndex(void) const;
		protected:
			int index;
		};

		/******************************************************************************/

		template <class ExtraInfo = void*>
		class EXPORTED NavNode : public Node
		{
		public:
			NavNode()
			:
			extraInfo(ExtraInfo)
			{

			}

			NavNode(int index, Ogre::Vector3 position)
			:
			Node	(index),
			position(position),
			extraInfo(ExtraInfo)
			{

			}

			NavNode(std::ifstream& stream)
			:
			extraInfo(ExtraInfo)
			{
				char buffer[50]; // are 50 bytes enough?
				stream >> buffer >> this->index >> this->position.x >> buffer >> this->position.y >> buffer >> this->position.z;
			}

			virtual ~NavNode()
			{

			}

			void setPosition(Ogre::Vector3 position)
			{
				this->position = position;
			}

			Ogre::Vector3 getPosition(void) const
			{
				return this->position;
			}

			void setExtraInfo(ExtraInfo extraInfo)
			{
				this->extraInfo = extraInfo;
			}

			ExtraInfo getExtraInfo(void) const
			{
				return this->extraInfo;
			}
		protected:
			Ogre::Vector3 position;
			//often you will require a navgraph node to contain additional information.
			//For example a node might represent a pickup such as armor in which
			//case m_ExtraInfo could be an enumerated value denoting the pickup type,
			//thereby enabling a search algorithm to search a graph for specific items.
			//Going one step further, extraInfo could be a pointer to the instance of
			//the item type the node is twinned with. This would allow a search algorithm
			//to test the status of the pickup during the search. 
			ExtraInfo  extraInfo;
		};


	}; //end namespace KI
}; //end namespace NOWA

#endif