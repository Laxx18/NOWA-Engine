#include "NOWAPrecompiled.h"
#include "Edge.h"

namespace NOWA
{
	namespace KI
	{

		Edge::Edge()
		:
		from(-1),
		to	(-1),
		cost(1.0)
		{

		}

		Edge::Edge(int from, int to, double cost)
		:
		from(from),
		to	(to),
		cost(cost)
		{

		}

		Edge::Edge(int from, int to)
		:
		from(from),
		to	(to),
		cost(1.0)
		{

		}

		Edge::Edge(std::ifstream& stream)
		{
			char buffer[50];
			stream >> buffer >> this->from >> buffer >> this->to >> buffer >> this->cost;
		}

		Edge::~Edge()
		{

		}

		void Edge::setFrom(int from)
		{
			this->from = from;
		}

		int Edge::getFrom(void) const
		{
			return this->from;
		}

		void Edge::setTo(int to)
		{
			this->to = to;
		}

		int Edge::getTo(void) const
		{
			return this->to;
		}

		void Edge::setCost(double cost)
		{
			this->cost = cost;
		}

		double Edge::getCost(void) const
		{
			return this->cost;
		}

		bool Edge::operator==(const Edge& otherEdge)
		{
			return ((otherEdge.from == this->from) && (otherEdge.to == this->to) && (otherEdge.cost == this->cost));
		}

		bool Edge::operator!=(const Edge& otherEdge)
		{
			return !(*this == otherEdge);
		}

		/******************************************************************************/

		NavEdge::NavEdge(int from, int to, double cost, int flags, int intersectingGameObjectId)
		:
		Edge					(from, to, cost),
		flags					(flags),
		intersectingGameObjectId(intersectingGameObjectId)
		{

		}

		NavEdge::NavEdge(std::ifstream& stream)
		{
			char buffer[50];
			stream >> buffer >> this->from >> buffer >> this->to >> buffer >> this->cost;
			stream >> buffer >> this->flags >> buffer >> this->intersectingGameObjectId;
		}

		NavEdge::~NavEdge()
		{

		}

		void NavEdge::setFlags(int flags)
		{
			this->flags = flags;
		}

		int NavEdge::getFlags(void) const
		{
			return this->flags;
		}

		void NavEdge::setIntersectingGameObjectId(int intersectingGameObjectId)
		{
			this->intersectingGameObjectId = intersectingGameObjectId;
		}

		int NavEdge::getIntersectingGameObjectId(void) const
		{
			return this->intersectingGameObjectId;
		}

	}; //end namespace KI
}; //end namespace NOWA