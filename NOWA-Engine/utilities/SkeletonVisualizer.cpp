#include "NOWAPrecompiled.h"
#include "SkeletonVisualizer.h"

namespace NOWA
{

	SkeletonVisualizer::SkeletonVisualizer(Ogre::v1::Entity* entity, Ogre::SceneManager* sceneManager, Ogre::Camera* camera, Ogre::Real boneSize, Ogre::Real scaleAxes)
		: entity(entity),
		sceneManager(sceneManager),
		camera(camera),
		boneSize(boneSize),
		scaleAxes(scaleAxes),
		showAxes(true),
		showBones(true),
		showNames(true),
		axisDatablock(nullptr),
		boneDatablock(nullptr)
	{
#if 0
		this->createAxesMaterial();
		this->createBoneMaterial();
		this->createAxesMesh();
		this->createBoneMesh();

		Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[SkeletonVisualizer]: List all bones for mesh: '" + entity->getName() + "':");
		int numBones = this->entity->getSkeleton()->getNumBones();

		for (unsigned short int iBone = 0; iBone < numBones; ++iBone)
		{
			Ogre::v1::OldBone* bone = this->entity->getSkeleton()->getBone(iBone);
			if (!bone)
			{
				assert(false);
				continue;
			}

			Ogre::v1::Entity* currentEntity;
			Ogre::v1::TagPoint* tagPoint;

			// Absolutely HAVE to create bone representations first. Otherwise we would get the wrong child count
			// because an attached object counts as a child
			// Would be nice to have a function that only gets the children that are bones...
			unsigned short numChildren = bone->numChildren();
			if (numChildren == 0)
			{
				if (bone->getName() == "Bone.012" || bone->getName() == "Bone.013" || bone->getName() == "Bone.010")
				{
					continue;
				}
				// There are no children, but we should still represent the bone
				// Creates a bone of length 1 for leaf bones (bones without children)
				currentEntity = this->sceneManager->createEntity("SkeletonDebug/BoneMesh");
				// Will not work at the moment: See TagPointComponent for solution, but is a heavy one, because of x-nodes, are necessary here!
				tagPoint = this->entity->attachObjectToBone(bone->getName(), static_cast<Ogre::MovableObject*>(currentEntity));
				if (nullptr == tagPoint)
				{
					continue;
				}

				this->boneEntities.push_back(currentEntity);
				Ogre::Vector3 v = bone->getPosition();
				// If the length is zero, no point in creating the bone representation
				float length = v.length();
				if (length < 0.00001f)
				{
					continue;
				}

				tagPoint->setScale(length, length, length);
			}
			else
			{
				for (int i = 0; i < numChildren; ++i)
				{
					if (bone->getChild(i)->getName() == "Bone.012" || bone->getChild(i)->getName() == "Bone.013" || bone->getChild(i)->getName() == "Bone.010")
					{
						continue;
					}
					Ogre::Vector3 v = bone->getChild(i)->getPosition();
					// If the length is zero, no point in creating the bone representation
					float length = v.length();
					if (length < 0.00001f)
					{
						continue;
					}

					currentEntity = this->sceneManager->createEntity("SkeletonDebug/BoneMesh");
					tagPoint = this->entity->attachObjectToBone(bone->getName(), static_cast<Ogre::MovableObject*>(currentEntity));
					if (nullptr == tagPoint)
					{
						continue;
					}
					this->boneEntities.push_back(currentEntity);

					tagPoint->setScale(length, length, length);
				}
			}

			currentEntity = this->sceneManager->createEntity("SkeletonDebug/AxesMesh");
			tagPoint = this->entity->attachObjectToBone(bone->getName(), static_cast<Ogre::MovableObject*>(currentEntity));
			if (nullptr == tagPoint)
			{
				continue;
			}

			// Make sure we don't wind up with tiny/giant axes and that one axis doesnt get squashed
			tagPoint->setScale((this->scaleAxes * 10.0f / this->entity->getParentSceneNode()->getScale().x),
				(this->scaleAxes * 10.0f / this->entity->getParentSceneNode()->getScale().y),
				(this->scaleAxes * 10.0f / this->entity->getParentSceneNode()->getScale().z));
			this->axisEntities.push_back(currentEntity);

			Ogre::String name = this->entity->getName() + "SkeletonDebug/BoneText/Bone_";
			name += Ogre::StringConverter::toString(iBone);
			ObjectTextDisplay* objectTextDisplay = new ObjectTextDisplay(name, bone, this->camera, this->entity);
			objectTextDisplay->setEnabled(true);
			objectTextDisplay->setText(bone->getName());
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[SkeletonVisualizer] Bone name: " + bone->getName() 
				+ " position: " + Ogre::StringConverter::toString(bone->_getDerivedPosition()) 
				+ " orientation: " + Ogre::StringConverter::toString(bone->_getDerivedOrientation()));
			this->textOverlays.push_back(objectTextDisplay);
		}

		this->setShowAxes(true);
		this->setShowBones(true);
		this->setShowNames(true);
#endif
	}

	SkeletonVisualizer::~SkeletonVisualizer()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[SkeletonVisualizer] Destructor skeleton visualizer for entity: " + this->entity->getName());
		while (this->textOverlays.size() > 0)
		{
			ObjectTextDisplay* objectTextDisplay = this->textOverlays.back();
			delete objectTextDisplay;
			this->textOverlays.pop_back();
		}
	}

	void SkeletonVisualizer::update(Ogre::Real dt)
	{
		std::vector<ObjectTextDisplay*>::iterator it;
		for (it = this->textOverlays.begin(); it < this->textOverlays.end(); ++it)
		{
			(*it)->update(dt);
		}
	}

	void SkeletonVisualizer::setShowAxes(bool show)
	{
		// Don't change anything if we are already in the proper state
		if (this->showAxes == show)
		{
			return;
		}

		this->showAxes = show;

		std::vector<Ogre::v1::Entity*>::iterator it;
		for (it = this->axisEntities.begin(); it < this->axisEntities.end(); ++it)
		{
			(*it)->setVisible(show);
		}
	}

	void SkeletonVisualizer::setShowBones(bool show)
	{
		// Don't change anything if we are already in the proper state
		if (this->showBones == show)
		{
			return;
		}

		this->showBones = show;

		std::vector<Ogre::v1::Entity*>::iterator it;
		for (it = this->boneEntities.begin(); it < this->boneEntities.end(); ++it)
		{
			(*it)->setVisible(show);
		}

	}

	void SkeletonVisualizer::setShowNames(bool show)
	{
		// Don't change anything if we are already in the proper state
		if (this->showNames == show)
		{
			return;
		}

		this->showNames = show;

		std::vector<ObjectTextDisplay*>::iterator it;
		for (it = this->textOverlays.begin(); it < this->textOverlays.end(); ++it)
		{
			(*it)->setEnabled(show);
		}
	}

	void SkeletonVisualizer::createAxesMaterial()
	{
		Ogre::String matName = "SkeletonDebug/AxesMat";

		//this->axisMatPtr = Ogre::MaterialManager::getSingleton().getByName(matName);
		//if (this->axisMatPtr.isNull())
		//{
		//	this->axisMatPtr = Ogre::MaterialManager::getSingleton().create(matName, Ogre::ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME);

		//	Ogre::HlmsUnlit* hlmsUnlit = dynamic_cast<Ogre::HlmsUnlit*>(hlmsManager->getHlms(Ogre::HLMS_UNLIT));

		//	// First pass for axes that are partially within the model (shows transparency)
		//	Ogre::Pass* p = this->axisMatPtr->getTechnique(0)->getPass(0);
		//	p->setLightingEnabled(false);
		//	p->setPolygonModeOverrideable(false);
		//	p->setVertexColourTracking(Ogre::TVC_AMBIENT);
		//	p->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
		//	p->setCullingMode(Ogre::CULL_NONE);
		//	p->setDepthWriteEnabled(false);
		//	p->setDepthCheckEnabled(false);

		//	// Second pass for the portion of the axis that is outside the model (solid color)
		//	Ogre::Pass* p2 = this->axisMatPtr->getTechnique(0)->createPass();
		//	p2->setLightingEnabled(false);
		//	p2->setPolygonModeOverrideable(false);
		//	p2->setVertexColourTracking(Ogre::TVC_AMBIENT);
		//	p2->setCullingMode(Ogre::CULL_NONE);
		//	p2->setDepthWriteEnabled(false);
		//}

		Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();
		Ogre::HlmsUnlit* hlmsUnlit = dynamic_cast<Ogre::HlmsUnlit*>(hlmsManager->getHlms(Ogre::HLMS_UNLIT));

		// this->datablock = static_cast<Ogre::HlmsUnlitDatablock*>(hlmsUnlit->createDatablock("FadeEffect", "FadeEffect", Ogre::HlmsMacroblock(), Ogre::HlmsBlendblock(), Ogre::HlmsParamVec()));
		this->axisDatablock = dynamic_cast<Ogre::HlmsUnlitDatablock*>(hlmsManager->getDatablock("WhiteNoLighting"));
		if (nullptr == axisDatablock)
		{
			OGRE_EXCEPT(Ogre::Exception::ERR_ITEM_NOT_FOUND, "Material name 'WhiteNoLighting' cannot be found for 'SkeletonVisualizer'.", "SkeletonVisualizer::createAxesMaterial");
		}
		// Via code, or scene_blend alpha_blend in material
		/*	Ogre::HlmsBlendblock blendblock;
		blendblock.setBlendType(Ogre::SBT_TRANSPARENT_ALPHA);
		this->datablock->setBlendblock(blendblock);*/
		this->axisDatablock->setUseColour(true);
	}

	void SkeletonVisualizer::createBoneMaterial()
	{
		Ogre::String matName = "SkeletonDebug/BoneMat";

		/*this->boneMatPtr = Ogre::MaterialManager::getSingleton().getByName(matName);
		if (this->boneMatPtr.isNull())
		{
			this->boneMatPtr = Ogre::MaterialManager::getSingleton().create(matName, Ogre::ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME);

			Ogre::Pass* p = this->boneMatPtr->getTechnique(0)->getPass(0);
			p->setLightingEnabled(false);
			p->setPolygonModeOverrideable(false);
			p->setVertexColourTracking(Ogre::TVC_AMBIENT);
			p->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
			p->setCullingMode(Ogre::CULL_ANTICLOCKWISE);
			p->setDepthWriteEnabled(false);
			p->setDepthCheckEnabled(false);
		}*/

		Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();
		Ogre::HlmsUnlit* hlmsUnlit = dynamic_cast<Ogre::HlmsUnlit*>(hlmsManager->getHlms(Ogre::HLMS_UNLIT));

		// this->datablock = static_cast<Ogre::HlmsUnlitDatablock*>(hlmsUnlit->createDatablock("FadeEffect", "FadeEffect", Ogre::HlmsMacroblock(), Ogre::HlmsBlendblock(), Ogre::HlmsParamVec()));
		this->boneDatablock = dynamic_cast<Ogre::HlmsUnlitDatablock*>(hlmsManager->getDatablock("WhiteNoLighting"));
		if (nullptr == boneDatablock)
		{
			OGRE_EXCEPT(Ogre::Exception::ERR_ITEM_NOT_FOUND, "Material name 'WhiteNoLighting' cannot be found for 'SkeletonVisualizer'.", "SkeletonVisualizer::createBoneMaterial");
		}
		// Via code, or scene_blend alpha_blend in material
		/*	Ogre::HlmsBlendblock blendblock;
		blendblock.setBlendType(Ogre::SBT_TRANSPARENT_ALPHA);
		this->datablock->setBlendblock(blendblock);*/
		this->axisDatablock->setUseColour(true);
	}

	void SkeletonVisualizer::createBoneMesh()
	{
		Ogre::String meshName = "SkeletonDebug/BoneMesh";
		this->boneMeshPtr = Ogre::v1::MeshManager::getSingletonPtr()->getByName(meshName);
		if (this->boneMeshPtr.isNull())
		{
			Ogre::v1::ManualObject mo(0, &this->sceneManager->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), this->sceneManager);
			mo.begin(this->boneDatablock->getName().getFriendlyText());

			Ogre::Vector3 basepos[6] = {
				Ogre::Vector3(0.0f, 0.0f, 0.0f),
				Ogre::Vector3(this->boneSize, this->boneSize * 4.0f, this->boneSize),
				Ogre::Vector3(-this->boneSize, this->boneSize * 4.0f, this->boneSize),
				Ogre::Vector3(-this->boneSize, this->boneSize * 4.0f, -this->boneSize),
				Ogre::Vector3(this->boneSize, this->boneSize * 4.0f, -this->boneSize),
				Ogre::Vector3(0.0f, 1.0f, 0.0f),
			};

			// Two colours so that we can distinguish the sides of the bones (we don't use any lighting on the material)
			Ogre::ColourValue col = Ogre::ColourValue(0.5f, 0.5f, 0.5f, 1.0f);
			Ogre::ColourValue col1 = Ogre::ColourValue(0.6f, 0.6f, 0.6f, 1.0f);

			mo.position(basepos[0]);
			mo.colour(col);
			mo.position(basepos[2]);
			mo.colour(col);
			mo.position(basepos[1]);
			mo.colour(col);

			mo.position(basepos[0]);
			mo.colour(col1);
			mo.position(basepos[3]);
			mo.colour(col1);
			mo.position(basepos[2]);
			mo.colour(col1);

			mo.position(basepos[0]);
			mo.colour(col);
			mo.position(basepos[4]);
			mo.colour(col);
			mo.position(basepos[3]);
			mo.colour(col);

			mo.position(basepos[0]);
			mo.colour(col1);
			mo.position(basepos[1]);
			mo.colour(col1);
			mo.position(basepos[4]);
			mo.colour(col1);

			mo.position(basepos[1]);
			mo.colour(col1);
			mo.position(basepos[2]);
			mo.colour(col1);
			mo.position(basepos[5]);
			mo.colour(col1);

			mo.position(basepos[2]);
			mo.colour(col);
			mo.position(basepos[3]);
			mo.colour(col);
			mo.position(basepos[5]);
			mo.colour(col);

			mo.position(basepos[3]);
			mo.colour(col1);
			mo.position(basepos[4]);
			mo.colour(col1);
			mo.position(basepos[5]);
			mo.colour(col1);

			mo.position(basepos[4]);
			mo.colour(col);
			mo.position(basepos[1]);
			mo.colour(col);
			mo.position(basepos[5]);
			mo.colour(col);

			// indices
			mo.triangle(0, 1, 2);
			mo.triangle(3, 4, 5);
			mo.triangle(6, 7, 8);
			mo.triangle(9, 10, 11);
			mo.triangle(12, 13, 14);
			mo.triangle(15, 16, 17);
			mo.triangle(18, 19, 20);
			mo.triangle(21, 22, 23);

			mo.end();

			this->boneMeshPtr = mo.convertToMesh(meshName, Ogre::ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME);
		}
	}

	void SkeletonVisualizer::createAxesMesh()
	{
		Ogre::String meshName = "SkeletonDebug/AxesMesh";
		this->axesMeshPtr = Ogre::v1::MeshManager::getSingletonPtr()->getByName(meshName);
		if (this->axesMeshPtr.isNull())
		{
			Ogre::v1::ManualObject mo(0, &this->sceneManager->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), this->sceneManager);
			mo.begin(this->axisDatablock->getName().getFriendlyText());
			/* 3 axes, each made up of 2 of these (base plane = XY)
			*   .------------|\
			*   '------------|/
			*/
			mo.estimateVertexCount(7 * 2 * 3);
			mo.estimateIndexCount(3 * 2 * 3);
			Ogre::Quaternion quat[6];
			Ogre::ColourValue col[3];

			// x-axis
			quat[0] = Ogre::Quaternion::IDENTITY;
			quat[1].FromAxes(Ogre::Vector3::UNIT_X, Ogre::Vector3::NEGATIVE_UNIT_Z, Ogre::Vector3::UNIT_Y);
			col[0] = Ogre::ColourValue::Red;
			col[0].a = 0.3f;
			// y-axis
			quat[2].FromAxes(Ogre::Vector3::UNIT_Y, Ogre::Vector3::NEGATIVE_UNIT_X, Ogre::Vector3::UNIT_Z);
			quat[3].FromAxes(Ogre::Vector3::UNIT_Y, Ogre::Vector3::UNIT_Z, Ogre::Vector3::UNIT_X);
			col[1] = Ogre::ColourValue::Green;
			col[1].a = 0.3f;
			// z-axis
			quat[4].FromAxes(Ogre::Vector3::UNIT_Z, Ogre::Vector3::UNIT_Y, Ogre::Vector3::NEGATIVE_UNIT_X);
			quat[5].FromAxes(Ogre::Vector3::UNIT_Z, Ogre::Vector3::UNIT_X, Ogre::Vector3::UNIT_Y);
			col[2] = Ogre::ColourValue::Blue;
			col[2].a = 0.3f;

			Ogre::Vector3 basepos[7] = {
				// stalk
				Ogre::Vector3(0.0f, 0.05f, 0.0f),
				Ogre::Vector3(0.0f, -0.05f, 0.0f),
				Ogre::Vector3(0.7f, -0.05f, 0.0f),
				Ogre::Vector3(0.7f, 0.05f, 0.0f),
				// head
				Ogre::Vector3(0.7f, -0.15f, 0.0f),
				Ogre::Vector3(1.0f, 0.0f, 0.0f),
				Ogre::Vector3(0.7f, 0.15f, 0.0f)
			};


			// vertices
			// 6 arrows
			for (size_t i = 0; i < 6; ++i)
			{
				// 7 points
				for (size_t p = 0; p < 7; ++p)
				{
					Ogre::Vector3 pos = quat[i] * (basepos[p] * this->scaleAxes);
					mo.position(pos);
					mo.colour(col[i / 2]);
				}
			}

			// indices
			// 6 arrows
			for (unsigned int i = 0; i < 6; ++i)
			{
				unsigned int base = i * 7;
				mo.triangle(base + 0, base + 1, base + 2);
				mo.triangle(base + 0, base + 2, base + 3);
				mo.triangle(base + 4, base + 5, base + 6);
			}

			mo.end();

			this->axesMeshPtr = mo.convertToMesh(meshName, Ogre::ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME);
		}
	}

}; //namespace end NOWA