#ifndef BOUNDING_BOX_DRAW_H
#define BOUNDING_BOX_DRAW_H

#include "defines.h"

namespace NOWA
{
	class EXPORTED BoundingBoxDraw
	{
	public:
		BoundingBoxDraw(Ogre::SceneManager* sceneManager);

		~BoundingBoxDraw();

		void add(Ogre::SceneNode* sceneNode, const Ogre::ColourValue& color);

		void toggle(Ogre::SceneNode* sceneNode, const Ogre::ColourValue& color);

		void remove(Ogre::SceneNode* sceneNode);

		bool isAdded(Ogre::SceneNode* sceneNode);

		void clear(void);

		void update(void);

		void add(const Ogre::AxisAlignedBox& aabb, const Ogre::ColourValue& color, int uniqueID);

		void remove(int uniqueID);
		
		void toggle(const Ogre::AxisAlignedBox& aabb, const Ogre::ColourValue& color, int uniqueID);

		bool isAdded(int uniqueID);

	private:
		Ogre::AxisAlignedBox aabbToAxisAlignedBox(Ogre::Aabb aabb);
		
		template<typename T>
		int indexOf(std::vector<T>* vector, T obj)
		{
			unsigned int i = 0;
			for (; i < vector->size(); i++)
			{
				if (obj == (*vector)[i])
					return i;
			}

			return -1;
		}

		template<typename T>
		void remove(std::vector<T>* vector, int listNum)
		{
			std::vector<T>::iterator del = vector->begin();
			del += listNum;
			vector->erase(del);
		}

		void fetchAABBs(std::vector<Ogre::AxisAlignedBox>* aabbs, Ogre::SceneNode* node);
	private:
		Ogre::ManualObject* manualObject;
		Ogre::SceneNode* node;
		std::vector<Ogre::IdType> targetSceneNodeIDs;
		std::vector<Ogre::ColourValue> targetColors;

		std::vector<Ogre::AxisAlignedBox> targetAABBs;
		std::vector<int> targetAABBIDs;
		std::vector<Ogre::ColourValue> targetAABBColors;
		Ogre::SceneManager* sceneManager;
	};

}; // namespace end

#endif
