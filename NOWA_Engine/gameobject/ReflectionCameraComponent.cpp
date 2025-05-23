#include "NOWAPrecompiled.h"
#include "ReflectionCameraComponent.h"
#include "GameObjectController.h"
#include "WorkspaceComponents.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	ReflectionCameraComponent::ReflectionCameraComponent()
		: GameObjectComponent(),
		camera(nullptr),
		dummyEntity(nullptr),
		hideEntity(false),
		workspaceBaseComponent(nullptr),
		nearClipDistance(new Variant(ReflectionCameraComponent::AttrNearClipDistance(), 0.5f, this->attributes)),
		farClipDistance(new Variant(ReflectionCameraComponent::AttrFarClipDistance(), 100.0f, this->attributes)),
		fovy(new Variant(ReflectionCameraComponent::AttrFovy(), 90.0f, this->attributes)),
		orthographic(new Variant(ReflectionCameraComponent::AttrOrthographic(), false, this->attributes)),
		fixedYawAxis(new Variant(ReflectionCameraComponent::AttrFixedYawAxis(), false, this->attributes)),
		cubeTextureSize(new Variant(ReflectionCameraComponent::AttrCubeTextureSize(), { "256", "512", "1024" }, this->attributes))
	{
		this->fovy->setDescription("Field Of View (FOV) is the angle made between the frustum's position, and the edges "
			"of the 'screen' onto which the scene is projected.High values(90 + degrees) result in a wide - angle, "
			"fish - eye kind of view, low values(30 - degrees) in a stretched, telescopic kind of view.Typical values "
			"are between 45 and 60 degrees.");

		this->fixedYawAxis->setDescription("Tells the camera whether to yaw around it's own local Y axis or a "
			"fixed axis of choice. This method allows you to change the yaw behaviour of the camera "
			"- by default, the camera yaws around a fixed Y axis.This is "
			"often what you want - for example if you're making a first-person "
			"shooter, you really don't want the yaw axis to reflect the local "
			"camera Y, because this would mean a different yaw axis if the "
			"player is looking upwards rather than when they are looking "
			"straight ahead.You can change this behaviour by calling this "
			"method, which you will want to do if you are making a completely "
			"free camera like the kind used in a flight simulator.");

		this->cubeTextureSize->setListSelectedValue("256");
		this->cubeTextureSize->setDescription("Sets the cube map texture size. The higher the size the more accurate the rendering, but at a huge performance price. "
											  "Note: A size below 1024 will show ugly artefacts at the borders, but is more performant.");
	}

	ReflectionCameraComponent::~ReflectionCameraComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ReflectionCameraComponent] Destructor reflection camera component for game object: " + this->gameObjectPtr->getName());
	}

	bool ReflectionCameraComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NearClipDistance")
		{
			this->setNearClipDistance(XMLConverter::getAttribReal(propertyElement, "data", 0.01f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FarClipDistance")
		{
			this->setFarClipDistance(XMLConverter::getAttribReal(propertyElement, "data", 100.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Fovy")
		{
			this->setFovy(Ogre::Degree(XMLConverter::getAttribReal(propertyElement, "data", 90.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Orthographic")
		{
			this->setOrthographic(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FixedYawAxis")
		{
			this->fixedYawAxis->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CubeTextureSize")
		{
			this->cubeTextureSize->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr ReflectionCameraComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool ReflectionCameraComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ReflectionCameraComponent] Init reflection camera component for game object: " + this->gameObjectPtr->getName());

		this->createCamera();
		return true;
	}

	void ReflectionCameraComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();
	
		if (nullptr != this->camera)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("ReflectionCameraComponent::onRemoveComponent",
			{
				this->gameObjectPtr->getSceneNode()->detachObject(this->camera);
				this->gameObjectPtr->getSceneManager()->destroyMovableObject(this->camera);
				this->camera = nullptr;
				this->dummyEntity = nullptr;
			});
		}

		if (nullptr != this->workspaceBaseComponent && false == AppStateManager::getSingletonPtr()->getIsShutdown())
		{
			this->workspaceBaseComponent->setUseReflection(false);
		}
	}

	void ReflectionCameraComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			if (nullptr != dummyEntity && true == this->hideEntity)
			{
				if (true == this->dummyEntity->isVisible())
				{
					ENQUEUE_RENDER_COMMAND("ReflectionCameraComponent::update",
					{
						this->dummyEntity->setVisible(false);
					});
				}
				else
				{
					// If its not the active camera
					if (false == this->dummyEntity->isVisible())
					{
						ENQUEUE_RENDER_COMMAND("ReflectionCameraComponent::update",
						{
							this->dummyEntity->setVisible(true);
						});
					}
				}
			}
		}
	}

	void ReflectionCameraComponent::createCamera(void)
	{
		if (nullptr == this->camera)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("ReflectionCameraComponent::createCamera",
			{
				this->camera = this->gameObjectPtr->getSceneManager()->createCamera(this->gameObjectPtr->getName(), true, true);

				Ogre::SceneNode * cameraParentNode = this->camera->getParentSceneNode();
				this->camera->detachFromParent();
				this->gameObjectPtr->getSceneNode()->attachObject(this->camera);

				// As attribute? because it should be set to false, when several viewports are used
				// this->camera->setAutoAspectRatio(true);
				this->camera->setNearClipDistance(this->nearClipDistance->getReal());
				this->camera->setFarClipDistance(this->farClipDistance->getReal());
				this->camera->setFOVy(Ogre::Degree(this->fovy->getReal()));
				this->camera->setFixedYawAxis(this->fixedYawAxis->getBool());
				this->camera->setProjectionType(static_cast<Ogre::ProjectionType>(this->orthographic->getBool() == true ? 0 : 1));
				this->camera->setQueryFlags(0 << 0);

				// Borrow the entity from the game object
				this->dummyEntity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
				if (nullptr != this->dummyEntity)
				{
					this->dummyEntity->setCastShadows(false);
				}
			});
		}
	}

	void ReflectionCameraComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (ReflectionCameraComponent::AttrNearClipDistance() == attribute->getName())
		{
			this->setNearClipDistance(attribute->getReal());
		}
		else if (ReflectionCameraComponent::AttrFarClipDistance() == attribute->getName())
		{
			this->setFarClipDistance(attribute->getReal());
		}
		else if (ReflectionCameraComponent::AttrFovy() == attribute->getName())
		{
			this->setFovy(Ogre::Degree(attribute->getReal()));
		}
		else if (ReflectionCameraComponent::AttrOrthographic() == attribute->getName())
		{
			this->setOrthographic(attribute->getBool());
		}
		else if (ReflectionCameraComponent::AttrFixedYawAxis() == attribute->getName())
		{
			this->setFixedYawAxis(attribute->getBool());
		}
		else if (ReflectionCameraComponent::AttrCubeTextureSize() == attribute->getName())
		{
			this->setCubeTextureSize(attribute->getListSelectedValue());
		}
	}

	void ReflectionCameraComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "NearClipDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->nearClipDistance->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FarClipDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->farClipDistance->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Fovy"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fovy->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Orthographic"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->orthographic->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FixedYawAxis"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fixedYawAxis->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CubeTextureSize"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->cubeTextureSize->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String ReflectionCameraComponent::getClassName(void) const
	{
		return "ReflectionCameraComponent";
	}

	Ogre::String ReflectionCameraComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void ReflectionCameraComponent::setNearClipDistance(Ogre::Real nearClipDistance)
	{
		this->nearClipDistance->setValue(nearClipDistance);
		if (nullptr != this->camera)
		{
			ENQUEUE_RENDER_COMMAND_MULTI("ReflectionCameraComponent::setNearClipDistance", _1(nearClipDistance),
			{
				this->camera->setNearClipDistance(nearClipDistance);
			});
		}
	}

	Ogre::Real ReflectionCameraComponent::getNearClipDistance(void) const
	{
		return this->nearClipDistance->getReal();
	}

	void ReflectionCameraComponent::setFarClipDistance(Ogre::Real farClipDistance)
	{
		this->farClipDistance->setValue(farClipDistance);
		if (nullptr != this->camera)
		{
			ENQUEUE_RENDER_COMMAND_MULTI("ReflectionCameraComponent::setFarClipDistance", _1(farClipDistance),
			{
				this->camera->setFarClipDistance(farClipDistance);
			});
		}
	}

	Ogre::Real ReflectionCameraComponent::getFarClipDistance(void) const
	{
		return this->farClipDistance->getReal();
	}

	void ReflectionCameraComponent::setFovy(const Ogre::Degree& fovy)
	{
		this->fovy->setValue(fovy.valueDegrees());
		if (nullptr != this->camera)
		{
			ENQUEUE_RENDER_COMMAND_MULTI("ReflectionCameraComponent::setFovy", _1(fovy),
			{
				this->camera->setFOVy(fovy);
			});
		}
	}

	Ogre::Degree ReflectionCameraComponent::getFovy(void) const
	{
		return Ogre::Degree(this->fovy->getReal());
	}

	void ReflectionCameraComponent::setOrthographic(bool orthographic)
	{
		this->orthographic->setValue(orthographic);
		if (nullptr != this->camera)
		{
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("ReflectionCameraComponent::setOrthographic", _1(orthographic),
			{
				this->camera->setProjectionType(static_cast<Ogre::ProjectionType>(this->orthographic->getBool() == true ? 0 : 1));
				if (true == orthographic)
				{
					this->camera->setOrthoWindowHeight(1.0f);
				}
			});
		}
	}

	bool ReflectionCameraComponent::getIsOrthographic(void) const
	{
		return this->orthographic->getBool();
	}

	Ogre::Camera* ReflectionCameraComponent::getCamera(void) const
	{
		return this->camera;
	}

	void ReflectionCameraComponent::setFixedYawAxis(bool fixedYawAxis)
	{
		this->fixedYawAxis->setValue(fixedYawAxis);
		if (nullptr != this->camera)
		{
			ENQUEUE_RENDER_COMMAND_MULTI("ReflectionCameraComponent::setFixedYawAxis", _1(fixedYawAxis),
			{
				this->camera->setFixedYawAxis(fixedYawAxis);
			});
		}
	}

	bool ReflectionCameraComponent::getFixedYawAxis(void) const
	{
		return this->fixedYawAxis->getBool();
	}

	void ReflectionCameraComponent::setCubeTextureSize(Ogre::String& cubeTextureSize)
	{
		this->cubeTextureSize->setListSelectedValue(cubeTextureSize);

		if (nullptr != this->workspaceBaseComponent)
		{
			this->workspaceBaseComponent->createWorkspace();
		}
	}

	Ogre::String ReflectionCameraComponent::getCubeTextureSize(void) const
	{
		return this->cubeTextureSize->getListSelectedValue();
	}

}; // namespace end