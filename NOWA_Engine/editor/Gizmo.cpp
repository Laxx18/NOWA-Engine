#include "NOWAPrecompiled.h"
#include "Gizmo.h"

#include "utilities/MathHelper.h"
#include "main/Core.h"
#include "main/AppStateManager.h"
#include "modules/RenderCommandQueueModule.h"

// Debug test:
#include "MyGUI.h"

namespace
{
	struct colourMap
	{
		int key;
		Ogre::ColourValue value;
	} colourMap[10] =
	{
		{ 0, Ogre::ColourValue(1.0f, 1.0f, 0.0f, 0.5f) },
		{ 1, Ogre::ColourValue(1.0f, 1.0f, 0.0f, 1.0f) },
		{ 2, Ogre::ColourValue(1.0f, 0.0f, 0.0f, 1.0f) },
		{ 3, Ogre::ColourValue(0.0f, 1.0f, 0.0f, 1.0f) },
		{ 4, Ogre::ColourValue(0.0f, 0.0f, 1.0f, 1.0f) },
		{ 5, Ogre::ColourValue(1.0f, 0.0f, 1.0f, 1.0f) },
		{ 6, Ogre::ColourValue(0.0f, 1.0f, 1.0f, 1.0f) },
		{ 7, Ogre::ColourValue(0.5f, 0.5f, 0.0f, 1.0f) },
		{ 8, Ogre::ColourValue(0.0f, 0.5f, 0.5f, 1.0f) },
		{ 8, Ogre::ColourValue(0.5f, 0.0f, 0.5f, 1.0f) }
	};
	
}

namespace NOWA
{

	Gizmo::Gizmo()
		: sceneManager(nullptr),
		camera(nullptr),
		thickness(0.6f),
		arrowNodeX(nullptr),
		arrowNodeY(nullptr),
		arrowNodeZ(nullptr),
		sphereNode(nullptr),
		selectNode(nullptr),
		arrowEntityX(nullptr),
		arrowEntityY(nullptr),
		arrowEntityZ(nullptr),
		sphereEntity(nullptr),
		defaultCategory(0 << 0),
		debugPlane(false),
		translationLine(nullptr),
		translationLineNode(nullptr),
		rotationCircle(nullptr),
		rotationCircleNode(nullptr),
		translationCaption(nullptr),
		rotationCaption(nullptr),
		firstPlaneEntity(nullptr),
		firstPlaneNode(nullptr),
		secondPlaneEntity(nullptr),
		secondPlaneNode(nullptr),
		thirdPlaneEntity(nullptr),
		thirdPlaneNode(nullptr),
		gizmoState(GIZMO_NONE),
		constraintAxis(Ogre::Vector3::ZERO)
	{
		
	}

	Gizmo::~Gizmo()
	{
		if (this->arrowNodeX != nullptr)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("Gizmo::~Gizmo",
			{
				this->arrowNodeX->detachAllObjects();
				this->arrowNodeY->detachAllObjects();
				this->arrowNodeZ->detachAllObjects();
				this->sphereNode->detachAllObjects();
				this->selectNode->detachAllObjects();
				if (true == debugPlane)
				{
					this->firstPlaneNode->detachAllObjects();
					this->secondPlaneNode->detachAllObjects();
				}
				if (nullptr != this->arrowEntityX)
				{
					this->sceneManager->destroyEntity(this->arrowEntityX);
				}
				if (nullptr != this->arrowEntityY)
				{
					this->sceneManager->destroyEntity(this->arrowEntityY);
				}
				if (nullptr != this->arrowEntityZ)
				{
					this->sceneManager->destroyEntity(this->arrowEntityZ);
				}
				if (nullptr != this->sphereEntity)
				{
					this->sceneManager->destroyEntity(this->sphereEntity);
				}
				if (true == debugPlane)
				{
					NOWA::RenderCommandQueueModule::getInstance()->removeTrackedNode(this->firstPlaneNode);
					NOWA::RenderCommandQueueModule::getInstance()->removeTrackedNode(this->secondPlaneNode);
					NOWA::RenderCommandQueueModule::getInstance()->removeTrackedNode(this->thirdPlaneNode);

					this->sceneManager->destroyEntity(this->firstPlaneEntity);
					this->sceneManager->destroyEntity(this->secondPlaneEntity);
					this->sceneManager->destroyEntity(this->thirdPlaneEntity);
					this->sceneManager->destroySceneNode(this->firstPlaneNode);
					this->sceneManager->destroySceneNode(this->secondPlaneNode);
					this->sceneManager->destroySceneNode(this->thirdPlaneNode);
				}

				NOWA::RenderCommandQueueModule::getInstance()->removeTrackedNode(this->arrowNodeX);
				NOWA::RenderCommandQueueModule::getInstance()->removeTrackedNode(this->arrowNodeY);
				NOWA::RenderCommandQueueModule::getInstance()->removeTrackedNode(this->arrowNodeZ);
				NOWA::RenderCommandQueueModule::getInstance()->removeTrackedNode(this->sphereNode);
				NOWA::RenderCommandQueueModule::getInstance()->removeTrackedNode(this->selectNode);

				this->sceneManager->destroySceneNode(this->arrowNodeX);
				this->sceneManager->destroySceneNode(this->arrowNodeY);
				this->sceneManager->destroySceneNode(this->arrowNodeZ);
				this->sceneManager->destroySceneNode(this->sphereNode);
				this->sceneManager->destroySceneNode(this->selectNode);
				this->destroyLine();
				this->destroyCircle();
				if (nullptr != this->translationCaption)
				{
					delete this->translationCaption;
					this->translationCaption = nullptr;
				}
				if (nullptr != this->rotationCaption)
				{
					delete this->rotationCaption;
					this->rotationCaption = nullptr;
				}
			});
		}
	}

	void Gizmo::init(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, Ogre::Real thickness,
		const Ogre::String& materialNameX, const Ogre::String& materialNameY, const Ogre::String& materialNameZ, const Ogre::String& materialNameHighlight)
	{
		this->sceneManager = sceneManager;
		this->camera = camera;
		this->thickness = thickness;
		this->materialNameX = materialNameX;
		this->materialNameY = materialNameY;
		this->materialNameZ = materialNameZ;
		this->materialNameHighlight = materialNameHighlight;
		// Register default category for gizmo
		this->defaultCategory = AppStateManager::getSingletonPtr()->getGameObjectController()->registerCategory("Default");

		ENQUEUE_RENDER_COMMAND_WAIT("Gizmo::init",{

			// Create the three arrows in x-y-z axes
			// This is the main node for selected objects, at which the gizmo will be placed
			this->selectNode = this->sceneManager->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
			this->selectNode->setName("SelectNode");

			this->arrowNodeX = this->selectNode->createChildSceneNode(Ogre::SCENE_DYNAMIC);
			this->arrowNodeX->setOrientation(Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Y));

			this->arrowNodeY = this->selectNode->createChildSceneNode(Ogre::SCENE_DYNAMIC);
			this->arrowNodeY->setOrientation(Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::NEGATIVE_UNIT_X));

			this->arrowNodeZ = this->selectNode->createChildSceneNode(Ogre::SCENE_DYNAMIC);

			// Sphere remains always the same, so just set all settings here
			this->sphereNode = this->selectNode->createChildSceneNode(Ogre::SCENE_DYNAMIC);
			this->sphereNode->setName("SphereGizmoNode");
			this->sphereEntity = this->sceneManager->createEntity("gizmosphere.mesh");
			this->sphereEntity->setName("SphereGizmoEntity");
			this->sphereEntity->setDatablock("BaseWhiteLine");
			// this->sphereEntity->setQueryFlags(this->categorySphere);
			// Not ported

			this->sphereEntity->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);

			this->sphereEntity->setCastShadows(false);
			this->sphereNode->attachObject(this->sphereEntity);
			this->sphereNode->setVisible(false);

			this->firstPlane = Ogre::Plane(Ogre::Vector3::UNIT_X, this->selectNode->_getDerivedPositionUpdated());
			this->secondPlane = Ogre::Plane(Ogre::Vector3::UNIT_Y, this->selectNode->_getDerivedPositionUpdated());
			this->thirdPlane = Ogre::Plane(Ogre::Vector3::UNIT_Z, this->selectNode->_getDerivedPositionUpdated());

			this->createLine();
			this->createCircle();

			if (true == debugPlane)
			{
				this->firstPlaneEntity = this->sceneManager->createEntity(Ogre::SceneManager::PrefabType::PT_PLANE);
				this->firstPlaneEntity->setName("FirstPlaneEntity");
				// this->firstPlaneEntity->setDebugDisplayEnabled(true);
	// Here with macroblocks!
				this->firstPlaneNode = this->sceneManager->getRootSceneNode()->createChildSceneNode();
				this->firstPlaneNode->attachObject(this->firstPlaneEntity);
				this->firstPlaneEntity->setCastShadows(false);
				this->firstPlaneNode->setVisible(false);
				this->firstPlaneEntity->setQueryFlags(0 << 0);
				this->secondPlaneEntity = this->sceneManager->createEntity(Ogre::SceneManager::PrefabType::PT_PLANE);
				this->secondPlaneEntity->setName("SecondPlaneEntity");
				// this->secondPlaneEntity->setDebugDisplayEnabled(true);
				this->secondPlaneNode = this->sceneManager->getRootSceneNode()->createChildSceneNode();
				this->secondPlaneNode->attachObject(this->secondPlaneEntity);
				this->secondPlaneEntity->setCastShadows(false);
				this->secondPlaneNode->setVisible(false);
				this->secondPlaneEntity->setQueryFlags(0 << 0);
				this->thirdPlaneEntity = this->sceneManager->createEntity(Ogre::SceneManager::PrefabType::PT_PLANE);
				this->thirdPlaneEntity->setName("ThirdPlaneEntity");
				// this->thirdPlaneEntity->setDebugDisplayEnabled(true);
				this->thirdPlaneNode = this->sceneManager->getRootSceneNode()->createChildSceneNode();
				this->thirdPlaneNode->attachObject(this->thirdPlaneEntity);
				this->thirdPlaneEntity->setCastShadows(false);
				this->thirdPlaneNode->setVisible(false);
				this->thirdPlaneEntity->setQueryFlags(0 << 0);

			}
			this->enabled = false;

			this->setEnabled(false);
		});
	}

	void Gizmo::update(void)
	{
		// TODO: Interpolation queue command?
		ENQUEUE_RENDER_COMMAND("Gizmo::update",
		{
			if (true == this->enabled)
			{
				this->translationLineNode->setPosition(this->getPosition());
				//update the size of the gizmo depending on the distance from the gizmo to the camera
				Ogre::Vector3 nodePos = this->sphereNode->_getDerivedPositionUpdated();

				Ogre::Vector3 camPos = this->camera->getDerivedPosition();

				Ogre::Real camDistance = 0.0;
				if (this->camera->getProjectionType() == Ogre::PT_PERSPECTIVE)
				{
					camDistance = nodePos.distance(this->camera->getDerivedPosition());
					//camDistance = (nodepos - campos).length();

					//Ogre::Vector3 camVec = nodepos - campos;
					//camVec.normalise();
					/*Ogre::Radian angle = Ogre::Radian(Ogre::Math::ACos((pCamera->getDirection().dotProduct(camVec)))
					 / (pCamera->getDirection().length() * camVec.length()));

					 camDistance = Ogre::Math::Cos(angle) * camVec.length();
					 QMessageBox::information(nullptr, "ok", QString("v: %1 a: %2").arg((float)camDistance).arg((float)angle.valueDegrees()));
					 //camDistance = 1;*/
				}
				else
				{
					camDistance = nodePos.distance(Ogre::Vector3(0, this->camera->getOrthoWindowHeight(), 0));
				}

				Ogre::Real distance = 0.0;
				Ogre::Real fovy = this->camera->getFOVy().valueRadians();
				// Attention: Is this correct?
				if (this->arrowEntityX != nullptr && this->sceneManager->hasMovableObject(this->arrowEntityX))
				{
					if (this->arrowEntityX->getMesh()->getName() != "Ring.mesh")
					{
						distance = camDistance * 0.03f;
						Ogre::Vector3 amount(distance * this->thickness * fovy, distance * this->thickness * fovy, distance * fovy);
						 this->arrowNodeX->setScale(amount);
						 this->arrowNodeY->setScale(amount);
						 this->arrowNodeZ->setScale(amount);
					}
					else
					{
						distance = camDistance * 0.03f;
						Ogre::Vector3 amount(distance * fovy, distance * this->thickness * fovy, distance * fovy);
						 this->arrowNodeX->setScale(distance * fovy, distance * this->thickness * fovy, distance * fovy);
						 this->arrowNodeY->setScale(distance * fovy, distance * this->thickness * fovy, distance * fovy);
						 this->arrowNodeZ->setScale(distance * fovy, distance * this->thickness * fovy, distance * fovy);
					}
				}
				distance = camDistance * 0.03f;
				Ogre::Vector3 amount(distance * fovy * 0.5f, distance * fovy * 0.5f, distance * fovy * 0.85f);
				this->sphereNode->setScale(distance * fovy * 0.5f, distance * fovy * 0.5f, distance * fovy * 0.85f);
			}
		});
	}

	void Gizmo::redefineFirstPlane(const Ogre::Vector3& normal, const Ogre::Vector3& position)
	{
		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("Gizmo::redefineFirstPlane", _2(normal, position),
		{
			this->firstPlane.redefine(normal, position);
			if (true == debugPlane)
			{
				this->firstPlaneNode->setPosition(position);
				this->firstPlaneNode->lookAt(position + normal, Ogre::Node::TS_WORLD);
			}
		});
	}

	void Gizmo::redefineSecondPlane(const Ogre::Vector3& normal, const Ogre::Vector3& position)
	{
		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("Gizmo::redefineSecondPlane", _2(normal, position),
		{
			this->secondPlane.redefine(normal, position);
			if (true == debugPlane)
			{
				this->secondPlaneNode->setPosition(position);
				this->secondPlaneNode->lookAt(position + normal, Ogre::Node::TS_WORLD);
			}
		});
	}

	void Gizmo::redefineThirdPlane(const Ogre::Vector3& normal, const Ogre::Vector3& position)
	{
		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("Gizmo::redefineThirdPlane", _2(normal, position),
		{
			this->thirdPlane.redefine(normal, position);
			if (true == debugPlane)
			{
				this->thirdPlaneNode->setPosition(position);
				this->thirdPlaneNode->lookAt(position + normal, Ogre::Node::TS_WORLD);
			}
		});
	}

	void Gizmo::_debugShowResultPlane(unsigned char planeId)
	{
		if (true == debugPlane)
		{
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("Gizmo::_debugShowResultPlane", _1(planeId),
			{
				if (1 == planeId)
				{
					this->firstPlaneNode->setVisible(true);
					this->firstPlaneNode->setPosition(this->selectNode->_getDerivedPosition());
					this->firstPlaneNode->lookAt(this->selectNode->_getDerivedPosition() + this->firstPlane.normal, Ogre::Node::TS_WORLD);
					this->secondPlaneNode->setVisible(false);
					this->thirdPlaneNode->setVisible(false);
				}
				else if (2 == planeId)
				{
					this->secondPlaneNode->setVisible(true);
					this->secondPlaneNode->setPosition(this->selectNode->_getDerivedPosition());
					this->secondPlaneNode->lookAt(this->selectNode->_getDerivedPosition() + this->firstPlane.normal, Ogre::Node::TS_WORLD);
					this->firstPlaneNode->setVisible(false);
					this->thirdPlaneNode->setVisible(false);
				}
				else if (3 == planeId)
				{
					this->thirdPlaneNode->setVisible(true);
					this->thirdPlaneNode->setPosition(this->selectNode->_getDerivedPosition());
					this->thirdPlaneNode->lookAt(this->selectNode->_getDerivedPosition() + this->firstPlane.normal, Ogre::Node::TS_WORLD);
					this->firstPlaneNode->setVisible(false);
					this->secondPlaneNode->setVisible(false);
				}
			});
		}
	}

	void Gizmo::changeToTranslateGizmo(void)
	{
		ENQUEUE_RENDER_COMMAND_WAIT("Gizmo::changeToTranslateGizmo",
		{

			// Change the look of the gizmo
			if (this->arrowEntityX != nullptr && this->sceneManager->hasMovableObject(this->arrowEntityX))
			{
				this->sceneManager->destroyEntity(this->arrowEntityX);
			}
			this->arrowEntityX = this->sceneManager->createEntity("Arrow.mesh");
			this->arrowEntityX->setName("XArrowGizmoEntity");
			this->arrowEntityX->setDatablock(this->materialNameX);
			this->arrowEntityX->setQueryFlags(this->defaultCategory);
			// Not ported

			this->arrowEntityX->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);

			this->arrowEntityX->setCastShadows(false);
			this->arrowNodeX->attachObject(this->arrowEntityX);

			if (this->arrowEntityY != nullptr && this->sceneManager->hasMovableObject(this->arrowEntityY))
			{
				this->sceneManager->destroyEntity(this->arrowEntityY);
			}
			this->arrowEntityY = this->sceneManager->createEntity("Arrow.mesh");
			this->arrowEntityY->setName("YArrowGizmoEntity");

			this->arrowEntityY->setDatablock(this->materialNameY);
			this->arrowEntityY->setQueryFlags(this->defaultCategory);
			// Not ported

			this->arrowEntityY->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);

			this->arrowEntityY->setCastShadows(false);
			this->arrowNodeY->setOrientation(Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::NEGATIVE_UNIT_X));
			this->arrowNodeY->attachObject(this->arrowEntityY);

			if (this->arrowEntityZ != nullptr && this->sceneManager->hasMovableObject(this->arrowEntityZ))
			{
				this->sceneManager->destroyEntity(this->arrowEntityZ);
			}
			this->arrowEntityZ = this->sceneManager->createEntity("Arrow.mesh");
			this->arrowEntityZ->setName("ZArrowGizmoEntity");
			this->arrowEntityZ->setDatablock(this->materialNameZ);
			this->arrowEntityZ->setQueryFlags(this->defaultCategory);
			// Not ported

			this->arrowEntityZ->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);

			this->arrowEntityZ->setCastShadows(false);
			this->arrowNodeZ->setOrientation(Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Z));
			this->arrowNodeZ->attachObject(this->arrowEntityZ);
			this->sphereNode->setVisible(true);

			this->setupFlag();
			this->unHighlightGizmo();

			if (this->constraintAxis.x != 0.0f)
			{
				this->arrowNodeX->setVisible(false);
			}
			if (this->constraintAxis.y != 0.0f)
			{
				this->arrowNodeY->setVisible(false);
			}
			if (this->constraintAxis.z != 0.0f)
			{
				this->arrowNodeZ->setVisible(false);
			}
		});
	}

	void Gizmo::changeToScaleGizmo(void)
	{
		ENQUEUE_RENDER_COMMAND_WAIT("Gizmo::changeToScaleGizmo",{

			if (this->arrowEntityX != nullptr && this->sceneManager->hasMovableObject(this->arrowEntityX))
			{
				this->sceneManager->destroyEntity(this->arrowEntityX);
			}
			this->arrowEntityX = this->sceneManager->createEntity("Scale.mesh");
			this->arrowEntityX->setName("XArrowGizmoEntity");
			this->arrowEntityX->setDatablock(this->materialNameX);
			this->arrowEntityX->setQueryFlags(this->defaultCategory);
			// Not ported

			this->arrowEntityX->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);

			this->arrowEntityX->setCastShadows(false);
			this->arrowNodeX->attachObject(this->arrowEntityX);

			if (this->arrowEntityY != nullptr && this->sceneManager->hasMovableObject(this->arrowEntityY))
			{
				this->sceneManager->destroyEntity(this->arrowEntityY);
			}
			this->arrowEntityY = this->sceneManager->createEntity("Scale.mesh");
			this->arrowEntityY->setName("YArrowGizmoEntity");
			this->arrowEntityY->setDatablock(this->materialNameY);
			this->arrowEntityY->setQueryFlags(this->defaultCategory);
			this->arrowNodeY->setOrientation(Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::NEGATIVE_UNIT_X));
			// Not ported

			this->arrowEntityY->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);

			this->arrowEntityY->setCastShadows(false);
			this->arrowNodeY->attachObject(this->arrowEntityY);

			if (this->arrowEntityZ != nullptr && this->sceneManager->hasMovableObject(this->arrowEntityZ))
			{
				this->sceneManager->destroyEntity(this->arrowEntityZ);
			}
			this->arrowEntityZ = this->sceneManager->createEntity("Scale.mesh");
			this->arrowEntityZ->setName("ZArrowGizmoEntity");
			this->arrowEntityZ->setDatablock(this->materialNameZ);
			this->arrowEntityZ->setQueryFlags(this->defaultCategory);
			this->arrowNodeZ->setOrientation(Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Z));
			// Not ported

			this->arrowEntityZ->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);

			this->arrowEntityZ->setCastShadows(false);
			this->arrowNodeZ->attachObject(this->arrowEntityZ);
			this->sphereNode->setVisible(true);

			this->setupFlag();
			this->unHighlightGizmo();
		});
	}

	void Gizmo::changeToRotateGizmo(void)
	{
		ENQUEUE_RENDER_COMMAND_WAIT("Gizmo::changeToRotateGizmo",
		{
			if (this->arrowEntityX != nullptr && this->sceneManager->hasMovableObject(this->arrowEntityX))
			{
				this->sceneManager->destroyEntity(this->arrowEntityX);
			}
			this->arrowEntityX = this->sceneManager->createEntity("Ring.mesh");
			this->arrowEntityX->setName("XArrowGizmoEntity");
			this->arrowEntityX->setDatablock(this->materialNameX);
			this->arrowEntityX->setQueryFlags(this->defaultCategory);
			// Not ported

			this->arrowEntityX->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);

			this->arrowEntityX->setCastShadows(false);
			this->arrowNodeX->attachObject(this->arrowEntityX);

			if (this->arrowEntityY != nullptr && this->sceneManager->hasMovableObject(this->arrowEntityY))
			{
				this->sceneManager->destroyEntity(this->arrowEntityY);
			}
			this->arrowEntityY = this->sceneManager->createEntity("Ring.mesh");
			this->arrowEntityY->setName("YArrowGizmoEntity");
			this->arrowEntityY->setDatablock(this->materialNameY);
			this->arrowEntityY->setQueryFlags(this->defaultCategory);
			this->arrowNodeY->setOrientation(Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_X));
			// Not ported

			this->arrowEntityY->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);

			this->arrowEntityY->setCastShadows(false);
			this->arrowNodeY->attachObject(this->arrowEntityY);

			if (this->arrowEntityZ != nullptr && this->sceneManager->hasMovableObject(this->arrowEntityZ))
			{
				this->sceneManager->destroyEntity(this->arrowEntityZ);
			}
			this->arrowEntityZ = this->sceneManager->createEntity("Ring.mesh");
			this->arrowEntityZ->setName("ZArrowGizmoEntity");
			// Z-must be orientated 270, instead of 90, in order to get the circle arc direction work properly!!!!
			this->arrowNodeZ->setOrientation(Ogre::Quaternion(Ogre::Degree(270), Ogre::Vector3::UNIT_Z));
			this->arrowEntityZ->setDatablock(this->materialNameZ);
			this->arrowEntityZ->setQueryFlags(this->defaultCategory);
			// Not ported

			this->arrowEntityZ->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);

			this->arrowEntityZ->setCastShadows(false);
			this->arrowNodeZ->attachObject(this->arrowEntityZ);
			this->sphereNode->setVisible(false);

			this->setupFlag();
			this->unHighlightGizmo();
		});
	}

	void Gizmo::setEnabled(bool enable)
	{
		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("Gizmo::setEnabled", _1(enable),
		{
			this->enabled = enable;

			if (nullptr != this->arrowEntityZ && this->sceneManager->hasMovableObject(this->arrowEntityZ))
			{
				if (enable)
				{
					// Set the specific query flags
					this->arrowEntityX->setQueryFlags(this->defaultCategory);
					this->arrowEntityY->setQueryFlags(this->defaultCategory);
					this->arrowEntityZ->setQueryFlags(this->defaultCategory);
					this->sphereEntity->setQueryFlags(this->defaultCategory);
					this->arrowNodeX->setVisible(true);
					this->arrowNodeY->setVisible(true);
					this->arrowNodeZ->setVisible(true);
					this->sphereNode->setVisible(true);
					if (true == debugPlane)
					{
						this->firstPlaneNode->setVisible(true);
						this->secondPlaneNode->setVisible(true);
						this->thirdPlaneNode->setVisible(true);
					}
				}
				else
				{
					// Hide the nodes and set the queryflags to UNUSED_OBJECT_MASK, so that the gizmo is unselectable!
					this->arrowEntityX->setQueryFlags(0 << 0);
					this->arrowEntityY->setQueryFlags(0 << 0);
					this->arrowEntityZ->setQueryFlags(0 << 0);
					this->sphereEntity->setQueryFlags(0 << 0);
					this->arrowNodeX->setVisible(false);
					this->arrowNodeY->setVisible(false);
					this->arrowNodeZ->setVisible(false);
					this->sphereNode->setVisible(false);
					this->gizmoState = GIZMO_NONE;
					if (true == debugPlane)
					{
						this->firstPlaneNode->setVisible(false);
						this->secondPlaneNode->setVisible(false);
						this->thirdPlaneNode->setVisible(false);
					}
				}
			}

			if (this->constraintAxis.x != 0.0f)
			{
				this->arrowNodeX->setVisible(false);
			}
			if (this->constraintAxis.y != 0.0f)
			{
				this->arrowNodeY->setVisible(false);
			}
			if (this->constraintAxis.z != 0.0f)
			{
				this->arrowNodeZ->setVisible(false);
			}
		});
	}

	bool Gizmo::isEnabled() const
	{
		return this->enabled;
	}

	void Gizmo::setSphereEnabled(bool enable)
	{
		if (nullptr != this->sphereNode)
		{
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("Gizmo::setSphereEnabled", _1(enable),
			{
				this->sphereNode->setVisible(enable);
			});
		}
	}

	void Gizmo::highlightXArrow(void)
	{
		ENQUEUE_RENDER_COMMAND_WAIT("Gizmo::highlightXArrow",
		{
			this->arrowEntityX->setDatablock(this->materialNameHighlight);
			this->arrowEntityY->setDatablock(this->materialNameY);
			this->arrowEntityZ->setDatablock(this->materialNameZ);
			this->gizmoState = GIZMO_ARROW_X;
		});
	}

	void Gizmo::highlightYArrow(void)
	{
		ENQUEUE_RENDER_COMMAND_WAIT("Gizmo::highlightYArrow",
		{
			this->arrowEntityX->setDatablock(this->materialNameX);
			this->arrowEntityY->setDatablock(this->materialNameHighlight);
			this->arrowEntityZ->setDatablock(this->materialNameZ);
			this->gizmoState = GIZMO_ARROW_Y;
		});
	}

	void Gizmo::highlightZArrow(void)
	{
		ENQUEUE_RENDER_COMMAND_WAIT("Gizmo::highlightZArrow",
		{
			this->arrowEntityX->setDatablock(this->materialNameX);
			this->arrowEntityY->setDatablock(this->materialNameY);
			this->arrowEntityZ->setDatablock(this->materialNameHighlight);
			this->gizmoState = GIZMO_ARROW_Z;
		});
	}

	void Gizmo::highlightSphere(void)
	{
		ENQUEUE_RENDER_COMMAND_WAIT("Gizmo::highlightSphere",
		{
			this->sphereEntity->setDatablock(this->materialNameHighlight);
		});
		this->gizmoState = GIZMO_SPHERE;
	}

	void Gizmo::unHighlightGizmo(void)
	{
		if (nullptr != this->arrowEntityX)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("Gizmo::unHighlightGizmo",
			{
				this->arrowEntityX->setDatablock(this->materialNameX);
				this->arrowEntityY->setDatablock(this->materialNameY);
				this->arrowEntityZ->setDatablock(this->materialNameZ);
				this->sphereEntity->setDatablock("WhiteNoLighting");
			});
			this->gizmoState = GIZMO_NONE;
		}
	}

	void Gizmo::setupFlag(void)
	{
		/*
		* setVisibilityFlags(...) changes internal rendering state that the GPU thread depends on. Ogre-Next’s threading model separates logic and rendering systems, 
		* so direct manipulation of scene graph or rendering state from the logic thread is unsafe and leads to data races or undefined behavior.
		*/
		ENQUEUE_RENDER_COMMAND_WAIT("Gizmo::setupFlag",
		{
			this->arrowEntityX->setVisibilityFlags(NOWA::GIZMO_VISIBLE_MASK_VISIBLE);
			this->arrowEntityY->setVisibilityFlags(NOWA::GIZMO_VISIBLE_MASK_VISIBLE);
			this->arrowEntityZ->setVisibilityFlags(NOWA::GIZMO_VISIBLE_MASK_VISIBLE);
			this->sphereEntity->setVisibilityFlags(NOWA::GIZMO_VISIBLE_MASK_VISIBLE);
		});
	}

	Ogre::Real Gizmo::getGizmoBoundingRadius(void) const
	{
		if (this->arrowEntityX != nullptr && this->sceneManager->hasMovableObject(this->arrowEntityX))
		{
// Attention: Is this correct?
			return this->arrowNodeX->getAttachedObject(0)->getWorldAabb().getRadius() * 1.2f;
		}
		else
		{
			return 10.0;
		}
	}

	Ogre::Plane& Gizmo::getFirstPlane(void)
	{
		return this->firstPlane;
	}

	Ogre::Plane& Gizmo::getSecondPlane(void)
	{
		return this->secondPlane;
	}

	Ogre::Plane& Gizmo::getThirdPlane(void)
	{
		return this->thirdPlane;
	}

	void Gizmo::setState(Gizmo::eGizmoState gizmoState)
	{
		this->gizmoState = gizmoState;
	}

	Gizmo::eGizmoState Gizmo::getState(void) const
	{
		return this->gizmoState;
	}

	void Gizmo::changeThickness(Ogre::Real thickness)
	{
		this->thickness = thickness;
	}

	void Gizmo::changeMaterials(const Ogre::String& materialNameX, const Ogre::String& materialNameY, const Ogre::String& materialNameZ, const Ogre::String& materialNameHighlight)
	{
		this->materialNameX = materialNameX;
		this->materialNameY = materialNameY;
		this->materialNameZ = materialNameZ;
		this->materialNameHighlight = materialNameHighlight;
	}

	Ogre::SceneNode* Gizmo::getArrowNodeX(void)
	{
		return this->arrowNodeX;
	}

	Ogre::SceneNode* Gizmo::getArrowNodeY(void)
	{
		return this->arrowNodeY;
	}

	Ogre::SceneNode* Gizmo::getArrowNodeZ(void)
	{
		return this->arrowNodeZ;
	}

	Ogre::SceneNode* Gizmo::getSelectedNode(void)
	{
		return this->selectNode;
	}

	Ogre::v1::Entity* Gizmo::getArrowEntityX(void)
	{
		return this->arrowEntityX;
	}

	Ogre::v1::Entity* Gizmo::getArrowEntityY(void)
	{
		return this->arrowEntityY;
	}

	Ogre::v1::Entity* Gizmo::getArrowEntityZ(void)
	{
		return this->arrowEntityZ;
	}

	Ogre::v1::Entity* Gizmo::getSphereEntity(void)
	{
		return this->sphereEntity;
	}

	void Gizmo::createLine(void)
	{
		ENQUEUE_RENDER_COMMAND_WAIT("Gizmo::createLine",
		{
			this->translationLineNode = this->sceneManager->getRootSceneNode()->createChildSceneNode();
			// this->translationLine = new Ogre::v1::ManualObject(0, &this->sceneManager->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), this->sceneManager);
			this->translationLine = this->sceneManager->createManualObject(Ogre::SCENE_DYNAMIC);
			// this->sceneManager->createManualObject(Ogre::SCENE_DYNAMIC);
			this->translationLine->setQueryFlags(0 << 0);
			this->translationLineNode->attachObject(this->translationLine);
			this->translationLine->setCastShadows(false);
			this->translationLine->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_OBJECTS_ALWAYS_IN_FOREGROUND);
			// this->translationLine->setDebugDisplayEnabled(true);

			this->translationCaption = new ObjectTitle("TranslationCaption", this->translationLine, this->camera, "BlueHighway", Ogre::ColourValue::White);
		});
	}

	void Gizmo::drawLine(const Ogre::Vector3& startPosition, const Ogre::Vector3& endPosition, Ogre::Real thickness, const Ogre::String& materialName)
	{
		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("Gizmo::drawLine", _4(startPosition, endPosition, thickness, materialName),
		{
			Ogre::Real const relativeThickness = this->sphereNode->getScale().x * thickness;
			this->translationLine->clear();
			this->translationLine->begin(materialName, Ogre::OperationType::OT_LINE_LIST);

			this->translationLine->position(startPosition);
			this->translationLine->index(0);
			this->translationLine->position(endPosition);
			this->translationLine->index(1);
			this->translationLine->end();

			if (this->translationCaption)
			{
				this->translationCaption->update();  // Assuming it's a render-thread safe operation
			}
		});
	}

	void Gizmo::hideLine(void)
	{
		ENQUEUE_RENDER_COMMAND("Gizmo::hideLine", 
		{
			this->translationLine->clear();
			this->translationCaption->setTitle("");
		});
	}

	void Gizmo::destroyLine()
	{
		ENQUEUE_RENDER_COMMAND_WAIT("Gizmo::destroyLine",
		{
			if (this->translationLineNode != nullptr)
			{
				this->translationLineNode->detachAllObjects();
				if (this->translationLine != nullptr)
				{
					this->sceneManager->destroyManualObject(this->translationLine);
					// delete this->translationLine;
					this->translationLine = nullptr;
				}
				this->translationLineNode->getParentSceneNode()->removeAndDestroyChild(this->translationLineNode);
				this->translationLineNode = nullptr;
			}
		});
	}

	void Gizmo::createCircle(void)
	{
		ENQUEUE_RENDER_COMMAND_WAIT("Gizmo::createCircle",
		{
			this->rotationCircleNode = this->sceneManager->getRootSceneNode()->createChildSceneNode();
			this->rotationCircle = this->sceneManager->createManualObject();
			this->rotationCircle->setQueryFlags(0 << 0);
			this->rotationCircleNode->attachObject(this->rotationCircle);
			this->rotationCircle->setCastShadows(false);

			this->rotationCircle->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_OBJECTS_ALWAYS_IN_FOREGROUND);
			// this->rotationCircle->setDebugDisplayEnabled(true);
			// this->rotationCircle->setDynamic(true);

			this->rotationCaption = new ObjectTitle("RotationCaption", this->rotationCircle, this->camera, "BlueHighway", Ogre::ColourValue::White);
		});
	}

	void Gizmo::drawCircle(const Ogre::Quaternion& orientation, Ogre::Real fromAngle, Ogre::Real toAngle, bool counterClockWise, Ogre::Real thickness, const Ogre::String& materialName)
	{
		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("Gizmo::drawCircle", _6(orientation, fromAngle, toAngle, counterClockWise, thickness, materialName),
		{
			// Draw a circle for visual effect
			this->rotationCircle->clear();
			this->rotationCircle->begin(materialName, Ogre::OperationType::OT_TRIANGLE_LIST);
			Ogre::Real const radius = this->sphereNode->getScale().x * 5.0f;
			Ogre::Real const accuracy = 50.0f; // Number of plots for accuracy
			Ogre::Real const relativeThickness = this->sphereNode->getScale().x * thickness;
			unsigned int index = 0;
			int colourFactor = 1;
			Ogre::ColourValue value;
			if (true == counterClockWise)
			{
				for (Ogre::Real theta = fromAngle; theta < toAngle; theta += Ogre::Math::PI / accuracy)
				{
					colourFactor = static_cast<int>(Ogre::Math::Abs(theta / Ogre::Math::TWO_PI)) % 10;

					this->rotationCircle->position(radius * cos(theta), 0.0f, radius * sin(theta));
					// this->rotationCircle->colour(colourMap[colourFactor].value);
					this->rotationCircle->position(radius * cos(theta - Ogre::Math::PI / accuracy), 0.0f, radius * sin(theta - Ogre::Math::PI / accuracy));
					// this->rotationCircle->colour(colourMap[colourFactor].value);
					this->rotationCircle->position((radius - relativeThickness) * cos(theta - Ogre::Math::PI / accuracy), 0.0f, (radius - relativeThickness) * sin(theta - Ogre::Math::PI / accuracy));
					// this->rotationCircle->colour(colourMap[colourFactor].value);
					this->rotationCircle->position((radius - relativeThickness) * cos(theta), 0.0f, (radius - relativeThickness) * sin(theta));
					// this->rotationCircle->colour(colourMap[colourFactor].value);
					// Join the 4 vertices created above to form a quad.
					this->rotationCircle->quad(index, index + 1, index + 2, index + 3);
					index += 4;
				}
				// MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("DebugLabel")->setCaption("C: " + Ogre::StringConverter::toString(colourFactor));
			}
			else
			{
				for (Ogre::Real theta = fromAngle; theta > toAngle; theta -= Ogre::Math::PI / accuracy)
				{
					colourFactor = static_cast<int>(Ogre::Math::Abs(theta / Ogre::Math::TWO_PI)) % 10;
					this->rotationCircle->position(radius * cos(theta), 0.0f, radius * sin(theta));
					// this->rotationCircle->colour(colourMap[colourFactor].value);
					this->rotationCircle->position(radius * cos(theta - Ogre::Math::PI / accuracy), 0.0f, radius * sin(theta - Ogre::Math::PI / accuracy));
					// this->rotationCircle->colour(colourMap[colourFactor].value);
					this->rotationCircle->position((radius - relativeThickness) * cos(theta - Ogre::Math::PI / accuracy), 0.0f, (radius - relativeThickness) * sin(theta - Ogre::Math::PI / accuracy));
					// this->rotationCircle->colour(colourMap[colourFactor].value);
					this->rotationCircle->position((radius - relativeThickness) * cos(theta), 0.0f, (radius - relativeThickness) * sin(theta));
					// this->rotationCircle->colour(colourMap[colourFactor].value);
					// Join the 4 vertices created above to form a quad.
					this->rotationCircle->quad(index, index + 1, index + 2, index + 3);
					index += 4;
				}
				// MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("DebugLabel")->setCaption("C: " + Ogre::StringConverter::toString(colourFactor));
			}
			this->rotationCircle->end();

			/*this->rotationCircleMaterialPtr->getTechnique(0)->getPass(0)->setDiffuse(colourMap[colourFactor].value);
			this->rotationCircleMaterialPtr->getTechnique(0)->getPass(0)->setAmbient(colourMap[colourFactor].value);
			this->rotationCircleMaterialPtr->getTechnique(0)->getPass(0)->setSelfIllumination(colourMap[colourFactor].value);*/

			/*if (std::fmod(toAngle, Ogre::Math::TWO_PI) <= 0.0f)
			{
			Ogre::TextureUnitState* textureUnitState = pass->getTextureUnitState(0);
			textureUnitState->setAlphaOperation(Ogre::LBX_SOURCE1, Ogre::LBS_MANUAL, Ogre::LBS_TEXTURE, 0.25f);
			}*/

			this->rotationCircleNode->setPosition(this->selectNode->_getDerivedPositionUpdated());
			this->rotationCircleNode->setOrientation(orientation);

			if (this->rotationCaption)
			{
				this->rotationCaption->update();
			}
		});
	}

	void Gizmo::hideCircle(void)
	{
		ENQUEUE_RENDER_COMMAND("Gizmo::hideCircle",
		{
			this->rotationCircle->clear();
			this->rotationCaption->setTitle("");
		});
	}

	void Gizmo::destroyCircle()
	{
		ENQUEUE_RENDER_COMMAND_WAIT("Gizmo::destroyCircle",
		{
			if (this->rotationCircleNode != nullptr)
			{
				this->rotationCircleNode->detachAllObjects();
				if (this->rotationCircle != nullptr)
				{
					this->sceneManager->destroyManualObject(this->rotationCircle);
					this->rotationCircle = nullptr;
				}
				this->rotationCircleNode->getParentSceneNode()->removeAndDestroyChild(this->rotationCircleNode);
				this->rotationCircleNode = nullptr;
			}
		});
	}

	void Gizmo::setTranslationCaption(const Ogre::String& caption, const Ogre::ColourValue& color)
	{
		if (this->translationCaption)
		{
			ENQUEUE_RENDER_COMMAND_MULTI("Gizmo::setTranslationCaption", _2(caption, color),
			{
				this->translationCaption->setTitle(caption);
				this->translationCaption->setColor(color);
			});
		}
	}

	void Gizmo::setRotationCaption(const Ogre::String& caption, const Ogre::ColourValue& color)
	{
		if (this->rotationCaption)
		{
			ENQUEUE_RENDER_COMMAND_MULTI("Gizmo::setRotationCaption", _2(caption, color),
			{
				this->rotationCaption->setTitle(caption);
				this->rotationCaption->setColor(color);
			});
		}
	}

	void Gizmo::setPosition(const Ogre::Vector3& position)
	{
		// ENQUEUE_RENDER_COMMAND_MULTI_WAIT("", _1(position), {
			if (this->constraintAxis == Ogre::Vector3::ZERO)
			{
				// this->selectNode->setPosition(position);
				RenderCommandQueueModule::getInstance()->updateNodePosition(this->selectNode, position);
			}
			else
			{
				if (this->constraintAxis.x != 0.0f)
				{
					// this->selectNode->setPosition(Ogre::Vector3(this->constraintAxis.x, position.y, position.z));
					RenderCommandQueueModule::getInstance()->updateNodePosition(this->selectNode, Ogre::Vector3(this->constraintAxis.x, position.y, position.z));
				}
				if (this->constraintAxis.y != 0.0f)
				{
					// this->selectNode->setPosition(Ogre::Vector3(position.x, this->constraintAxis.y, position.z));
					RenderCommandQueueModule::getInstance()->updateNodePosition(this->selectNode, Ogre::Vector3(position.x, this->constraintAxis.y, position.z));
				}
				if (this->constraintAxis.z != 0.0f)
				{
					// this->selectNode->setPosition(Ogre::Vector3(position.x, position.y, this->constraintAxis.z));
					RenderCommandQueueModule::getInstance()->updateNodePosition(this->selectNode, Ogre::Vector3(position.x, position.y, this->constraintAxis.z));
				}
			}
		// });
	}

	Ogre::Vector3 Gizmo::getPosition(void) const
	{
		return this->selectNode->_getDerivedPositionUpdated();
	}

	void Gizmo::setOrientation(const Ogre::Quaternion& orientation)
	{
		RenderCommandQueueModule::getInstance()->updateNodeOrientation(this->selectNode, orientation);
	}

	Ogre::Quaternion Gizmo::getOrientation(void) const
	{
		return this->selectNode->_getDerivedOrientationUpdated();
	}

	void Gizmo::translate(const Ogre::Vector3& translateVector)
	{
		Ogre::Vector3 internalTranslateVector = translateVector;
		if (this->constraintAxis.x != 0.0f)
		{
			internalTranslateVector *= Ogre::Vector3(0.0f, 1.0f, 1.0f);
		}
		if (this->constraintAxis.y != 0.0f)
		{
			internalTranslateVector *= Ogre::Vector3(1.0f, 0.0f, 1.0f);
		}
		if (this->constraintAxis.z != 0.0f)
		{
			internalTranslateVector *= Ogre::Vector3(1.0f, 1.0f, 0.0f);
		}
		
		Ogre::Vector3 newPosition = this->selectNode->getPosition() + internalTranslateVector;
		RenderCommandQueueModule::getInstance()->updateNodePosition(this->selectNode, newPosition);
	}

	void Gizmo::rotate(const Ogre::Quaternion& rotateQuaternion)
	{
		Ogre::Quaternion newOrientation = this->selectNode->getOrientation() * rotateQuaternion;
		RenderCommandQueueModule::getInstance()->updateNodeOrientation(this->selectNode, newOrientation);
	}

	void Gizmo::setConstraintAxis(const Ogre::Vector3& constraintAxis)
	{
		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("Gizmo::setConstraintAxis", _1(constraintAxis), 
		{
			this->constraintAxis = constraintAxis;
			this->arrowNodeX->setVisible(true);
			this->arrowNodeY->setVisible(true);
			this->arrowNodeZ->setVisible(true);
			// Hide the arrow
			if (this->constraintAxis.x != 0.0f)
			{
				this->arrowNodeX->setVisible(false);
			}
			if (this->constraintAxis.y != 0.0f)
			{
				this->arrowNodeY->setVisible(false);
			}
			if (this->constraintAxis.z != 0.0f)
			{
				this->arrowNodeZ->setVisible(false);
			}
		});
	}

	Ogre::Vector3 Gizmo::getCurrentDirection(void)
	{
		Ogre::Vector3 direction = Ogre::Vector3::ZERO;

		switch (this->getState())
		{
		case Gizmo::GIZMO_ARROW_X:
		{
			direction = this->getOrientation().xAxis();
			break;
		}
		case Gizmo::GIZMO_ARROW_Y:
		{
			direction = this->getOrientation().yAxis();
			break;
		}
		case Gizmo::GIZMO_ARROW_Z:
		{
			direction = this->getOrientation().zAxis();
			break;
		}
		case Gizmo::GIZMO_SPHERE:
		{
			/*gizmoDirectionX = this->gizmo->getSelectedNode()->_getDerivedOrientationUpdated().xAxis();
			gizmoDirectionY = this->gizmo->getSelectedNode()->_getDerivedOrientationUpdated().yAxis();
			gizmoDirectionZ = this->gizmo->getSelectedNode()->_getDerivedOrientationUpdated().zAxis();*/
		}
		}

		return direction;
	}

}; // namespace end