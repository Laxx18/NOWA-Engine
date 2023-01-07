#ifndef EDGE_H
#define EDGE_H

#include "defines.h"
#include <fstream>

namespace NOWA
{
	namespace KI
	{
		class EXPORTED Edge
		{
		public:
			Edge();
			Edge(int from, int to, double cost);
			Edge(int from, int to);
			Edge(std::ifstream& stream);
			virtual ~Edge();

			void setFrom(int from);
			int getFrom(void) const;
			
			void setTo(int to);
			int getTo(void) const;
			
			void setCost(double cost);
			double getCost(void) const;

			bool operator==(const Edge& otherEdge);
			bool operator!=(const Edge& otherEdge);
		protected:
			int		from;
			int		to;
			double	cost;
		};

		/******************************************************************************/

		class EXPORTED NavEdge : public Edge
		{
		public:
			//examples of typical flags
			enum
			{
				normal				= 0,
				swim				= 1 << 0,
				crawl				= 1 << 1,
				jump				= 1 << 3,
				fly					= 1 << 4,
				goesThroughDoor		= 1 << 5
			};

			NavEdge(int from, int to, double cost, int flags = 0, int intersectingGameObjectId = -1);
			NavEdge(std::ifstream& stream);
			~NavEdge();

			void setFlags(int flags);
			int getFlags(void) const;

			void setIntersectingGameObjectId(int intersectingGameObjectId);
			int getIntersectingGameObjectId(void) const;

		protected:
			int flags;
			int intersectingGameObjectId;
		};


	}; //end namespace KI
}; //end namespace NOWA

#endif