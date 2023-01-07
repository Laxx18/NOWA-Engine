#include "NOWAPrecompiled.h"
#include "PathFinding.h"

namespace NOWA
{
	namespace KI
	{
		PathingNode::PathingNode(const Ogre::Vector3& pos, float tolerance) 
			:	pos(pos),
				tolerance(tolerance)
		{

		}

		void PathingNode::addArc(PathingArc* arc)
		{
			assert(arc);
			this->arcs.push_back(arc);
		}

		void PathingNode::getNeighbors(PathingNodeList& outNeighbors)
		{
			for (PathingArcList::iterator it = this->arcs.begin(); it != this->arcs.end(); ++it) {
				outNeighbors.push_back((*it)->getNeighbor(this));
			}
		}

		float PathingNode::getCostFromNode(PathingNode* fromNode)
		{
			assert(fromNode);
			PathingArc* arc = this->findArc(fromNode);
			assert(arc);
			Ogre::Vector3 diff = fromNode->getPos() - this->pos;
			return arc->getWeight() * diff.length(); // or squaredLength?
		}

		PathingArc* PathingNode::findArc(PathingNode* linkedNode)
		{
			assert(linkedNode);

			for (PathingArcList::iterator it = this->arcs.begin(); it != this->arcs.end(); ++it) {
				PathingArc* arc = *it;
				if (arc->getNeighbor(this) == linkedNode) {
					return arc;
				}
			}
			return NULL;
		}

		const Ogre::Vector3& PathingNode::getPos(void) const
		{ 
			return this->pos;
		}

		float PathingNode::getTolerance(void) const
		{ 
			return this->tolerance;
		}

		//--------------------------------------------------------------------------------------------------------
		// PathingArc
		//--------------------------------------------------------------------------------------------------------

		PathingArc::PathingArc(float weight)
			:	weight(weight)
		{
			
		}

		void PathingArc::linkNodes(PathingNode* nodeA, PathingNode* nodeB)
		{
			assert(nodeA);
			assert(nodeB);

			this->nodes[0] = nodeA;
			this->nodes[1] = nodeB;
		}

		PathingNode* PathingArc::getNeighbor(PathingNode* me)
		{
			assert(me);

			// if me is the node get the other connected
			if (this->nodes[0] == me) {
				return this->nodes[1];
			} else {
				return this->nodes[0];
			}
		}

		float PathingArc::getWeight(void) const
		{
			return this->weight; 
		}

		//--------------------------------------------------------------------------------------------------------
		// PathPlan
		//--------------------------------------------------------------------------------------------------------

		PathPlan::PathPlan()
		{
			it = path.end(); 
		}

		bool PathPlan::checkForNextNode(const Ogre::Vector3& pos)
		{
			if (this->it == this->path.end()) {
				return false;
			}

			Ogre::Vector3 diff = pos - (*this->it)->getPos();

			// DEBUG dump target orientation
			//wchar_t str[64];  // I'm sure this is overkill
			//memset(str,0,sizeof(wchar_t));
			//swprintf_s(str,64,_T("distance: %f\n"),diff.Length());
			//GCC_LOG("AI", str);
			// end DEBUG

			if (diff.length() <= (*this->it)->getTolerance()) {
				++this->it;
				return true;
			}
			return false;
		}

		bool PathPlan::checkForEnd(void)
		{
			if (this->it == this->path.end()) {
				return true;
			}
			return false;
		}

		void PathPlan::addNode(PathingNode* node)
		{
			assert(node);
			this->path.push_front(node);
		}

		void PathPlan::resetPath(void)
		{
			it = path.begin();
		}

		const Ogre::Vector3& PathPlan::getCurrentNodePosition(void) const
		{
			assert(it != path.end());
			return (*it)->getPos();
		}

		//--------------------------------------------------------------------------------------------------------
		// PathPlanNode
		//--------------------------------------------------------------------------------------------------------
		PathPlanNode::PathPlanNode(PathingNode* node, PathPlanNode* prevPlanNode, PathingNode* goalNode)
		{
			assert(node);
		
			this->pathNode = node;
			this->prevPlanNode = prevPlanNode;  // NULL is a valid value, though it should only be NULL for the start node
			this->goalNode = goalNode;
			this->closed = false;
			this->updateHeuristics();
		}

		void PathPlanNode::updatePrevNode(PathPlanNode* prevPlanNode)
		{
			assert(prevPlanNode);
			this->prevPlanNode = prevPlanNode;
			this->updateHeuristics();
		}

		void PathPlanNode::updateHeuristics(void)
		{
			// total cost (g)
			if (this->prevPlanNode) {
				this->goal = this->prevPlanNode->getGoal() + this->pathNode->getCostFromNode(this->prevPlanNode->getPathingNode());
			} else {
				this->goal = 0;
			}

			// heuristic (h)
			Ogre::Vector3 diff = this->pathNode->getPos() - this->goalNode->getPos();
			this->heuristic = diff.length();

			// cost to goal (f)
			this->fitness = this->goal + this->heuristic;
		}

		PathPlanNode* PathPlanNode::getPrev(void) const
		{
			return this->prevPlanNode;
		}

		PathingNode* PathPlanNode::getPathingNode(void) const
		{
			return this->pathNode;
		}

		bool PathPlanNode::isClosed(void) const
		{
			return this->closed;
		}

		float PathPlanNode::getGoal(void) const
		{
			return this->goal;
		}

		float PathPlanNode::getHeuristic(void) const
		{
			return this->heuristic;
		}

		float PathPlanNode::getFitness(void) const
		{
			return this->fitness;
		}

		void PathPlanNode::setClosed(bool toClose)
		{
			this->closed = toClose;
		}

		bool PathPlanNode::isBetterChoiceThan(PathPlanNode* right)
		{
			return (this->fitness < right->getFitness());
		}

		//--------------------------------------------------------------------------------------------------------
		// AStar
		//--------------------------------------------------------------------------------------------------------
		AStar::AStar(void)
		{
			this->startNode = NULL;
			this->goalNode = NULL;
		}

		AStar::~AStar(void)
		{
			this->destroy();
		}

		void AStar::destroy(void)
		{
			// destroy all the PathPlanNode objects and clear the map
			for (PathingNodeToPathPlanNodeMap::iterator it = this->nodes.begin(); it != this->nodes.end(); ++it) {
				delete it->second;
			}
			this->nodes.clear();

			// clear the open set
			this->openSet.clear();

			// clear the start & goal nodes
			this->startNode = NULL;
			this->goalNode = NULL;
		}


		//
		// AStar::operator()					- Chapter 18, page 638
		//
		PathPlan* AStar::operator()(PathingNode* startNode, PathingNode* goalNode)
		{
			assert(startNode);
			assert(goalNode);

			// if the start and end nodes are the same, we're close enough to b-line to the goal
			if (startNode == goalNode) {
				return NULL;
			}

			// set our members
			this->startNode = startNode;
			this->goalNode = goalNode;

			// The open set is a priority queue of the nodes to be evaluated.  If it's ever empty, it means 
			// we couldn't find a path to the goal.  The start node is the only node that is initially in 
			// the open set.
			this->addToOpenSet(this->startNode, NULL);

			while (!this->openSet.empty()) {
				// grab the most likely candidate
				PathPlanNode* node = this->openSet.front();

				// If this node is our goal node, we've successfully found a path.
				if (node->getPathingNode() == this->goalNode) {
					return this->rebuildPath(node);
				}

				// we're processing this node so remove it from the open set and add it to the closed set
				this->openSet.pop_front();
				this->addToClosedSet(node);

				// get the neighboring nodes
				PathingNodeList neighbors;
				node->getPathingNode()->getNeighbors(neighbors);

				// loop though all the neighboring nodes and evaluate each one
				for (PathingNodeList::iterator it = neighbors.begin(); it != neighbors.end(); ++it)
				{
					PathingNode* nodeToEvaluate = *it;

					// Try and find a PathPlanNode object for this node.
					PathingNodeToPathPlanNodeMap::iterator findIt = this->nodes.find(nodeToEvaluate);

					// If one exists and it's in the closed list, we've already evaluated the node.  We can
					// safely skip it.
					if (findIt != this->nodes.end() && findIt->second->isClosed()) {
						continue;
					}

					// figure out the cost for this route through the node
					float costForThisPath = node->getGoal() + nodeToEvaluate->getCostFromNode(node->getPathingNode());
					bool isPathBetter = false;

					// Grab the PathPlanNode if there is one.
					PathPlanNode* pathPlanNodeToEvaluate = NULL;
					if (findIt != this->nodes.end()) {
						pathPlanNodeToEvaluate = findIt->second;
					}

					// No PathPlanNode means we've never evaluated this pathing node so we need to add it to 
					// the open set, which has the side effect of setting all the heuristic data.  It also 
					// means that this is the best path through this node that we've found so the nodes are 
					// linked together (which is why we don't bother setting isPathBetter to true; it's done
					// for us in AddToOpenSet()).
					if (!pathPlanNodeToEvaluate) {
						pathPlanNodeToEvaluate = this->addToOpenSet(nodeToEvaluate,node);
					} else if (costForThisPath < pathPlanNodeToEvaluate->getGoal()) {
						// If this node is already in the open set, check to see if this route to it is better than
						// the last.
						isPathBetter = true;
					}

					// If this path is better, relink the nodes appropriately, update the heuristics data, and
					// reinsert the node into the open list priority queue.
					if (isPathBetter) {
						pathPlanNodeToEvaluate->updatePrevNode(node);
						this->reInsertNode(pathPlanNodeToEvaluate);
					}
				}
			}

			// If we get here, there's no path to the goal.
			return NULL;
		}

		PathPlanNode* AStar::addToOpenSet(PathingNode* node, PathPlanNode* prevNode)
		{
			assert(node);

			// create a new PathPlanNode if necessary
			PathingNodeToPathPlanNodeMap::iterator it = this->nodes.find(node);
			PathPlanNode* thisNode = NULL;
			if (it == this->nodes.end()) {
				thisNode = new PathPlanNode(node, prevNode, this->goalNode);
				this->nodes.insert(std::make_pair(node, thisNode));
			}
			else
			{
				////  GCC_WARNING("Adding existing PathPlanNode to open set");
				thisNode = it->second;
				thisNode->setClosed(false);
			}

			// now insert it into the priority queue
			this->insertNode(thisNode);

			return thisNode;
		}

		void AStar::addToClosedSet(PathPlanNode* node)
		{
			assert(node);
			node->setClosed();
		}

		//
		// AStar::InsertNode					- Chapter 17, page 636
		//
		void AStar::insertNode(PathPlanNode* node)
		{
			assert(node);

			// just add the node if the open set is empty
			if (this->openSet.empty()) {
				this->openSet.push_back(node);
				return;
			}

			// otherwise, perform an insertion sort	
			PathPlanNodeList::iterator it = this->openSet.begin();
			PathPlanNode* compare = (*this->openSet.begin());
			while (compare->isBetterChoiceThan(node)) {
				++it;

				if (it != this->openSet.end()) {
					compare = *it;
				} else {
					break;
				}
			}
			this->openSet.insert(it,node);
		}

		void AStar::reInsertNode(PathPlanNode* node)
		{
			assert(node);

			for (PathPlanNodeList::iterator it = this->openSet.begin(); it != this->openSet.end(); ++it) {
				if (node == (*it)) {
					this->openSet.erase(it);
					this->insertNode(node);
					return;
				}
			}

			// if we get here, the node was never in the open set to begin with
			///   GCC_WARNING("Attemping to reinsert node that was never in the open list");
			this->insertNode(node);
		}

		PathPlan* AStar::rebuildPath(PathPlanNode* goalNode)
		{
			assert(goalNode);

			PathPlan* plan = new PathPlan;

			PathPlanNode* node = goalNode;
			while (node) {
				plan->addNode(node->getPathingNode());
				node = node->getPrev();
			}

			return plan;
		}


		//--------------------------------------------------------------------------------------------------------
		// PathingGraph
		//--------------------------------------------------------------------------------------------------------

		PathingGraph::PathingGraph()
		{

		}

		PathingGraph::~PathingGraph(void)
		{
			this->destroyGraph();
		}
		
		PathingGraph* PathingGraph::get()
		{
			static PathingGraph instance;

			return &instance;
		}

		void PathingGraph::destroyGraph(void)
		{
			// destroy all the nodes
			for (PathingNodeVec::iterator it = this->nodes.begin(); it != this->nodes.end(); ++it) {
				delete (*it);
			}
			this->nodes.clear();

			// destroy all the arcs
			for (PathingArcList::iterator it = this->arcs.begin(); it != this->arcs.end(); ++it) {
				delete (*it);
			}
			this->arcs.clear();
		}

		PathingNode* PathingGraph::findClosestNode(const Ogre::Vector3& pos)
		{
			// This is a simple brute-force O(n) algorithm that could be made a LOT faster by utilizing
			// spatial partitioning, like an octree (or quadtree for flat worlds) or something similar.
			PathingNode* closestNode = this->nodes.front();
			float length = FLT_MAX;
			for (PathingNodeVec::iterator it = this->nodes.begin(); it != this->nodes.end(); ++it) {
				PathingNode* node = *it;
				Ogre::Vector3 diff = pos - node->getPos();
				if (diff.length() < length) {
					closestNode = node;
					length = diff.length();
				}
			}

			return closestNode;
		}

		PathingNode* PathingGraph::findFurthestNode(const Ogre::Vector3& pos)
		{
			// This is a simple brute-force O(n) algorithm that could be made a LOT faster by utilizing
			// spatial partitioning, like an octree (or quadtree for flat worlds) or something similar.
			PathingNode* furthestNode = this->nodes.front();
			float length = 0;
			for (PathingNodeVec::iterator it = this->nodes.begin(); it != this->nodes.end(); ++it) {
				PathingNode* node = *it;
				Ogre::Vector3 diff = pos - node->getPos();
				if (diff.length() > length) {
					furthestNode = node;
					length = diff.length();
				}
			}

			return furthestNode;
		}

		PathingNode* PathingGraph::findRandomNode(void)
		{
			// cache this since it's not guaranteed to be constant time
			unsigned int numNodes = static_cast<unsigned int>(this->nodes.size());

			// choose a random node
			// todo insert random from gameapp
			unsigned int node = static_cast<unsigned int>(Ogre::Math::RangeRandom(0.0f, static_cast<Ogre::Real>(numNodes))); 
			
			// if we're in the lower half of the node list, start from the bottom
			if (node <= numNodes / 2) {
				PathingNodeVec::iterator it = this->nodes.begin();
				for (unsigned int i = 0; i < node; i++) {
					++it;
				}
				return (*it);
			} else {
				// otherwise, start from the top
				PathingNodeVec::iterator it = this->nodes.end();
				for (unsigned int i = numNodes; i >= node; i--) {
					--it;
				}
				return (*it);
			}
		}

		PathPlan* PathingGraph::findPath(const Ogre::Vector3& startPoint, const Ogre::Vector3& endPoint)
		{
			// Find the closest nodes to the start and end points.  There really should be some ray-casting 
			// to ensure that we can actually make it to the closest node, but this is good enough for now.
			PathingNode* start = this->findClosestNode(startPoint);
			PathingNode* goal = this->findClosestNode(endPoint);
			return this->findPath(start, goal);
		}

		PathPlan* PathingGraph::findPath(const Ogre::Vector3& startPoint, PathingNode* goalNode)
		{
			PathingNode* start = this->findClosestNode(startPoint);
			return this->findPath(start, goalNode);
		}

		PathPlan* PathingGraph::findPath(PathingNode* startNode, const Ogre::Vector3& endPoint)
		{
			PathingNode* goal = this->findClosestNode(endPoint);
			return this->findPath(startNode, goal);
		}

		PathPlan* PathingGraph::findPath(PathingNode* startNode, PathingNode* goalNode)
		{
			// find the best path using an A* search algorithm
			AStar aStar;
			return aStar(startNode, goalNode);
		}

		void PathingGraph::buildTestGraph(void)
		{
			// this should never occur, but better safe than sorry
			if (!this->nodes.empty()) {
				this->destroyGraph();
			}

			// keep from reallocating and copying the array
			this->nodes.reserve(81);

			// Create a simple grid of nodes.  Using these hard-coded values is a bit hacky but it's okay 
			// because this is just a debug function.
			int index = 0;  // this is used to keep track of the node we just inserted so we can link it to adjacent nodes
			for (float x = -45.0f; x < 45.0f; x += 10.0f) {
				for (float z = -45.0f; z < 45.0f; z += 10.0f) {
					// add the new node
					PathingNode* node = new PathingNode(Ogre::Vector3(x, 0.0f, z));
					this->nodes.push_back(node);

					// link it to the previous node
					int temnode = index - 1;
					if (temnode >= 0) {
						this->linkNodes(this->nodes[temnode], node);
					}

					// link it to the node above it
					temnode = index - 9;  // reusing temnode
					if (temnode >= 0) {
						this->linkNodes(this->nodes[temnode], node);
					}

					index++;
				}
			}
		}

		void PathingGraph::linkNodes(PathingNode* nodeA, PathingNode* nodeB)
		{
			assert(nodeA);
			assert(nodeB);

			PathingArc* arc = new PathingArc;
			arc->linkNodes(nodeA, nodeB);
			nodeA->addArc(arc);
			nodeB->addArc(arc);
			this->arcs.push_back(arc);
		}


		//--------------------------------------------------------------------------------------------------------
		// Global functions
		//--------------------------------------------------------------------------------------------------------
		/*PathingGraph* CreatePathingGraph(void)
		{
			PathingGraph* pathingGraph = new PathingGraph;
			pathingGraph->buildTestGraph();
			return pathingGraph;
		}*/

	}; // namespace end KI

}; // namespace end NOWA