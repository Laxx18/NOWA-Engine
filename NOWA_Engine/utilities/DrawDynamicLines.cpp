#include "NOWAPrecompiled.h"
#include "DrawDynamicLines.hpp"
#include "DynamicLines.hpp"

namespace NOWA
{

    //------------------------------------------------------------------------------------------------
	DrawDynamicLines::DrawDynamicLines(Ogre::SceneManager* sceneManager)
		: objectMemoryManager(nullptr),
		dynamicLines(nullptr),
		sceneManager(sceneManager),
		swapBuffer(false),
		rendererisDone(true)
	{
		//scene.getGraphicsManager().getIntoRendererQueue().push([this, &scene](){
		//
		//	Ogre::LogManager::getSingleton().logMessage("RQ -> DrawDynamicLines::DrawDynamicLines");

		//	auto test = scene.getOgreSceneManager();
		//
		//	Ogre::SceneNode* node = test.__OgreSceneMgrPtr->getRootSceneNode(Ogre::SCENE_DYNAMIC)->createChildSceneNode(Ogre::SCENE_DYNAMIC);
		//	
		//	//Ogre::VaoManager*  vao = m_GraphicsManager.getRoot()->getRenderSystem()->getVaoManager();

		//	//_t_ObjectMemoryManager = new Ogre::ObjectMemoryManager();
		//	
		//	__t_DynamicLines = new DynamicLines(0, &test.__OgreSceneMgrPtr->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), test.__OgreSceneMgrPtr, Ogre::OperationType::OT_LINE_LIST);
		//	__t_DynamicLines->setCastShadows(false);
		//	node->attachObject(__t_DynamicLines);

		//	//---------------------------------------------------------------------------------------
		//	//
		//	//---------------------------------------------------------------------------------------
		//	//Ogre::ManualObject * manualObject = new Ogre::ManualObject(0, &test.__OgreSceneMgrPtr->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), test.__OgreSceneMgrPtr);

		//	//manualObject->begin("BaseWhite", Ogre::OperationType::OT_LINE_LIST);

		//	//// Back
		//	//manualObject->position(0.0f, 0.0f, 0.0f);
		//	//manualObject->position(0.0f, 1.0f, 0.0f);
		//	//manualObject->line(0, 1);

		//	//manualObject->position(0.0f, 1.0f, 0.0f);
		//	//manualObject->position(1.0f, 1.0f, 0.0f);
		//	//manualObject->line(2, 3);

		//	//manualObject->position(1.0f, 1.0f, 0.0f);
		//	//manualObject->position(1.0f, 0.0f, 0.0f);
		//	//manualObject->line(4, 5);

		//	//manualObject->position(1.0f, 0.0f, 0.0f);
		//	//manualObject->position(0.0f, 0.0f, 0.0f);
		//	//manualObject->line(6, 7);

		//	//// Front
		//	//manualObject->position(0.0f, 0.0f, 1.0f);
		//	//manualObject->position(0.0f, 10.0f, 1.0f);
		//	//manualObject->line(8, 9);

		//	//manualObject->position(0.0f, 1.0f, 1.0f);
		//	//manualObject->position(1.0f, 1.0f, 1.0f);
		//	//manualObject->line(10, 11);

		//	//manualObject->position(1.0f, 1.0f, 1.0f);
		//	//manualObject->position(1.0f, 0.0f, 1.0f);
		//	//manualObject->line(12, 13);

		//	//manualObject->position(1.0f, 0.0f, 1.0f);
		//	//manualObject->position(0.0f, 0.0f, 1.0f);
		//	//manualObject->line(14, 15);

		//	//// Sides
		//	//manualObject->position(0.0f, 0.0f, 0.0f);
		//	//manualObject->position(0.0f, 0.0f, 1.0f);
		//	//manualObject->line(16, 17);

		//	//manualObject->position(0.0f, 1.0f, 0.0f);
		//	//manualObject->position(0.0f, 1.0f, 1.0f);
		//	//manualObject->line(18, 19);

		//	//manualObject->position(1.0f, 0.0f, 0.0f);
		//	//manualObject->position(1.0f, 0.0f, 1.0f);
		//	//manualObject->line(20, 21);

		//	//manualObject->position(1.0f, 1.0f, 0.0f);
		//	//manualObject->position(1.0f, 1.0f, 1.0f);
		//	//manualObject->line(22, 23);

		//	//manualObject->end();

		//	//Ogre::SceneNode *sceneNodeLines = test.__OgreSceneMgrPtr->getRootSceneNode(Ogre::SCENE_DYNAMIC)->createChildSceneNode(Ogre::SCENE_DYNAMIC);
		//	//sceneNodeLines->attachObject(manualObject);


		////	sceneNodeLines->scale(4.0f, 4.0f, 4.0f);
		////	sceneNodeLines->translate(-4.5f, -1.5f, 0.0f, Ogre::SceneNode::TS_LOCAL);

		//});
	}

    //------------------------------------------------------------------------------------------------
	DrawDynamicLines::~DrawDynamicLines()
    {
		/*m_GraphicsManager.getIntoRendererQueue().push([this](){

			Ogre::MaterialManager::getSingleton().remove("BtOgre/DebugLines");
			Ogre::ResourceGroupManager::getSingleton().destroyResourceGroup("BtOgre");
			delete __t_DynamicLines;
		});*/
    }

    //------------------------------------------------------------------------------------------------
	void DrawDynamicLines::addPoint(const Ogre::Vector3& p)
    {
		if (this->swapBuffer)
			this->pointsBuf1.push_back(p);
		else
			this->pointsBuf2.push_back(p);
    }

    //------------------------------------------------------------------------------------------------
	void DrawDynamicLines::addPoint(float x, float y, float z)
    {
		if (this->swapBuffer)
			this->pointsBuf1.push_back(Ogre::Vector3(x, y, z));
		else
			this->pointsBuf2.push_back(Ogre::Vector3(x, y, z));
    }

    //------------------------------------------------------------------------------------------------
	void DrawDynamicLines::setPoint(unsigned short index, const Ogre::Vector3 &value)
    {
		if (this->swapBuffer)
			this->pointsBuf1[index] = value;
		else
			this->pointsBuf2[index] = value;
    }

    //------------------------------------------------------------------------------------------------
	void DrawDynamicLines::clear()
    {
		if (this->swapBuffer)
			this->pointsBuf1.clear();
		else
			this->pointsBuf2.clear();
    }

    //------------------------------------------------------------------------------------------------
	void DrawDynamicLines::update()
	{
		if (rendererisDone)
		{
			this->rendererisDone = false;

			bool tmp = this->swapBuffer;

			this->swapBuffer = this->swapBuffer ? false : true;
			//Ogre::LogManager::getSingleton().logMessage("RQ -> DrawDynamicLines::update");
			this->dynamicLines->points = tmp ? this->pointsBuf1 : this->pointsBuf2;
			this->dynamicLines->update();
			this->rendererisDone = true;
		}
	}
} // namespace end
