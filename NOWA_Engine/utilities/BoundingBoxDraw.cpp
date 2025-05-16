#include "NOWAPrecompiled.h"
#include "BoundingBoxDraw.h"
#include "modules/RenderCommandQueueModule.h"

namespace NOWA
{
	BoundingBoxDraw::BoundingBoxDraw(Ogre::SceneManager* sceneManager)
		: manualObject(nullptr),
		node(nullptr),
		sceneManager(sceneManager)
	{
		// Create the manual object
		this->manualObject = this->sceneManager->createManualObject(Ogre::SCENE_DYNAMIC);
		this->manualObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
		this->manualObject->setCastShadows(false);

		// Create the node
		this->node = this->sceneManager->getRootSceneNode(Ogre::SCENE_DYNAMIC)->createChildSceneNode(Ogre::SCENE_DYNAMIC);

		// Attach the manual object to the node
		this->node->attachObject(this->manualObject);

		// Hide the scene node
		this->node->setVisible(false);

		this->manualObject->estimateVertexCount(24);
		this->manualObject->estimateIndexCount(12);
	}

	BoundingBoxDraw::~BoundingBoxDraw()
	{
		// Clear all targets
		this->clear();

		// Detach the manual object from the node
		this->node->detachAllObjects();

		// Destroy the manual object
		this->sceneManager->destroyManualObject(this->manualObject);
		this->manualObject = nullptr;

		// Destroy the node
		if (this->node->getParent() != nullptr)
		{
			((Ogre::SceneNode*)this->node->getParent())->removeChild(this->node);
		}
		NOWA::RenderCommandQueueModule::getInstance()->removeTrackedNode(this->node);
		this->sceneManager->destroySceneNode(this->node);
		this->node = nullptr;
	}

	void BoundingBoxDraw::add(Ogre::SceneNode* sceneNode, const Ogre::ColourValue& color)
	{
		// Check the target specified has already been added
		if (this->isAdded(sceneNode))
		{
			// Return the function, we are done
			return;
		}

		// Add the target specified
		this->targetSceneNodeIDs.push_back(sceneNode->getId());
		this->targetColors.push_back(color);
	}

	void BoundingBoxDraw::toggle(Ogre::SceneNode* sceneNode, const Ogre::ColourValue& color)
	{
		// Check the target specified has already been added
		if (this->isAdded(sceneNode))
		{
			// this->remove the target
			this->remove(sceneNode);
		}
		else
		{
			// Add the target
			this->add(sceneNode, color);
		}
	}

	void BoundingBoxDraw::remove(Ogre::SceneNode* sceneNode)
	{
		// Check the target specified has already been added
		int tmpIndex = this->indexOf<Ogre::IdType>(&this->targetSceneNodeIDs, sceneNode->getId());
		if (tmpIndex != -1)
		{
			// this->remove the target specified
			this->remove<Ogre::IdType>(&this->targetSceneNodeIDs, tmpIndex);
			this->remove<Ogre::ColourValue>(&this->targetColors, tmpIndex);

			// Check if we now have no targets left
			if (this->targetSceneNodeIDs.size() == 0 && this->targetAABBs.size() == 0)
			{
				// Hide the scene node
				this->node->setVisible(false);
			}
		}
	}

	bool BoundingBoxDraw::isAdded(Ogre::SceneNode* sceneNode)
	{
		// Check the target specified has been added
		int tmpIndex = this->indexOf<Ogre::IdType>(&this->targetSceneNodeIDs, sceneNode->getId());
		if (tmpIndex != -1)
		{
			// Return that the target has been added
			return true;
		}
		// Return that the target has not been added
		return false;
	}

	void BoundingBoxDraw::clear(void)
	{
		// Clear the list of targets
		this->targetSceneNodeIDs.clear();
		this->targetColors.clear();
		this->targetAABBs.clear();
		this->targetAABBColors.clear();
		this->targetAABBIDs.clear();

		// Hide the scene node
		this->node->setVisible(false);
	}

	void BoundingBoxDraw::update(void)
	{
		// Check if we have not yet initialized the manager
		if (!this->node)
		{
			// Return the function, we are done
			return;
		}

		// Check if we have at least one target
		if (this->targetSceneNodeIDs.size() != 0 || this->targetAABBs.size() != 0)
		{
			// Show the the scene node
			this->node->setVisible(true);

			// Loop through all targets
			std::vector<Ogre::AxisAlignedBox> tmpAABBs;
			std::vector<Ogre::ColourValue> tmpColors;
			unsigned int i = 0;
			for (; i < this->targetSceneNodeIDs.size(); i++)
			{
				// Get the current target
				Ogre::IdType tmpSceneNodeId = this->targetSceneNodeIDs[i];
				Ogre::ColourValue tmpColor = this->targetColors[i];

				// Get the scene node
				Ogre::SceneNode* tmpSceneNode = this->sceneManager->getSceneNode(tmpSceneNodeId);
				if (!tmpSceneNode)
				{
					Ogre::SceneNode* tmpRootSceneNode = this->sceneManager->getRootSceneNode(Ogre::SCENE_STATIC);
					if (tmpRootSceneNode->getId() == tmpSceneNodeId)
					{
						tmpSceneNode = tmpRootSceneNode;
					}
					tmpRootSceneNode = this->sceneManager->getRootSceneNode(Ogre::SCENE_DYNAMIC);
					if (tmpRootSceneNode->getId() == tmpSceneNodeId)
					{
						tmpSceneNode = tmpRootSceneNode;
					}
				}

				// Check if the scene node is valid
				if (tmpSceneNode)
				{
					// Fetch all AABBs of the scene node
					this->fetchAABBs(&tmpAABBs, tmpSceneNode);

					// Add the color to the AABBs fetched
					while (tmpColors.size() < tmpAABBs.size())
					{
						tmpColors.push_back(tmpColor);
					}
				}
				else
				{
					// this->remove the current target, it is no longer exists
					this->remove<Ogre::IdType>(&this->targetSceneNodeIDs, i);
					this->remove<Ogre::ColourValue>(&this->targetColors, i);
					i--;
				}
			}

			// Loop through the list of target AABBs
			i = 0;
			for (; i < this->targetAABBs.size(); i++)
			{
				// Add the current target AABB
				tmpAABBs.push_back(this->targetAABBs[i]);
				tmpColors.push_back(this->targetAABBColors[i]);
			}

			// Start updating the manual object
			//if (this->manualObject && this->manualObject->getNumSections() > 0)
			//{
			//	try
			//	{
			//		this->manualObject->beginUpdate(0);
			//	}
			//	catch (...)
			//	{
			//		this->manualObject->clear();
			//		this->manualObject->begin(/*"BaseWhiteNoLighting"*/"RedNoLighting", Ogre::OperationType::OT_LINE_LIST);
			//	}
			//}
			//else
			//{
				// this->manualObject->setDynamic(true);
				this->manualObject->clear();
				this->manualObject->begin(/*"BaseWhiteNoLighting"*/"RedNoLighting", Ogre::OperationType::OT_LINE_LIST);
			// }

			// Loop through all AABBs
			i = 0;
			for (; i < tmpAABBs.size(); i++)
			{
				// Check if the current AABB is invalid
				Ogre::AxisAlignedBox tmpAABB = tmpAABBs[i];
				if (!tmpAABB.isFinite())
				{
					// Continue to the next loop
					continue;
				}

				// Add the current AABB to the manual object
				Ogre::ColourValue tmpColor = tmpColors[i];
				const Ogre::Vector3* tmpCorners = tmpAABB.getAllCorners();


				//       1-------2
				//      /|      /|
				//     / |     / |
				//    5-------4  |
				//    |  0----|--3
				//    | /     | /
				//    |/      |/
				//    6-------7

				this->manualObject->position(tmpCorners[0]); this->manualObject->colour(tmpColor);
				this->manualObject->position(tmpCorners[3]); this->manualObject->colour(tmpColor);
				this->manualObject->line(0, 1);
				this->manualObject->position(tmpCorners[0]); this->manualObject->colour(tmpColor);
				this->manualObject->position(tmpCorners[6]); this->manualObject->colour(tmpColor);
				this->manualObject->line(2, 3);
				this->manualObject->position(tmpCorners[7]); this->manualObject->colour(tmpColor);
				this->manualObject->position(tmpCorners[3]); this->manualObject->colour(tmpColor);
				this->manualObject->line(4, 5);
				this->manualObject->position(tmpCorners[7]); this->manualObject->colour(tmpColor);
				this->manualObject->position(tmpCorners[6]); this->manualObject->colour(tmpColor);
				this->manualObject->line(6, 7);

				this->manualObject->position(tmpCorners[1]); this->manualObject->colour(tmpColor);
				this->manualObject->position(tmpCorners[2]); this->manualObject->colour(tmpColor);
				this->manualObject->line(8, 9);
				this->manualObject->position(tmpCorners[1]); this->manualObject->colour(tmpColor);
				this->manualObject->position(tmpCorners[5]); this->manualObject->colour(tmpColor);
				this->manualObject->line(10, 11);
				this->manualObject->position(tmpCorners[4]); this->manualObject->colour(tmpColor);
				this->manualObject->position(tmpCorners[2]); this->manualObject->colour(tmpColor);
				this->manualObject->line(12, 13);
				this->manualObject->position(tmpCorners[4]); this->manualObject->colour(tmpColor);
				this->manualObject->position(tmpCorners[5]); this->manualObject->colour(tmpColor);
				this->manualObject->line(14, 15);

				this->manualObject->position(tmpCorners[0]); this->manualObject->colour(tmpColor);
				this->manualObject->position(tmpCorners[1]); this->manualObject->colour(tmpColor);
				this->manualObject->line(16, 17);
				this->manualObject->position(tmpCorners[3]); this->manualObject->colour(tmpColor);
				this->manualObject->position(tmpCorners[2]); this->manualObject->colour(tmpColor);
				this->manualObject->line(18, 19);
				this->manualObject->position(tmpCorners[7]); this->manualObject->colour(tmpColor);
				this->manualObject->position(tmpCorners[4]); this->manualObject->colour(tmpColor);
				this->manualObject->line(20, 21);
				this->manualObject->position(tmpCorners[6]); this->manualObject->colour(tmpColor);
				this->manualObject->position(tmpCorners[5]); this->manualObject->colour(tmpColor);
				this->manualObject->line(22, 23);
			}

			// Clear the temporary lists
			tmpAABBs.clear();
			tmpColors.clear();

			// End updating the manual object
			this->manualObject->end();
		}
		else
		{
			// Hide the scene node
			this->node->setVisible(false);
		}
	}

	void BoundingBoxDraw::add(const Ogre::AxisAlignedBox& aabb, const Ogre::ColourValue& color, int uniqueID)
	{
		// Check the target specified has already been added
		if (this->isAdded(uniqueID))
		{
			// Return the function, we are done
			return;
		}
		// Add the target specified
		this->targetAABBs.emplace_back(aabb);
		this->targetAABBColors.emplace_back(color);
		this->targetAABBIDs.emplace_back(uniqueID);
	}

	void BoundingBoxDraw::remove(int uniqueID)
	{
		// Check the target specified has already been added
		int tmpIndex = this->indexOf<int>(&this->targetAABBIDs, uniqueID);
		if (tmpIndex != -1)
		{
			// this->remove the target specified
			this->remove<Ogre::AxisAlignedBox>(&this->targetAABBs, tmpIndex);
			this->remove<Ogre::ColourValue>(&this->targetAABBColors, tmpIndex);
			this->remove<int>(&this->targetAABBIDs, tmpIndex);

			// Check if we now have no targets left
			if (this->targetSceneNodeIDs.size() == 0 && this->targetAABBs.size() == 0)
			{
				// Hide the scene node
				this->node->setVisible(false);
			}
		}
	}

	void BoundingBoxDraw::toggle(const Ogre::AxisAlignedBox& aabb, const Ogre::ColourValue& color, int uniqueID)
	{
		// Check the target specified has already been added
		if (this->isAdded(uniqueID))
		{
			// this->remove the target
			this->remove(uniqueID);
		}
		else
		{
			// Add the target
			this->add(aabb, color, uniqueID);
		}
	}

	bool BoundingBoxDraw::isAdded(int uniqueID)
	{
		// Check the target specified has been added
		int tmpIndex = this->indexOf<int>(&this->targetAABBIDs, uniqueID);
		if (tmpIndex != -1)
		{
			// Return that the target has been added
			return true;
		}
		// Return that the target has not been added
		return false;
	}

	Ogre::AxisAlignedBox BoundingBoxDraw::aabbToAxisAlignedBox(Ogre::Aabb aabb)
	{
		Ogre::AxisAlignedBox tmpRet;
		if (aabb.mHalfSize == Ogre::Aabb::BOX_NULL.mHalfSize)
			tmpRet.setNull();
		else if (aabb.mHalfSize == Ogre::Aabb::BOX_INFINITE.mHalfSize)
			tmpRet.setInfinite();
		else
			tmpRet.setExtents(aabb.getMinimum(), aabb.getMaximum());
		return tmpRet;
	}

	void BoundingBoxDraw::fetchAABBs(std::vector<Ogre::AxisAlignedBox>* aabbs, Ogre::SceneNode* node)
	{
		// Loop through all attached objects of the scene node
		Ogre::SceneNode::ObjectIterator tmpItr = node->getAttachedObjectIterator();
		while (tmpItr.hasMoreElements())
		{
			// Get the AABB of the current attached object
			Ogre::MovableObject* tmpMovableObject = tmpItr.getNext();
			Ogre::AxisAlignedBox tmpAABB = this->aabbToAxisAlignedBox(tmpMovableObject->getWorldAabbUpdated());

			// Add the AABB to the list of AABBs
			aabbs->emplace_back(tmpAABB);
		}

		// Loop through all attached objects of the scene node
		Ogre::Node::NodeVecIterator tmpChildItr = node->getChildIterator();
		while (tmpChildItr.hasMoreElements())
		{
			// Fetch all AABBs of the current child scene node
			Ogre::Node* tmpSceneNode = tmpChildItr.getNext();
			fetchAABBs(aabbs, (Ogre::SceneNode*)tmpSceneNode);
		}
	}

}; // namespace end
