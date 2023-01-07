#if defined _MSC_VER && _MSC_VER >= 1020 
#pragma once
#endif

#ifndef PATH_FINDING_H
#define PATH_FINDING_H

#include "defines.h"

namespace NOWA
{
	namespace KI
	{

		class PathingArc;
		class PathingNode;
		class PathPlanNode;
		class AStar;

		typedef std::list<PathingArc*> PathingArcList;
		typedef std::list<PathingNode*> PathingNodeList;
		typedef std::vector<PathingNode*> PathingNodeVec;
		typedef std::map<PathingNode*, PathPlanNode*> PathingNodeToPathPlanNodeMap;
		typedef std::list<PathPlanNode*> PathPlanNodeList;

		const float PATHING_DEFAULT_NODE_TOLERANCE = 5.0f;
		const float PATHING_DEFAULT_ARC_WEIGHT = 1.0f;

		//--------------------------------------------------------------------------------------------------------
		// class PathingNode				- Chapter 18, page 636
		// This class represents a single node in the pathing graph.
		//
		//--------------------------------------------------------------------------------------------------------
		class PathingNode
		{
			float tolerance;
			Ogre::Vector3 pos;
			PathingArcList arcs;

		public:
			explicit PathingNode(const Ogre::Vector3& pos, float tolerance = PATHING_DEFAULT_NODE_TOLERANCE);

			const Ogre::Vector3& getPos(void) const;

			float getTolerance(void) const;

			void addArc(PathingArc* arc);

			void getNeighbors(PathingNodeList& outNeighbors);

			float getCostFromNode(PathingNode* fromNode);

		private:
			PathingArc* findArc(PathingNode* linkedNode);

			// GCC_MEMORYPOOL_DECLARATION(0);
		};


		//--------------------------------------------------------------------------------------------------------
		// class PathingArc				- Chapter 18, page 636
		// This class represents an arc that links two nodes, allowing travel between them.
		//--------------------------------------------------------------------------------------------------------
		class PathingArc
		{
			float weight;
			PathingNode* nodes[2];  // an arc always connects two nodes

		public:
			explicit PathingArc(float weight = PATHING_DEFAULT_ARC_WEIGHT);

			float getWeight(void) const;

			void linkNodes(PathingNode* nodeA, PathingNode* nodeB);

			PathingNode* getNeighbor(PathingNode* me);
		};


		//--------------------------------------------------------------------------------------------------------
		// class PathingPlan				- Chapter 18, page 636
		// This class represents a complete path and is used by the higher-level AI to determine where to go.
		//--------------------------------------------------------------------------------------------------------
		class PathPlan
		{
			friend class AStar;

			PathingNodeList path;
			PathingNodeList::iterator it;

		public:
			PathPlan();

			void resetPath(void);

			const Ogre::Vector3& getCurrentNodePosition(void) const;

			bool checkForNextNode(const Ogre::Vector3& pos);

			bool checkForEnd(void);

		private:
			void addNode(PathingNode* node);
		};


		//--------------------------------------------------------------------------------------------------------
		// class PathPlanNode						- Chapter 18, page 636
		// This class is a helper used in PathingGraph::FindPath().  It tracks the heuristical and cost data for
		// a given node when building a path.
		//--------------------------------------------------------------------------------------------------------
		class PathPlanNode
		{
			PathPlanNode* prevPlanNode;  // node we just came from
			PathingNode* pathNode;  // pointer to the pathing node from the pathing graph
			PathingNode* goalNode;  // pointer to the goal node
			bool closed;  // the node is closed if it's already been processed
			float goal;  // cost of the entire path up to this point (often called g)
			float heuristic;  // estimated cost of this node to the goal (often called h)
			float fitness;  // estimated cost from start to the goal through this node (often called f)

		public:
			explicit PathPlanNode(PathingNode* node, PathPlanNode* prevPlanNode, PathingNode* goalNode);

			PathPlanNode* getPrev(void) const;

			PathingNode* getPathingNode(void) const;

			bool isClosed(void) const;

			float getGoal(void) const;

			float getHeuristic(void) const;

			float getFitness(void) const;

			void updatePrevNode(PathPlanNode* prevPlanNode);

			void setClosed(bool toClose = true);

			bool isBetterChoiceThan(PathPlanNode* right);

		private:
			void updateHeuristics(void);
		};


		//--------------------------------------------------------------------------------------------------------
		// class AStar								- Chapter 18, page 638
		// This class implements the A* algorithm.
		//--------------------------------------------------------------------------------------------------------
		class AStar
		{
			PathingNodeToPathPlanNodeMap nodes;
			PathingNode* startNode;
			PathingNode* goalNode;
			PathPlanNodeList openSet;

		public:
			AStar(void);
			~AStar(void);
			void destroy(void);

			PathPlan* operator()(PathingNode* startNode, PathingNode* goalNode);

		private:
			PathPlanNode* addToOpenSet(PathingNode* node, PathPlanNode* prevNode);

			void addToClosedSet(PathPlanNode* node);

			void insertNode(PathPlanNode* node);

			void reInsertNode(PathPlanNode* node);

			PathPlan* rebuildPath(PathPlanNode* goalNode);
		};


		//--------------------------------------------------------------------------------------------------------
		// class PathingGraph					- Chapter 18, 636
		// This class is the main interface into the pathing system.  It holds the pathing graph itself and owns
		// all the PathingNode and Pathing Arc objects.  There is only one instance of PathingGraph, which lives
		//--------------------------------------------------------------------------------------------------------
		class PathingGraph
		{
			PathingNodeVec nodes;  // master list of all nodes
			PathingArcList arcs;  // master list of all arcs

		public:
			PathingGraph(void);

			~PathingGraph(void);

			void destroyGraph(void);

			PathingNode* findClosestNode(const Ogre::Vector3& pos);

			PathingNode* findFurthestNode(const Ogre::Vector3& pos);

			PathingNode* findRandomNode(void);

			PathPlan* findPath(const Ogre::Vector3& startPoint, const Ogre::Vector3& endPoint);

			PathPlan* findPath(const Ogre::Vector3& startPoint, PathingNode* goalNode);

			PathPlan* findPath(PathingNode* startNode, const Ogre::Vector3& endPoint);

			PathPlan* findPath(PathingNode* startNode, PathingNode* goalNode);

			// debug functions
			void buildTestGraph(void);
		public:
			public:
			static PathingGraph* get();

		private:
			// helpers
			void linkNodes(PathingNode* nodeA, PathingNode* nodeB);
		};

	}; // namespace end KI

}; // namespace end NOWA

#endif

