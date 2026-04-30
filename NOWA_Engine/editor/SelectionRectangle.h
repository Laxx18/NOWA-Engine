#ifndef SELECTIONRECTANGLE_H_
#define SELECTIONRECTANGLE_H_

#include "defines.h"

namespace NOWA
{
	class EXPORTED SelectionRectangle : public Ogre::ManualObject
	{
	public:
		SelectionRectangle(const Ogre::String& name, Ogre::SceneManager* sceneManager, Ogre::Camera* camera);

		virtual ~SelectionRectangle();

		void setCorners(Ogre::Real left, Ogre::Real top, Ogre::Real right, Ogre::Real bottom);

		void setCorners(const Ogre::Vector2& topLeft, const Ogre::Vector2& bottomRight);
	private:
		Ogre::Camera* camera;
	};

}; // namespace end

#endif
