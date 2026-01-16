#include "NOWAPrecompiled.h"
#include "Gizmo.h"

#include "utilities/MathHelper.h"
#include "main/Core.h"
#include "main/AppStateManager.h"
#include "modules/GraphicsModule.h"

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
		if (this->arrowNodeX == nullptr)
		{
			return;
		}

		NOWA::GraphicsModule::getInstance()->removeTrackedNode(firstPlaneNode);
		NOWA::GraphicsModule::getInstance()->removeTrackedNode(secondPlaneNode);
		NOWA::GraphicsModule::getInstance()->removeTrackedNode(thirdPlaneNode);
		NOWA::GraphicsModule::getInstance()->removeTrackedNode(arrowNodeX);
		NOWA::GraphicsModule::getInstance()->removeTrackedNode(arrowNodeY);
		NOWA::GraphicsModule::getInstance()->removeTrackedNode(arrowNodeZ);
		NOWA::GraphicsModule::getInstance()->removeTrackedNode(sphereNode);
		NOWA::GraphicsModule::getInstance()->removeTrackedNode(selectNode);

		auto sceneManager = this->sceneManager;

		auto arrowNodeX = this->arrowNodeX;
		auto arrowNodeY = this->arrowNodeY;
		auto arrowNodeZ = this->arrowNodeZ;
		auto sphereNode = this->sphereNode;
		auto selectNode = this->selectNode;

		auto arrowEntityX = this->arrowEntityX;
		auto arrowEntityY = this->arrowEntityY;
		auto arrowEntityZ = this->arrowEntityZ;
		auto sphereEntity = this->sphereEntity;

		auto debugPlane = this->debugPlane;

		auto firstPlaneEntity = this->firstPlaneEntity;
		auto secondPlaneEntity = this->secondPlaneEntity;
		auto thirdPlaneEntity = this->thirdPlaneEntity;

		auto firstPlaneNode = this->firstPlaneNode;
		auto secondPlaneNode = this->secondPlaneNode;
		auto thirdPlaneNode = this->thirdPlaneNode;

		ENQUEUE_DESTROY_COMMAND("Destroy Gizmo", _17(sceneManager, arrowNodeX, arrowNodeY, arrowNodeZ, sphereNode, selectNode, arrowEntityX, arrowEntityY, arrowEntityZ,
			sphereEntity, debugPlane, firstPlaneEntity, secondPlaneEntity, thirdPlaneEntity, firstPlaneNode, secondPlaneNode, thirdPlaneNode),
		{
			arrowNodeX->detachAllObjects();
			arrowNodeY->detachAllObjects();
			arrowNodeZ->detachAllObjects();
			sphereNode->detachAllObjects();
			selectNode->detachAllObjects();

			if (debugPlane)
			{
				firstPlaneNode->detachAllObjects();
				secondPlaneNode->detachAllObjects();

				sceneManager->destroyEntity(firstPlaneEntity);
				sceneManager->destroyEntity(secondPlaneEntity);
				sceneManager->destroyEntity(thirdPlaneEntity);
				sceneManager->destroySceneNode(firstPlaneNode);
				sceneManager->destroySceneNode(secondPlaneNode);
				sceneManager->destroySceneNode(thirdPlaneNode);
			}

			if (arrowEntityX) sceneManager->destroyEntity(arrowEntityX);
			if (arrowEntityY) sceneManager->destroyEntity(arrowEntityY);
			if (arrowEntityZ) sceneManager->destroyEntity(arrowEntityZ);
			if (sphereEntity) sceneManager->destroyEntity(sphereEntity);

			sceneManager->destroySceneNode(arrowNodeX);
			sceneManager->destroySceneNode(arrowNodeY);
			sceneManager->destroySceneNode(arrowNodeZ);
			sceneManager->destroySceneNode(sphereNode);
			sceneManager->destroySceneNode(selectNode);
		});

		this->destroyLine();
		this->destroyCircle();

		delete this->translationCaption;
		this->translationCaption = nullptr;

		delete this->rotationCaption;
		this->rotationCaption = nullptr;
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

		ENQUEUE_RENDER_COMMAND("Gizmo::init",{

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

#if 0
	void Gizmo::update(void)
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
	}
#endif

	void Gizmo::update(void)
	{
		if (false == this->enabled)
		{
			return;
		}

		// Compute all values on logic thread
		Ogre::Vector3 nodePos = this->sphereNode->_getDerivedPositionUpdated();
		Ogre::Vector3 camPos = this->camera->getDerivedPosition();

		Ogre::Real camDistance = 0.0;
		if (this->camera->getProjectionType() == Ogre::PT_PERSPECTIVE)
		{
			camDistance = nodePos.distance(camPos);
		}
		else
		{
			camDistance = nodePos.distance(Ogre::Vector3(0, this->camera->getOrthoWindowHeight(), 0));
		}

		Ogre::Real fovy = this->camera->getFOVy().valueRadians();
		Ogre::Real distance = camDistance * 0.03f;

		Ogre::Vector3 sphereScale(distance * fovy * 0.5f, distance * fovy * 0.5f, distance * fovy * 0.85f);

		auto closureFunction = [this, sphereScale, distance, fovy](Ogre::Real renderDt)
		{
			if (this->translationLineNode)
			{
				this->translationLineNode->setPosition(this->getPosition());
			}

			if (sphereNode)
			{
				sphereNode->setScale(sphereScale);
			}

			if (this->arrowEntityX && this->sceneManager && this->sceneManager->hasMovableObject(arrowEntityX))
			{
				bool useRingMesh = (arrowEntityX->getMesh()->getName() == "Ring.mesh");

				Ogre::Vector3 arrowScaleLocal;
				if (!useRingMesh)
				{
					arrowScaleLocal = Ogre::Vector3(distance * thickness * fovy, distance * thickness * fovy, distance * fovy);
				}
				else
				{
					arrowScaleLocal = Ogre::Vector3(distance * fovy, distance * thickness * fovy, distance * fovy);
				}

				this->arrowNodeX->setScale(arrowScaleLocal);
				this->arrowNodeY->setScale(arrowScaleLocal);
				this->arrowNodeZ->setScale(arrowScaleLocal);
			}
		};
		Ogre::String id = "Gizmo::update";
		NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction);
	}

	void Gizmo::redefineFirstPlane(const Ogre::Vector3& normal, const Ogre::Vector3& position)
	{
		this->firstPlane.redefine(normal, position);

		if (debugPlane)
		{
			auto firstPlaneNode = this->firstPlaneNode;
			auto normalCopy = normal;
			auto positionCopy = position;

			ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::redefineFirstPlane", _3(firstPlaneNode, normalCopy, positionCopy),
			{
				firstPlaneNode->setPosition(positionCopy);
				firstPlaneNode->lookAt(positionCopy + normalCopy, Ogre::Node::TS_WORLD);
			});
		}
	}

	void Gizmo::redefineSecondPlane(const Ogre::Vector3& normal, const Ogre::Vector3& position)
	{
		this->secondPlane.redefine(normal, position);

		if (debugPlane)
		{
			auto secondPlaneNode = this->secondPlaneNode;
			auto normalCopy = normal;
			auto positionCopy = position;

			ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::redefineSecondPlane", _3(secondPlaneNode, normalCopy, positionCopy),
			{
				secondPlaneNode->setPosition(positionCopy);
				secondPlaneNode->lookAt(positionCopy + normalCopy, Ogre::Node::TS_WORLD);
			});
		}
	}

	void Gizmo::redefineThirdPlane(const Ogre::Vector3& normal, const Ogre::Vector3& position)
	{
		this->thirdPlane.redefine(normal, position);

		if (debugPlane)
		{
			auto thirdPlaneNode = this->thirdPlaneNode;
			auto normalCopy = normal;
			auto positionCopy = position;

			ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::redefineThirdPlane", _3(thirdPlaneNode, normalCopy, positionCopy),
			{
				thirdPlaneNode->setPosition(positionCopy);
				thirdPlaneNode->lookAt(positionCopy + normalCopy, Ogre::Node::TS_WORLD);
			});
		}
	}

	void Gizmo::_debugShowResultPlane(unsigned char planeId)
	{
		if (!debugPlane)
		{
			return;
		}

		auto firstPlaneNode = this->firstPlaneNode;
		auto secondPlaneNode = this->secondPlaneNode;
		auto thirdPlaneNode = this->thirdPlaneNode;
		auto selectNode = this->selectNode;
		auto firstPlane = this->firstPlane;
		auto secondPlane = this->secondPlane;
		auto thirdPlane = this->thirdPlane;

		ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::_debugShowResultPlane", _8(firstPlaneNode, secondPlaneNode, thirdPlaneNode, selectNode, firstPlane, secondPlane, thirdPlane, planeId),
		{
			auto pos = selectNode->_getDerivedPosition();

			if (planeId == 1)
			{
				firstPlaneNode->setVisible(true);
				firstPlaneNode->setPosition(pos);
				firstPlaneNode->lookAt(pos + firstPlane.normal, Ogre::Node::TS_WORLD);
				secondPlaneNode->setVisible(false);
				thirdPlaneNode->setVisible(false);
			}
			else if (planeId == 2)
			{
				secondPlaneNode->setVisible(true);
				secondPlaneNode->setPosition(pos);
				secondPlaneNode->lookAt(pos + secondPlane.normal, Ogre::Node::TS_WORLD);
				firstPlaneNode->setVisible(false);
				thirdPlaneNode->setVisible(false);
			}
			else if (planeId == 3)
			{
				thirdPlaneNode->setVisible(true);
				thirdPlaneNode->setPosition(pos);
				thirdPlaneNode->lookAt(pos + thirdPlane.normal, Ogre::Node::TS_WORLD);
				firstPlaneNode->setVisible(false);
				secondPlaneNode->setVisible(false);
			}
		});
	}


	void Gizmo::changeToTranslateGizmo()
	{
		auto sceneManager = this->sceneManager;
		auto defaultCategory = this->defaultCategory;
		auto materialNameX = this->materialNameX;
		auto materialNameY = this->materialNameY;
		auto materialNameZ = this->materialNameZ;
		auto arrowNodeX = this->arrowNodeX;
		auto arrowNodeY = this->arrowNodeY;
		auto arrowNodeZ = this->arrowNodeZ;
		auto sphereNode = this->sphereNode;
		auto constraintAxis = this->constraintAxis;

		auto oldArrowEntityX = this->arrowEntityX;
		auto oldArrowEntityY = this->arrowEntityY;
		auto oldArrowEntityZ = this->arrowEntityZ;

		auto thisPtr = this;

		ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::changeToTranslateGizmo", _14(sceneManager, defaultCategory, materialNameX, materialNameY, materialNameZ,
			arrowNodeX, arrowNodeY, arrowNodeZ, sphereNode, constraintAxis, oldArrowEntityX, oldArrowEntityY, oldArrowEntityZ, thisPtr),
		{
			// Destroy old entities
			if (oldArrowEntityX && sceneManager->hasMovableObject(oldArrowEntityX))
				sceneManager->destroyEntity(oldArrowEntityX);
			if (oldArrowEntityY && sceneManager->hasMovableObject(oldArrowEntityY))
				sceneManager->destroyEntity(oldArrowEntityY);
			if (oldArrowEntityZ && sceneManager->hasMovableObject(oldArrowEntityZ))
				sceneManager->destroyEntity(oldArrowEntityZ);

			// Create new X arrow entity
			Ogre::v1::Entity* newArrowEntityX = sceneManager->createEntity("Arrow.mesh");
			newArrowEntityX->setName("XArrowGizmoEntity");
			newArrowEntityX->setDatablock(materialNameX);
			newArrowEntityX->setQueryFlags(defaultCategory);
			newArrowEntityX->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);
			newArrowEntityX->setCastShadows(false);
			if (arrowNodeX)
				arrowNodeX->attachObject(newArrowEntityX);

			// Create new Y arrow entity
			Ogre::v1::Entity* newArrowEntityY = sceneManager->createEntity("Arrow.mesh");
			newArrowEntityY->setName("YArrowGizmoEntity");
			newArrowEntityY->setDatablock(materialNameY);
			newArrowEntityY->setQueryFlags(defaultCategory);
			newArrowEntityY->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);
			newArrowEntityY->setCastShadows(false);
			if (arrowNodeY)
			{
				arrowNodeY->setOrientation(Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::NEGATIVE_UNIT_X));
				arrowNodeY->attachObject(newArrowEntityY);
			}

			// Create new Z arrow entity
			Ogre::v1::Entity* newArrowEntityZ = sceneManager->createEntity("Arrow.mesh");
			newArrowEntityZ->setName("ZArrowGizmoEntity");
			newArrowEntityZ->setDatablock(materialNameZ);
			newArrowEntityZ->setQueryFlags(defaultCategory);
			newArrowEntityZ->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);
			newArrowEntityZ->setCastShadows(false);
			if (arrowNodeZ)
			{
				arrowNodeZ->setOrientation(Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Z));
				arrowNodeZ->attachObject(newArrowEntityZ);
			}

			if (sphereNode)
				sphereNode->setVisible(true);

			// Update pointers safely on render thread
			thisPtr->arrowEntityX = newArrowEntityX;
			thisPtr->arrowEntityY = newArrowEntityY;
			thisPtr->arrowEntityZ = newArrowEntityZ;

			thisPtr->setupFlag();
			thisPtr->unHighlightGizmo();

			// Handle constraintAxis visibility
			if (arrowNodeX && constraintAxis.x != 0.0f)
				arrowNodeX->setVisible(false);
			if (arrowNodeY && constraintAxis.y != 0.0f)
				arrowNodeY->setVisible(false);
			if (arrowNodeZ && constraintAxis.z != 0.0f)
				arrowNodeZ->setVisible(false);
		});
	}


	void Gizmo::changeToScaleGizmo()
	{
		auto sceneManager = this->sceneManager;
		auto defaultCategory = this->defaultCategory;
		auto materialNameX = this->materialNameX;
		auto materialNameY = this->materialNameY;
		auto materialNameZ = this->materialNameZ;
		auto arrowNodeX = this->arrowNodeX;
		auto arrowNodeY = this->arrowNodeY;
		auto arrowNodeZ = this->arrowNodeZ;
		auto sphereNode = this->sphereNode;

		auto oldArrowEntityX = this->arrowEntityX;
		auto oldArrowEntityY = this->arrowEntityY;
		auto oldArrowEntityZ = this->arrowEntityZ;

		auto thisPtr = this;

		ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::changeToScaleGizmo", _13(sceneManager, defaultCategory, materialNameX, materialNameY, materialNameZ,
			arrowNodeX, arrowNodeY, arrowNodeZ, sphereNode, oldArrowEntityX, oldArrowEntityY, oldArrowEntityZ, thisPtr),
		{
			if (oldArrowEntityX && sceneManager->hasMovableObject(oldArrowEntityX))
			{
				sceneManager->destroyEntity(oldArrowEntityX);
			}
			if (oldArrowEntityY && sceneManager->hasMovableObject(oldArrowEntityY))
			{
				sceneManager->destroyEntity(oldArrowEntityY);
			}
			if (oldArrowEntityZ && sceneManager->hasMovableObject(oldArrowEntityZ))
			{
				sceneManager->destroyEntity(oldArrowEntityZ);
			}

			// Create new X entity
			Ogre::v1::Entity* newArrowEntityX = sceneManager->createEntity("Scale.mesh");
			newArrowEntityX->setName("XArrowGizmoEntity");
			newArrowEntityX->setDatablock(materialNameX);
			newArrowEntityX->setQueryFlags(defaultCategory);
			newArrowEntityX->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);
			newArrowEntityX->setCastShadows(false);
			if (arrowNodeX)
			{
				arrowNodeX->attachObject(newArrowEntityX);
			}

			// Create new Y entity
			Ogre::v1::Entity* newArrowEntityY = sceneManager->createEntity("Scale.mesh");
			newArrowEntityY->setName("YArrowGizmoEntity");
			newArrowEntityY->setDatablock(materialNameY);
			newArrowEntityY->setQueryFlags(defaultCategory);
			newArrowEntityY->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);
			newArrowEntityY->setCastShadows(false);
			if (arrowNodeY)
			{
				arrowNodeY->setOrientation(Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::NEGATIVE_UNIT_X));
				arrowNodeY->attachObject(newArrowEntityY);
			}

			// Create new Z entity
			Ogre::v1::Entity* newArrowEntityZ = sceneManager->createEntity("Scale.mesh");
			newArrowEntityZ->setName("ZArrowGizmoEntity");
			newArrowEntityZ->setDatablock(materialNameZ);
			newArrowEntityZ->setQueryFlags(defaultCategory);
			newArrowEntityZ->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);
			newArrowEntityZ->setCastShadows(false);
			if (arrowNodeZ)
			{
				arrowNodeZ->setOrientation(Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Z));
				arrowNodeZ->attachObject(newArrowEntityZ);
			}

			if (sphereNode)
			{
				sphereNode->setVisible(true);
			}

			// Update pointers safely in render thread
			thisPtr->arrowEntityX = newArrowEntityX;
			thisPtr->arrowEntityY = newArrowEntityY;
			thisPtr->arrowEntityZ = newArrowEntityZ;

			// Call functions that touch render state inside render thread
			thisPtr->setupFlag();
			thisPtr->unHighlightGizmo();
		});
	}


	void Gizmo::changeToRotateGizmo()
	{
		// Capture this and pointers to all needed members and params safely for lambda capture
		auto sceneManager = this->sceneManager;
		auto defaultCategory = this->defaultCategory;
		auto materialNameX = this->materialNameX;
		auto materialNameY = this->materialNameY;
		auto materialNameZ = this->materialNameZ;
		auto arrowNodeX = this->arrowNodeX;
		auto arrowNodeY = this->arrowNodeY;
		auto arrowNodeZ = this->arrowNodeZ;
		auto sphereNode = this->sphereNode;

		// Also capture pointers to entities so we can destroy them safely in render thread
		auto oldArrowEntityX = this->arrowEntityX;
		auto oldArrowEntityY = this->arrowEntityY;
		auto oldArrowEntityZ = this->arrowEntityZ;

		// Pointers to this to update members inside render thread
		auto thisPtr = this;

		ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::changeToRotateGizmo", _13(sceneManager, defaultCategory, materialNameX, materialNameY, materialNameZ,
			arrowNodeX, arrowNodeY, arrowNodeZ, sphereNode, oldArrowEntityX, oldArrowEntityY, oldArrowEntityZ, thisPtr),
		{
			// Destroy old entities if valid
			if (oldArrowEntityX && sceneManager->hasMovableObject(oldArrowEntityX))
			{
				sceneManager->destroyEntity(oldArrowEntityX);
			}
			if (oldArrowEntityY && sceneManager->hasMovableObject(oldArrowEntityY))
			{
				sceneManager->destroyEntity(oldArrowEntityY);
			}
			if (oldArrowEntityZ && sceneManager->hasMovableObject(oldArrowEntityZ))
			{
				sceneManager->destroyEntity(oldArrowEntityZ);
			}

			// Create new X entity
			Ogre::v1::Entity* newArrowEntityX = sceneManager->createEntity("Ring.mesh");
			newArrowEntityX->setName("XArrowGizmoEntity");
			newArrowEntityX->setDatablock(materialNameX);
			newArrowEntityX->setQueryFlags(defaultCategory);
			newArrowEntityX->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);
			newArrowEntityX->setCastShadows(false);
			if (arrowNodeX)
			{
				arrowNodeX->attachObject(newArrowEntityX);
			}

			// Create new Y entity
			Ogre::v1::Entity* newArrowEntityY = sceneManager->createEntity("Ring.mesh");
			newArrowEntityY->setName("YArrowGizmoEntity");
			newArrowEntityY->setDatablock(materialNameY);
			newArrowEntityY->setQueryFlags(defaultCategory);
			newArrowEntityY->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);
			newArrowEntityY->setCastShadows(false);
			if (arrowNodeY)
			{
				arrowNodeY->setOrientation(Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_X));
				arrowNodeY->attachObject(newArrowEntityY);
			}

			// Create new Z entity
			Ogre::v1::Entity* newArrowEntityZ = sceneManager->createEntity("Ring.mesh");
			newArrowEntityZ->setName("ZArrowGizmoEntity");
			newArrowEntityZ->setDatablock(materialNameZ);
			newArrowEntityZ->setQueryFlags(defaultCategory);
			newArrowEntityZ->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);
			newArrowEntityZ->setCastShadows(false);
			if (arrowNodeZ)
			{
				// 270 degrees around Z axis for proper arc direction
				arrowNodeZ->setOrientation(Ogre::Quaternion(Ogre::Degree(270), Ogre::Vector3::UNIT_Z));
				arrowNodeZ->attachObject(newArrowEntityZ);
			}

			if (sphereNode)
			{
				sphereNode->setVisible(false);
			}

			// Update this->arrowEntity* pointers on logic side is unsafe here,
			// but we can store them here in thisPtr for thread safety.
			thisPtr->arrowEntityX = newArrowEntityX;
			thisPtr->arrowEntityY = newArrowEntityY;
			thisPtr->arrowEntityZ = newArrowEntityZ;

			// Call setupFlag() and unHighlightGizmo() inside render command to avoid threading issues
			thisPtr->setupFlag();
			thisPtr->unHighlightGizmo();
		});
	}


	void Gizmo::setEnabled(bool enable)
	{
		// Update logic-side flag immediately
		this->enabled = enable;

		// Capture pointers and values needed on render thread
		auto arrowEntityX = this->arrowEntityX;
		auto arrowEntityY = this->arrowEntityY;
		auto arrowEntityZ = this->arrowEntityZ;
		auto sphereEntity = this->sphereEntity;
		auto arrowNodeX = this->arrowNodeX;
		auto arrowNodeY = this->arrowNodeY;
		auto arrowNodeZ = this->arrowNodeZ;
		auto sphereNode = this->sphereNode;
		auto firstPlaneNode = this->firstPlaneNode;
		auto secondPlaneNode = this->secondPlaneNode;
		auto thirdPlaneNode = this->thirdPlaneNode;
		auto defaultCategory = this->defaultCategory;
		auto debugPlane = this->debugPlane; // assumed member bool
		auto constraintAxis = this->constraintAxis;

		// Enqueue render commands for all render state changes
		ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::setEnabled", _15(arrowEntityX, arrowEntityY, arrowEntityZ, sphereEntity, arrowNodeX, arrowNodeY, arrowNodeZ, sphereNode,
				firstPlaneNode, secondPlaneNode, thirdPlaneNode, defaultCategory, debugPlane, constraintAxis, enable),
		{
			if (arrowEntityZ && arrowEntityZ->getParentSceneNode()) // safer check than hasMovableObject
			{
				if (enable)
				{
					if (arrowEntityX) arrowEntityX->setQueryFlags(defaultCategory);
					if (arrowEntityY) arrowEntityY->setQueryFlags(defaultCategory);
					if (arrowEntityZ) arrowEntityZ->setQueryFlags(defaultCategory);
					if (sphereEntity) sphereEntity->setQueryFlags(defaultCategory);

					if (arrowNodeX) arrowNodeX->setVisible(true);
					if (arrowNodeY) arrowNodeY->setVisible(true);
					if (arrowNodeZ) arrowNodeZ->setVisible(true);
					if (sphereNode) sphereNode->setVisible(true);

					if (debugPlane)
					{
						if (firstPlaneNode) firstPlaneNode->setVisible(true);
						if (secondPlaneNode) secondPlaneNode->setVisible(true);
						if (thirdPlaneNode) thirdPlaneNode->setVisible(true);
					}
				}
				else
				{
					if (arrowEntityX) arrowEntityX->setQueryFlags(0);
					if (arrowEntityY) arrowEntityY->setQueryFlags(0);
					if (arrowEntityZ) arrowEntityZ->setQueryFlags(0);
					if (sphereEntity) sphereEntity->setQueryFlags(0);

					if (arrowNodeX) arrowNodeX->setVisible(false);
					if (arrowNodeY) arrowNodeY->setVisible(false);
					if (arrowNodeZ) arrowNodeZ->setVisible(false);
					if (sphereNode) sphereNode->setVisible(false);

					// Since gizmoState affects logic, update outside render command if needed
					// Here just for safety, but you might prefer it outside the enqueue
					// You could store a pointer and update safely if thread-safe, or do it on logic thread

					if (debugPlane)
					{
						if (firstPlaneNode) firstPlaneNode->setVisible(false);
						if (secondPlaneNode) secondPlaneNode->setVisible(false);
						if (thirdPlaneNode) thirdPlaneNode->setVisible(false);
					}
				}
			}

			 // Hide arrows according to constraintAxis
			if (constraintAxis.x != 0.0f && arrowNodeX)
				arrowNodeX->setVisible(false);

			if (constraintAxis.y != 0.0f && arrowNodeY)
				arrowNodeY->setVisible(false);

			if (constraintAxis.z != 0.0f && arrowNodeZ)
				arrowNodeZ->setVisible(false);
		});

		// Since gizmoState is a logic variable, update it on logic thread directly (outside enqueue)
		if (false == enable)
		{
			this->gizmoState = GIZMO_NONE;
		}
	}


	bool Gizmo::isEnabled() const
	{
		return this->enabled;
	}

	void Gizmo::setSphereEnabled(bool enable)
	{
		auto sphereNode = this->sphereNode;
		ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::setSphereEnabled", _2(sphereNode, enable),
		{
			if (sphereNode)
			{
				sphereNode->setVisible(enable);
			}
		});
	}

	void Gizmo::highlightXArrow(void)
	{
		auto arrowEntityX = this->arrowEntityX;
		auto arrowEntityY = this->arrowEntityY;
		auto arrowEntityZ = this->arrowEntityZ;
		auto materialNameHighlight = this->materialNameHighlight;
		auto materialNameY = this->materialNameY;
		auto materialNameZ = this->materialNameZ;

		ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::highlightXArrow", _6(arrowEntityX, arrowEntityY, arrowEntityZ, materialNameHighlight, materialNameY, materialNameZ),
		{
			if (arrowEntityX) arrowEntityX->setDatablock(materialNameHighlight);
			if (arrowEntityY) arrowEntityY->setDatablock(materialNameY);
			if (arrowEntityZ) arrowEntityZ->setDatablock(materialNameZ);
		});

		// Update gizmoState safely on logic thread since it is not a render resource
		this->gizmoState = GIZMO_ARROW_X;
	}

	void Gizmo::highlightYArrow(void)
	{
		auto arrowEntityX = this->arrowEntityX;
		auto arrowEntityY = this->arrowEntityY;
		auto arrowEntityZ = this->arrowEntityZ;
		auto materialNameX = this->materialNameX;
		auto materialNameHighlight = this->materialNameHighlight;
		auto materialNameZ = this->materialNameZ;

		ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::highlightYArrow", _6(arrowEntityX, arrowEntityY, arrowEntityZ, materialNameX, materialNameHighlight, materialNameZ),
		{
			if (arrowEntityX) arrowEntityX->setDatablock(materialNameX);
			if (arrowEntityY) arrowEntityY->setDatablock(materialNameHighlight);
			if (arrowEntityZ) arrowEntityZ->setDatablock(materialNameZ);
		});

		this->gizmoState = GIZMO_ARROW_Y;
	}

	void Gizmo::highlightZArrow(void)
	{
		auto arrowEntityX = this->arrowEntityX;
		auto arrowEntityY = this->arrowEntityY;
		auto arrowEntityZ = this->arrowEntityZ;
		auto materialNameX = this->materialNameX;
		auto materialNameY = this->materialNameY;
		auto materialNameHighlight = this->materialNameHighlight;

		ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::highlightZArrow", _6(arrowEntityX, arrowEntityY, arrowEntityZ, materialNameX, materialNameY, materialNameHighlight),
		{
			if (arrowEntityX) arrowEntityX->setDatablock(materialNameX);
			if (arrowEntityY) arrowEntityY->setDatablock(materialNameY);
			if (arrowEntityZ) arrowEntityZ->setDatablock(materialNameHighlight);
		});

		this->gizmoState = GIZMO_ARROW_Z;
	}

	void Gizmo::highlightSphere(void)
	{
		auto sphereEntity = this->sphereEntity;
		auto materialNameHighlight = this->materialNameHighlight;

		ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::highlightSphere", _2(sphereEntity, materialNameHighlight),
		{
			if (sphereEntity) sphereEntity->setDatablock(materialNameHighlight);
		});

		this->gizmoState = GIZMO_SPHERE;
	}

	void Gizmo::unHighlightGizmo(void)
	{
		auto arrowEntityX = this->arrowEntityX;
		auto arrowEntityY = this->arrowEntityY;
		auto arrowEntityZ = this->arrowEntityZ;
		auto sphereEntity = this->sphereEntity;
		auto materialNameX = this->materialNameX;
		auto materialNameY = this->materialNameY;
		auto materialNameZ = this->materialNameZ;

		ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::unHighlightGizmo", _7(arrowEntityX, arrowEntityY, arrowEntityZ, sphereEntity, materialNameX, materialNameY, materialNameZ),
		{
			if (arrowEntityX) arrowEntityX->setDatablock(materialNameX);
			if (arrowEntityY) arrowEntityY->setDatablock(materialNameY);
			if (arrowEntityZ) arrowEntityZ->setDatablock(materialNameZ);
			if (sphereEntity) sphereEntity->setDatablock("WhiteNoLighting");
		});

		this->gizmoState = GIZMO_NONE;
	}


	void Gizmo::setupFlag(void)
	{
		// Capture pointers locally
		auto arrowEntityX = this->arrowEntityX;
		auto arrowEntityY = this->arrowEntityY;
		auto arrowEntityZ = this->arrowEntityZ;
		auto sphereEntity = this->sphereEntity;

		ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::setupFlag", _4(arrowEntityX, arrowEntityY, arrowEntityZ, sphereEntity),
		{
			if (arrowEntityX)
				arrowEntityX->setVisibilityFlags(NOWA::GIZMO_VISIBLE_MASK_VISIBLE);
			if (arrowEntityY)
				arrowEntityY->setVisibilityFlags(NOWA::GIZMO_VISIBLE_MASK_VISIBLE);
			if (arrowEntityZ)
				arrowEntityZ->setVisibilityFlags(NOWA::GIZMO_VISIBLE_MASK_VISIBLE);
			if (sphereEntity)
				sphereEntity->setVisibilityFlags(NOWA::GIZMO_VISIBLE_MASK_VISIBLE);
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
		auto closureFunction = [this, startPosition, endPosition, thickness, materialName](Ogre::Real renderDt)
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
				this->translationCaption->update();
			}
		};
		Ogre::String id = "Gizmo::drawLine";
		NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction);
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
		if (this->translationLineNode == nullptr)
		{
			return;
		}

		auto sceneManager = this->sceneManager;
		auto lineNode = this->translationLineNode;
		auto line = this->translationLine;

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("Destroy Gizmo Line", _3(sceneManager, lineNode, line),
		{
			lineNode->detachAllObjects();
			if (line)
			{
				sceneManager->destroyManualObject(line);
			}
			if (lineNode->getParentSceneNode())
			{
				lineNode->getParentSceneNode()->removeAndDestroyChild(lineNode);
			}
		});

		this->translationLine = nullptr;
		this->translationLineNode = nullptr;
	}

	void Gizmo::destroyCircle()
	{
		auto sceneManager = this->sceneManager;
		auto rotationCircle = this->rotationCircle;
		auto rotationCircleNode = this->rotationCircleNode;

		if (rotationCircleNode == nullptr)
			return;

		ENQUEUE_RENDER_COMMAND_MULTI("Gizmo::destroyCircle", _3(sceneManager, rotationCircle, rotationCircleNode),
		{
			rotationCircleNode->detachAllObjects();

			if (rotationCircle != nullptr)
			{
				sceneManager->destroyManualObject(rotationCircle);
				// Can't set Gizmo's rotationCircle pointer here safely,
				// because this lambda captures copies
			}

			// Remove and destroy the child node
			if (rotationCircleNode->getParentSceneNode())
			{
				rotationCircleNode->getParentSceneNode()->removeAndDestroyChild(rotationCircleNode);
			}
		});

		// After enqueueing the render command, clear your pointers on the main thread:
		this->rotationCircle = nullptr;
		this->rotationCircleNode = nullptr;
	}

	void Gizmo::createCircle(void)
	{
		ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::createCircle", _1(this),
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
		auto rotationCircle = this->rotationCircle;
		auto rotationCircleNode = this->rotationCircleNode;
		auto selectNode = this->selectNode;
		auto sphereNode = this->sphereNode;
		auto rotationCaption = this->rotationCaption;

		auto closureFunction = [rotationCircle, rotationCircleNode, selectNode, sphereNode, rotationCaption, fromAngle, toAngle, counterClockWise, thickness, materialName](Ogre::Real renderDt)
		{
			if (!rotationCircle || !rotationCircleNode || !selectNode || !sphereNode)
				return; // Safety check to avoid null pointers

			// Draw a circle for visual effect
			rotationCircle->clear();
			rotationCircle->begin(materialName, Ogre::OperationType::OT_TRIANGLE_LIST);

			Ogre::Real const radius = sphereNode->getScale().x * 5.0f;
			Ogre::Real const accuracy = 50.0f; // Number of plots for accuracy
			Ogre::Real const relativeThickness = sphereNode->getScale().x * thickness;
			unsigned int index = 0;
			int colourFactor = 1;
			Ogre::ColourValue value;

			if (true == counterClockWise)
			{
				for (Ogre::Real theta = fromAngle; theta < toAngle; theta += Ogre::Math::PI / accuracy)
				{
					colourFactor = static_cast<int>(Ogre::Math::Abs(theta / Ogre::Math::TWO_PI)) % 10;

					rotationCircle->position(radius * cos(theta), 0.0f, radius * sin(theta));
					// rotationCircle->colour(colourMap[colourFactor].value);
					rotationCircle->position(radius * cos(theta - Ogre::Math::PI / accuracy), 0.0f, radius * sin(theta - Ogre::Math::PI / accuracy));
					// rotationCircle->colour(colourMap[colourFactor].value);
					rotationCircle->position((radius - relativeThickness) * cos(theta - Ogre::Math::PI / accuracy), 0.0f, (radius - relativeThickness) * sin(theta - Ogre::Math::PI / accuracy));
					// rotationCircle->colour(colourMap[colourFactor].value);
					rotationCircle->position((radius - relativeThickness) * cos(theta), 0.0f, (radius - relativeThickness) * sin(theta));
					// rotationCircle->colour(colourMap[colourFactor].value);
					// Join the 4 vertices created above to form a quad.
					rotationCircle->quad(index, index + 1, index + 2, index + 3);
					index += 4;
				}
			}
			else
			{
				for (Ogre::Real theta = fromAngle; theta > toAngle; theta -= Ogre::Math::PI / accuracy)
				{
					colourFactor = static_cast<int>(Ogre::Math::Abs(theta / Ogre::Math::TWO_PI)) % 10;
					rotationCircle->position(radius * cos(theta), 0.0f, radius * sin(theta));
					// rotationCircle->colour(colourMap[colourFactor].value);
					rotationCircle->position(radius * cos(theta - Ogre::Math::PI / accuracy), 0.0f, radius * sin(theta - Ogre::Math::PI / accuracy));
					// rotationCircle->colour(colourMap[colourFactor].value);
					rotationCircle->position((radius - relativeThickness) * cos(theta - Ogre::Math::PI / accuracy), 0.0f, (radius - relativeThickness) * sin(theta - Ogre::Math::PI / accuracy));
					// rotationCircle->colour(colourMap[colourFactor].value);
					rotationCircle->position((radius - relativeThickness) * cos(theta), 0.0f, (radius - relativeThickness) * sin(theta));
					// rotationCircle->colour(colourMap[colourFactor].value);
					// Join the 4 vertices created above to form a quad.
					rotationCircle->quad(index, index + 1, index + 2, index + 3);
					index += 4;
				}
			}
			rotationCircle->end();
		};
		Ogre::String id = "Gizmo::drawCircle";
		NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction);

		// rotationCircleNode->setPosition(selectNode->_getDerivedPositionUpdated());
		// rotationCircleNode->setOrientation(orientation);
		NOWA::GraphicsModule::getInstance()->updateNodeTransform(this->rotationCircleNode, selectNode->_getDerivedPositionUpdated(), orientation);

		if (rotationCaption)
		{
			rotationCaption->update();
		}
	}

	void Gizmo::hideCircle(void)
	{
		auto rotationCircle = this->rotationCircle;
		auto rotationCaption = this->rotationCaption;

		ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::hideCircle", _2(rotationCircle, rotationCaption),
		{
			if (rotationCircle)
				rotationCircle->clear();

			if (rotationCaption)
				rotationCaption->setTitle("");
		});
	}


	void Gizmo::setTranslationCaption(const Ogre::String& caption, const Ogre::ColourValue& color)
	{
		auto translationCaption = this->translationCaption;

		ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::setTranslationCaption", _3(translationCaption, caption, color),
		{
			if (translationCaption)
			{
				translationCaption->setTitle(caption);
				translationCaption->setColor(color);
			}
		});
	}

	void Gizmo::setRotationCaption(const Ogre::String& caption, const Ogre::ColourValue& color)
	{
		auto rotationCaption = this->rotationCaption;

		ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::setRotationCaption", _3(rotationCaption, caption, color),
		{
			if (rotationCaption)
			{
				rotationCaption->setTitle(caption);
				rotationCaption->setColor(color);
			}
		});
	}

	void Gizmo::setPosition(const Ogre::Vector3& position)
	{
		if (this->constraintAxis == Ogre::Vector3::ZERO)
		{
			// this->selectNode->setPosition(position);
			NOWA::GraphicsModule::getInstance()->updateNodePosition(this->selectNode, position, false);
		}
		else
		{
			if (this->constraintAxis.x != 0.0f)
			{
				// this->selectNode->setPosition(Ogre::Vector3(this->constraintAxis.x, position.y, position.z));
				NOWA::GraphicsModule::getInstance()->updateNodePosition(this->selectNode, Ogre::Vector3(this->constraintAxis.x, position.y, position.z), false);
			}
			if (this->constraintAxis.y != 0.0f)
			{
				// this->selectNode->setPosition(Ogre::Vector3(position.x, this->constraintAxis.y, position.z));
				NOWA::GraphicsModule::getInstance()->updateNodePosition(this->selectNode, Ogre::Vector3(position.x, this->constraintAxis.y, position.z), false);
			}
			if (this->constraintAxis.z != 0.0f)
			{
				// this->selectNode->setPosition(Ogre::Vector3(position.x, position.y, this->constraintAxis.z));
				NOWA::GraphicsModule::getInstance()->updateNodePosition(this->selectNode, Ogre::Vector3(position.x, position.y, this->constraintAxis.z), false);
			}
		}
	}

	Ogre::Vector3 Gizmo::getPosition(void) const
	{
		return this->selectNode->_getDerivedPositionUpdated();
	}

	void Gizmo::setOrientation(const Ogre::Quaternion& orientation)
	{
		NOWA::GraphicsModule::getInstance()->updateNodeOrientation(this->selectNode, orientation, false/*, true*/);
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
		NOWA::GraphicsModule::getInstance()->updateNodePosition(this->selectNode, newPosition, false);
	}

	void Gizmo::rotate(const Ogre::Quaternion& rotateQuaternion)
	{
		Ogre::Quaternion newOrientation = this->selectNode->getOrientation() * rotateQuaternion;
		NOWA::GraphicsModule::getInstance()->updateNodeOrientation(this->selectNode, newOrientation, false);
	}

	void Gizmo::setConstraintAxis(const Ogre::Vector3& constraintAxis)
	{
		// Update the member variable immediately (logic thread)
		this->constraintAxis = constraintAxis;

		// Capture the nodes and constraintAxis for the render thread lambda
		auto arrowNodeX = this->arrowNodeX;
		auto arrowNodeY = this->arrowNodeY;
		auto arrowNodeZ = this->arrowNodeZ;
		auto constraintAxisCopy = constraintAxis; // capture by value

		ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::setConstraintAxis", _4(arrowNodeX, arrowNodeY, arrowNodeZ, constraintAxisCopy),
		{
			// Show all arrows first
			arrowNodeX->setVisible(true);
			arrowNodeY->setVisible(true);
			arrowNodeZ->setVisible(true);

			// Hide arrows according to constraintAxis
			if (constraintAxisCopy.x != 0.0f)
				arrowNodeX->setVisible(false);
			if (constraintAxisCopy.y != 0.0f)
				arrowNodeY->setVisible(false);
			if (constraintAxisCopy.z != 0.0f)
				arrowNodeZ->setVisible(false);
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