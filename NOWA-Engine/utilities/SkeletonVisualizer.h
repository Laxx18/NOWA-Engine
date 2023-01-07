#ifndef SKELETON_VISUALIZER_H
#define SKELETON_VISUALIZER_H

#include "defines.h"
#include <OgreTagPoint.h>
#include <vector>
#include "utilities/ObjectTextDisplay.h"

// Depends on text display, which has not been ported yet

namespace NOWA
{
	class Ogre::SceneManager;
	class Ogre::Camera;
	class Ogre::SceneNode;
	class Ogre::v1::Entity;
	class Ogre::v1::OldBone;

	class EXPORTED SkeletonVisualizer
	{
	public:
		SkeletonVisualizer(Ogre::v1::Entity* entity, Ogre::SceneManager* sceneManager, Ogre::Camera* camera, Ogre::Real boneSize = 0.0025f, Ogre::Real scaleAxes = 0.1f);

		~SkeletonVisualizer();

		void setAxesScale(Ogre::Real scaleAxes) { this->scaleAxes = scaleAxes; }
		Ogre::Real getAxesScale() { return this->scaleAxes; }

		void setShowAxes(bool show);
		void setShowNames(bool show);
		void setShowBones(bool show);
		bool axesShown() { return this->showAxes; }
		bool namesShown() { return this->showNames; }
		bool bonesShown() { return this->showBones; }

		void update(Ogre::Real dt);
	private:
		void createAxesMaterial();
		void createBoneMaterial();
		void createAxesMesh();
		void createBoneMesh();

	private:

		std::vector<Ogre::v1::Entity*> axisEntities;
		std::vector<Ogre::v1::Entity*> boneEntities;
		std::vector<ObjectTextDisplay*> textOverlays;

		Ogre::Real boneSize;

		Ogre::v1::Entity* entity;
		Ogre::HlmsUnlitDatablock* axisDatablock;
		Ogre::HlmsUnlitDatablock* boneDatablock;
		Ogre::v1::MeshPtr boneMeshPtr;
		Ogre::v1::MeshPtr axesMeshPtr;
		Ogre::SceneManager* sceneManager;
		Ogre::Camera* camera;

		Ogre::Real scaleAxes;

		bool showAxes;
		bool showBones;
		bool showNames;
	};

};  //namespace end

#endif