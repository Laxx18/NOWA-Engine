#include "NOWAPrecompiled.h"

#if 0

#include "OceanComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "camera/CameraManager.h"
#include "modules/WorkspaceModule.h"
#include "ocean/OgreHlmsOcean.h"
#include "ocean/OgreHlmsOceanDatablock.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	OceanComponent::OceanComponent()
		: GameObjectComponent(),
		ocean(nullptr),
		datablock(nullptr)
		/*diffuseColor(new Variant(OceanComponent::AttrDiffuseColor(), Ogre::Vector3::UNIT_SCALE, this->attributes)),
		specularColor(new Variant(OceanComponent::AttrSpecularColor(), Ogre::Vector3::UNIT_SCALE, this->attributes)),
		powerScale(new Variant(OceanComponent::AttrPowerScale(), 1.0f, this->attributes)),
		direction(new Variant(OceanComponent::AttrDirection(), Ogre::Vector3::ZERO, this->attributes)),
		affectParentNode(new Variant(OceanComponent::AttrAffectParentNode(), true, this->attributes)),
		castShadows(new Variant(OceanComponent::AttrCastShadows(), true, this->attributes)),
		showDummyEntity(new Variant(OceanComponent::AttrShowDummyEntity(), false, this->attributes))*/
	{
		
	}

	OceanComponent::~OceanComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[OceanComponent] Destructor ocean component for game object: " + this->gameObjectPtr->getName());
		if (nullptr != this->datablock)
		{
			Ogre::Hlms* hlmsUnlit = Ogre::Root::getSingletonPtr()->getHlmsManager()->getHlms(Ogre::HLMS_UNLIT);
			hlmsUnlit->destroyDatablock(this->datablock->getName());
			this->datablock = nullptr;
		}
		if (nullptr != this->ocean)
		{
			this->gameObjectPtr->getSceneNode()->detachObject(this->ocean);
			delete this->ocean;
			this->ocean = nullptr;
		}
	}

	bool OceanComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		/*if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Diffuse")
		{
			this->diffuseColor->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3::UNIT_SCALE));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Specular")
		{
			this->specularColor->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3::UNIT_SCALE));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PowerScale")
		{
			this->powerScale->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Direction")
		{
			this->direction->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3::ZERO));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AffectParentNode")
		{
			this->affectParentNode->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CastShadows")
		{
			this->castShadows->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}*/
		return true;
	}

	GameObjectCompPtr OceanComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		OceanCompPtr clonedCompPtr(boost::make_shared<OceanComponent>());

		
		/*clonedCompPtr->setDiffuseColor(this->diffuseColor->getVector3());
		clonedCompPtr->setSpecularColor(this->specularColor->getVector3());
		clonedCompPtr->setPowerScale(this->powerScale->getReal());
		clonedCompPtr->setDirection(this->direction->getVector3());
		clonedCompPtr->setAffectParentNode(this->affectParentNode->getBool());
		clonedCompPtr->setCastShadows(this->castShadows->getBool());
		clonedCompPtr->setShowDummyEntity(this->showDummyEntity->getBool());*/

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool OceanComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[OceanComponent] Init ocean component for game object: " + this->gameObjectPtr->getName());

		this->createOcean();
		return true;
	}

	void OceanComponent::update(Ogre::Real dt, bool notSimulating)
	{
		// if (false == notSimulating)
		{
			this->ocean->update();
		}
		
	}

	void OceanComponent::createOcean(void)
	{
		
// Attention: When camera changed event must be triggered, to set the new camera for the ocean
		if (nullptr == this->ocean)
		{
			// Register Ocean
			// https://forums.ogre3d.org/viewtopic.php?f=25&t=93592&hilit=ocean

			//Ogre::HlmsPbs* hlmsPbs = WorkspaceModule::getInstance()->getHlmsPbs();
			//Ogre::ArchiveVec libraryPbs = hlmsPbs->getPiecesLibraryAsArchiveVec();
			//Ogre::Archive* archivePbs = hlmsPbs->getDataFolder();

			//Ogre::String dataFolder = "../../media/";
			//// Note a the moment only OpenGL available
			//Ogre::Archive* archiveOcean = Ogre::ArchiveManager::getSingletonPtr()->load(dataFolder + "Hlms/Ocean/GLSL", "FileSystem", true);
			//Ogre::HlmsOcean* hlmsOcean = OGRE_NEW Ogre::HlmsOcean(archiveOcean, &libraryPbs);
			//WorkspaceModule::getInstance()->getHlmsManager()->registerHlms(hlmsOcean);
			//WorkspaceModule::getInstance()->setHlmsOcean(hlmsOcean);
			//hlmsOcean->setDebugOutputPath(false, false);

			//Ogre::Archive* archiveLibraryCustom = Ogre::ArchiveManager::getSingletonPtr()->load(dataFolder + "Hlms/Ocean/GLSL/Custom", "FileSystem", true);
			//libraryPbs.push_back(archiveLibraryCustom);
			//hlmsPbs->reloadFrom(archivePbs, &libraryPbs);

			/*
			when you have a reflection probe, set it
			Ogre::HlmsOcean* hlmsOcean = static_cast<Ogre::HlmsOcean*>( Ogre::Root::getSingletonPtr()->getHlmsManager()->getHlms( Ogre::HLMS_USER2 ) );
			hlmsOcean->setEnvProbe( probeTexture );
			*/


			//Create ocean
			this->ocean = new Ogre::Ocean(Ogre::Id::generateNewId<Ogre::MovableObject>(), &this->gameObjectPtr->getSceneManager()->_getEntityMemoryManager(Ogre::SCENE_STATIC),
				this->gameObjectPtr->getSceneManager(), 0, Ogre::Root::getSingletonPtr()->getCompositorManager2(),
				AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera());

			this->ocean->setCastShadows(false);
			this->ocean->setName(this->gameObjectPtr->getName());

			Ogre::Vector3 center = Ogre::Vector3::ZERO;
			Ogre::Vector2 size = Ogre::Vector2(200, 200);
			this->ocean->create(center, size);

			Ogre::TexturePtr probeTexture = Ogre::TextureManager::getSingletonPtr()->getByName("oceanData.dds");
			if (probeTexture.isNull())
			{
				probeTexture = Ogre::TextureManager::getSingleton().load("oceanData.dds", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_3D);
			}

			if (nullptr == this->datablock)
			{
				Ogre::HlmsOcean* hlmsOcean = WorkspaceModule::getInstance()->getHlmsOcean();
				hlmsOcean->setEnvProbe(probeTexture);

				Ogre::String datablockName = "Ocean";
				this->datablock = static_cast<Ogre::HlmsOceanDatablock*>(hlmsOcean->createDatablock(datablockName, datablockName, Ogre::HlmsMacroblock(),
					Ogre::HlmsBlendblock(), Ogre::HlmsParamVec()));

				// static_cast<Ogre::HlmsOceanDatablock*>(this->datablock)->setDeepColour
				// static_cast<Ogre::HlmsOceanDatablock*>(this->datablock)->setShallowColour
				// static_cast<Ogre::HlmsOceanDatablock*>(this->datablock)->setShallowColour
				// static_cast<Ogre::HlmsOceanDatablock*>(this->datablock)->setWavesScale

				this->ocean->setDatablock(this->datablock);
			}

			this->ocean->setStatic(true);
			this->gameObjectPtr->setDynamic(false);
			this->gameObjectPtr->getSceneNode()->attachObject(this->ocean);
			this->gameObjectPtr->setDynamic(false);

			this->gameObjectPtr->init(this->ocean);
			// Register after the component has been created
			AppStateManager::getSingletonPtr()->getGameObjectController()->registerGameObject(gameObjectPtr);
			// this->ocean->update();
		}
	}

	void OceanComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		/*if (OceanComponent::AttrDiffuseColor() == attribute->getName())
		{
			this->setDiffuseColor(attribute->getVector3());
		}
		else if (OceanComponent::AttrSpecularColor() == attribute->getName())
		{
			this->setSpecularColor(attribute->getVector3());
		}
		else if (OceanComponent::AttrPowerScale() == attribute->getName())
		{
			this->setPowerScale(attribute->getReal());
		}
		else if (OceanComponent::AttrDirection() == attribute->getName())
		{
			this->setDirection(attribute->getVector3());
		}
		else if (OceanComponent::AttrAffectParentNode() == attribute->getName())
		{
			this->setAffectParentNode(attribute->getBool());
		}
		else if (OceanComponent::AttrCastShadows() == attribute->getName())
		{
			this->setCastShadows(attribute->getBool());
		}*/
	}

	void OceanComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>*

		/*propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Diffuse"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->diffuseColor->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Specular"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->specularColor->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PowerScale"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->powerScale->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Direction"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->direction->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AffectParentNode"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->affectParentNode->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CastShadows"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->castShadows->getBool())));
		propertiesXML->append_node(propertyXML);*/
	}

	/*void OceanComponent::setActivated(bool activated)
	{
		if (nullptr != this->light)
		{
			this->light->setVisible(activated);
		}
	}*/

	Ogre::String OceanComponent::getClassName(void) const
	{
		return "OceanComponent";
	}

	Ogre::String OceanComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	/*void OceanComponent::setDiffuseColor(const Ogre::Vector3& diffuseColor)
	{
		this->diffuseColor->setValue(diffuseColor);
		if (nullptr != this->light)
		{
			this->light->setDiffuseColour(this->diffuseColor->getVector3().x, this->diffuseColor->getVector3().y, this->diffuseColor->getVector3().z);
		}
	}

	Ogre::Vector3 OceanComponent::getDiffuseColor(void) const
	{
		return this->diffuseColor->getVector3();
	}

	void OceanComponent::setSpecularColor(const Ogre::Vector3& specularColor)
	{
		this->specularColor->setValue(specularColor);
		if (nullptr != this->light)
		{
			this->light->setSpecularColour(this->specularColor->getVector3().x, this->specularColor->getVector3().y, this->specularColor->getVector3().z);
		}
	}

	Ogre::Vector3 OceanComponent::getSpecularColor(void) const
	{
		return this->specularColor->getVector3();
	}

	void OceanComponent::setPowerScale(Ogre::Real powerScale)
	{
		this->powerScale->setValue(powerScale);
		if (nullptr != this->light)
		{
			this->light->setPowerScale(this->powerScale->getReal());
		}
	}

	Ogre::Real OceanComponent::getPowerScale(void) const
	{
		return this->powerScale->getReal();
	}

	void OceanComponent::setDirection(const Ogre::Vector3& direction)
	{
		this->direction->setValue(direction);
		if (nullptr != this->light)
		{
			this->light->setDirection(this->direction->getVector3());
		}
	}

	Ogre::Vector3 OceanComponent::getDirection(void) const
	{
		return this->direction->getVector3();
	}

	void OceanComponent::setAffectParentNode(bool affectParentNode)
	{
		this->affectParentNode->setValue(affectParentNode);
		if (nullptr != this->light)
		{
			this->light->setAffectParentNode(this->affectParentNode->getBool());
		}
	}

	bool OceanComponent::getAffectParentNode(void) const
	{
		return this->affectParentNode->getBool();
	}

	void OceanComponent::setCastShadows(bool castShadows)
	{
		this->castShadows->setValue(castShadows);
		if (nullptr != this->light)
		{
			this->light->setCastShadows(this->castShadows->getBool());
		}
	}

	bool OceanComponent::getCastShadows(void) const
	{
		return this->castShadows->getBool();
	}

	void OceanComponent::setShowDummyEntity(bool showDummyEntity)
	{
		this->showDummyEntity->setValue(showDummyEntity);
	}

	bool OceanComponent::getShowDummyEntity(void) const
	{
		return 	this->showDummyEntity->getBool();
	}*/

	Ogre::Ocean* OceanComponent::getOgreOcean(void) const
	{
		return this->ocean;
	}

}; // namespace end

#endif