#include "NOWAPrecompiled.h"
#include "DesignState.h"
#include "GuiEvents.h"
#include "OgreFrameStats.h"
#include "MyGUIHelper.h"
#include "Procedural.h"

// Compile directly into project
#include "TreeControl/TreeControl.cpp"
#include "TreeControl/TreeControlItem.cpp"
#include "Slider/Slider.cpp"
#include "HyperTextBox/HyperTextBox.cpp"
#include "HyperTextBox/Panel.cpp"
#include "HyperTextBox/ScrollViewPanel.cpp"
#include "HyperTextBox/StackPanel.cpp"
#include "HyperTextBox/WrapPanel.cpp"

class SelectionObserver : public NOWA::SelectionManager::ISelectionObserver
{
public:
	SelectionObserver(NOWA::EditorManager* editorManager)
		: selectionStrategy(new NOWA::DefaultOutLine()), // = new NOWA::RimEffectOutLine();
		editorManager(editorManager)
	{
		
	}

	virtual ~SelectionObserver()
	{
		if (this->selectionStrategy)
		{
			delete this->selectionStrategy;
			this->selectionStrategy = nullptr;
		}
	}

	virtual void onHandleSelection(NOWA::GameObject* gameObject, bool selected) override
	{
		if (true == selected && NOWA::EditorManager::EDITOR_PICKER_MODE != this->editorManager->getManipulationMode())
		{
			this->selectionStrategy->highlight(gameObject);
		}
		else
		{
			this->selectionStrategy->unHighlight(gameObject);
		}
	}
private:
	NOWA::DefaultOutLine* selectionStrategy;
	NOWA::EditorManager* editorManager;
};

//////////////////////////////////////////////////////////////////////

DesignState::DesignState()
	: NOWA::AppState()
{

}

void DesignState::enter(void)
{
	this->hasStarted = true;
	this->canUpdate = true;

	this->projectManager = nullptr;
	this->editorManager = nullptr;
	this->simulating = false;
	this->propertiesPanel = nullptr;
	this->componentsPanel = nullptr;
	this->resourcesPanel = nullptr;
	this->mainMenuBar = nullptr;
	this->nextInfoUpdate = 1.0f;
	this->validScene = false;
	this->activeCategory = "All";
	this->cameraMoveSpeed = 10.0f;
	this->lastOrbitValue = Ogre::Vector2::ZERO;
	this->firstTimeValueSet = true;
	this->playerInControl = false;
	this->ogreNewt = nullptr;
	this->selectQuery = nullptr;

	// Register the tree control
	MyGUI::FactoryManager& factory = MyGUI::FactoryManager::getInstance();
	std::string widgetCategory = MyGUI::WidgetManager::getInstance().getCategoryName();

	// MyGUI::ResourceManager::getInstance().load("FrameworkFonts.xml");
	// MyGUI::ResourceManager::getInstance().load("NOWA_Design_Font.xml"); // does crash
	MyGUI::ResourceManager::getInstance().load("TreeControlSkin.xml");
	MyGUI::ResourceManager::getInstance().load("TreeControlTemplate.xml");
	factory.registerFactory<MyGUI::TreeControl>(widgetCategory);
	factory.registerFactory<MyGUI::TreeControlItem>(widgetCategory);

	// MyGUI::ResourceManager::getInstance().load("HyperTextFonts.xml");
	MyGUI::ResourceManager::getInstance().load("HyperTextSkins.xml");
	factory.registerFactory<MyGUI::HyperTextBox>(widgetCategory);
	factory.registerFactory<MyGUI::WrapPanel>(widgetCategory);
	factory.registerFactory<MyGUI::StackPanel>(widgetCategory);
	factory.registerFactory<MyGUI::ScrollViewPanel>(widgetCategory);

	MyGUI::ResourceManager::getInstance().load("SliderTemplate.xml");
	factory.registerFactory<MyGUI::Slider>(widgetCategory);

	// Load images for buttons
	MyGUI::ResourceManager::getInstance().load("ButtonsImages.xml");
	// Load skin for +- expand button for each panel cell
	MyGUI::ResourceManager::getInstance().load("ButtonExpandSkin.xml");
	MyGUI::ResourceManager::getInstance().load("Brushes.xml");
	MyGUI::ResourceManager::getInstance().load("MyGUI_NOWA_Images.xml");

	MyGUIHelper::getInstance()->initToolTipData();

	this->createScene();
}

void DesignState::exit(void)
{
	this->canUpdate = false;
	this->hasStarted = false;

	this->sceneManager->destroyQuery(this->selectQuery);
	this->selectQuery = nullptr;

	NOWA::Core::getSingletonPtr()->switchFullscreen(false, 0, 0, 0);

	if (nullptr != this->editorManager && true == this->simulating)
	{
		// Stop simulation, since there can be tag-point components involved which changed the scene node owner ship, so a crash may occur if a movable object is detached from its
		// origin node, but the object is attached to another one
		this->editorManager->stopSimulation();
	}
	MyGUI::FactoryManager& factory = MyGUI::FactoryManager::getInstance();
	std::string widgetCategory = MyGUI::WidgetManager::getInstance().getCategoryName();
	factory.unregisterFactory<MyGUI::TreeControl>(widgetCategory);
	factory.unregisterFactory<MyGUI::TreeControlItem>(widgetCategory);
	factory.unregisterFactory<MyGUI::Slider>(widgetCategory);
	factory.unregisterFactory<MyGUI::HyperTextBox>(widgetCategory);
	factory.unregisterFactory<MyGUI::WrapPanel>(widgetCategory);
	factory.unregisterFactory<MyGUI::StackPanel>(widgetCategory);
	factory.unregisterFactory<MyGUI::ScrollViewPanel>(widgetCategory);
	
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &DesignState::handleGenerateCategoriesDelegate), NOWA::EventDataGenerateCategories::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &DesignState::handleStopSimulation), NOWA::EventDataStopSimulation::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &DesignState::handleExit), EventDataExit::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &DesignState::handleProjectManipulation), EventDataProjectManipulation::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &DesignState::handleEditorMode), NOWA::EventDataEditorMode::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &DesignState::handleSceneValid), EventDataSceneValid::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &DesignState::handleFeedback), NOWA::EventDataFeedback::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &DesignState::handlePlayerInControl), NOWA::EventDataActivatePlayerController::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &DesignState::handleWorldLoaded), NOWA::EventDataWorldLoaded::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &DesignState::handleTestSelectedGameObjects), EventDataTestSelectedGameObjects::getStaticEventType());

	MyGUI::LayoutManager::getInstancePtr()->unloadLayout(this->widgetsSimulation);
	MyGUI::LayoutManager::getInstancePtr()->unloadLayout(this->widgetsManipulation);
	ColourPanelManager::getInstance()->destroyContent();

	if (this->editorManager)
	{
		delete this->editorManager;
		this->editorManager = nullptr;
	}

	if (this->propertiesPanel)
	{
		this->propertiesPanel->destroyContent();
		delete this->propertiesPanel;
		this->propertiesPanel = nullptr;
	}

	if (this->resourcesPanel)
	{
		this->resourcesPanel->destroyContent();
		delete this->resourcesPanel;
		this->resourcesPanel = nullptr;
	}

	if (this->componentsPanel)
	{
		this->componentsPanel->destroyContent();
		delete this->componentsPanel;
		this->componentsPanel = nullptr;
	}

	if (this->mainMenuBar)
	{
		delete this->mainMenuBar;
		this->mainMenuBar = nullptr;
	}

	if (this->projectManager)
	{
		delete this->projectManager;
		this->projectManager = nullptr;
	}

	AppState::destroyModules();
}

void DesignState::createScene(void)
{
	// constexpr size_t numThreads = 1;
	#ifdef _DEBUG
		//Debugging multithreaded code is a PITA, disable it.
		const size_t numThreads = 1;
	#else
		//getNumLogicalCores() may return 0 if couldn't detect
		const size_t numThreads = std::max<size_t>(1, Ogre::PlatformInformation::getNumLogicalCores());
	#endif
	// Create the SceneManager, in this case a generic one
	this->sceneManager = NOWA::Core::getSingletonPtr()->getOgreRoot()->createSceneManager(Ogre::ST_GENERIC, numThreads, "ExampleSMInstance");
	Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: Using " + Ogre::StringConverter::toString(numThreads) + " threads.");

	// http://www.ogre3d.org/2016/01/01/ogre-progress-report-december-2015
	// Longer loading times, but faster, test it
#ifndef _DEBUG
	// Causes crash when loading plants etc.
	// OGRE EXCEPTION(3:RenderingAPIException): The shader requires more input attributes/semantics than what the VertexArrayObject / v1::VertexDeclaration has to offer. You're missing a component in D3D11HLSLProgram::getLayoutForPso at h:\gameenginedevelopment2_2\external\ogre2.2sdk\rendersystems\direct3d11\src\ogred3d11hlslprogram.cpp (line 2104)
	// Ogre::v1::Mesh::msOptimizeForShadowMapping = true;
#endif

// Attention: Tests
	// this->sceneManager->setFlipCullingOnNegativeScale(true);

	this->camera = this->sceneManager->createCamera("GamePlayCamera");
	NOWA::Core::getSingletonPtr()->setMenuSettingsForCamera(this->camera);
	// this->camera->setAutoAspectRatio(true);;
	// this->camera->setFOVy(Ogre::Degree(90.f));
	// this->camera->setPosition(0.0f, 5.0f, -2.0f);

	this->camera->setPosition(this->camera->getParentSceneNode()->convertLocalToWorldPositionUpdated(Ogre::Vector3(0.0f, 5.0f, -2.0f)));

	this->camera->setNearClipDistance(0.1f);
	this->camera->setFarClipDistance(500.0f);
	this->camera->setQueryFlags(0 << 0);

	// Causes a realy shit mess: All objects are positioned by this offset and the selection cube is also at an different place, what the fuck :(
	// this->camera->getParentNode()->setPosition(0.0f, 5.0f, -2.0f);

	this->initializeModules(false, false);

	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &DesignState::handleGenerateCategoriesDelegate), NOWA::EventDataGenerateCategories::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &DesignState::handleStopSimulation), NOWA::EventDataStopSimulation::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &DesignState::handleExit), EventDataExit::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &DesignState::handleProjectManipulation), EventDataProjectManipulation::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &DesignState::handleEditorMode), NOWA::EventDataEditorMode::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &DesignState::handleSceneValid), EventDataSceneValid::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &DesignState::handleFeedback), NOWA::EventDataFeedback::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &DesignState::handlePlayerInControl), NOWA::EventDataActivatePlayerController::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &DesignState::handleWorldLoaded), NOWA::EventDataWorldLoaded::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &DesignState::handleTestSelectedGameObjects), EventDataTestSelectedGameObjects::getStaticEventType());

	//this->sceneManager->setAmbientLight(Ogre::ColourValue(0.3f, 0.5f, 0.7f) * 0.1f * 0.75f, Ogre::ColourValue(0.6f, 0.45f, 0.3f) * 0.065f * 0.75f, Ogre::Vector3(-1, -1, -1).normalisedCopy());
	////Set sane defaults for proper shadow mapping
	//this->sceneManager->setShadowDirectionalLightExtrusionDistance(500.0f);
	//this->sceneManager->setShadowFarDistance(500.0f);
	// http://www.ogre3d.org/forums/viewtopic.php?f=25&t=83081#p518819
	// http://www.ogre3d.org/forums/viewtopic.php?f=25&t=82878#p515450
	// this->sceneManager->setFog(Ogre::FogMode::FOG_LINEAR);
	// this->sceneManager->setFog(Ogre::FOG_EXP, Ogre::ColourValue::White, 0.1f);

	NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->init("CameraManager1", this->camera);
	auto baseCamera = new NOWA::BaseCamera(NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getCameraBehaviorId());
	NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->addCameraBehavior(baseCamera);
	NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setActiveCameraBehavior(baseCamera->getBehaviorType());

	this->projectManager = new ProjectManager(this->sceneManager);

	// Setup all MyGUI widgets
	this->setupMyGUIWidgets();

	this->selectQuery = this->sceneManager->createRayQuery(Ogre::Ray(), NOWA::GameObjectController::ALL_CATEGORIES_ID);
	this->selectQuery->setSortByDistance(true);

	// Attention: Dangerous test, as maybe textures are deleted, that are in use, check background scroll etc.!
	// Causes crash, when exiting application
	// NOWA::Core::getSingletonPtr()->minimizeMemory(this->sceneManager);
	// NOWA::Core::getSingletonPtr()->setTightMemoryBudget();

	//Ogre::v1::MeshPtr meshPtr =  Procedural::PlaneGenerator().setNumSegX(20).setNumSegY(20).setSizeX(150).setSizeY(150).setUTile(5.0).setVTile(5.0).realizeMesh("testMesh");
	//Ogre::v1::MeshPtr meshPtr =  Procedural::SphereGenerator().setRadius(2.f).setUTile(5.).setVTile(5.).realizeMesh("testMesh");
	//Ogre::v1::MeshPtr meshPtr = Procedural::CylinderGenerator().setHeight(3.f).setRadius(1.f).setUTile(3.).realizeMesh("testMesh");
	//Ogre::v1::MeshPtr meshPtr = Procedural::TorusGenerator().setRadius(3.f).setSectionRadius(1.f).setUTile(10.).setVTile(5.).realizeMesh("testMesh");
	//Ogre::v1::MeshPtr meshPtr = Procedural::ConeGenerator().setRadius(2.f).setHeight(3.f).setNumSegBase(36).setNumSegHeight(2).setUTile(3.).realizeMesh("testMesh");
	// Ogre::v1::MeshPtr meshPtr = Procedural::TubeGenerator().setHeight(3.f).setUTile(3.).realizeMesh("testMesh");
	//Ogre::v1::MeshPtr meshPtr = Procedural::BoxGenerator().setSizeX(2.0).setSizeY(4.f).setSizeZ(6.f).realizeMesh("testMesh");
	//Ogre::v1::MeshPtr meshPtr = Procedural::CapsuleGenerator().setHeight(2.f).realizeMesh("testMesh");
	//Ogre::v1::MeshPtr meshPtr = Procedural::TorusKnotGenerator().setRadius(2.f).setSectionRadius(.5f).setUTile(3.f).setNumSegCircle(64).setNumSegSection(16).realizeMesh("testMesh");
	//Ogre::v1::MeshPtr meshPtr = Procedural::IcoSphereGenerator().setRadius(2.).setNumIterations(3).setUTile(5.).setVTile(5.).realizeMesh("testMesh");
	// Ogre::v1::MeshPtr meshPtr = Procedural::RoundedBoxGenerator().setSizeX(1.f).setSizeY(5.f).setSizeZ(5.f).setChamferSize(1.f).realizeMesh("testMesh");
	// Ogre::v1::MeshPtr meshPtr = Procedural::SpringGenerator().setNumSegCircle(32).setNumSegPath(30).realizeMesh("testMesh");

	/*Procedural::Shape shape = Procedural::CircleShape().setRadius(1).setNumSeg(16).realizeShape();
	Procedural::Path line = Procedural::CatmullRomSpline3().addPoint(0, 0, -3).addPoint(0, 0, -1).addPoint(0, 0, 1).addPoint(-1, 0, 2).addPoint(-3, 0, 2).addPoint(-5, 0, 2).realizePath();
	Ogre::v1::MeshPtr meshPtr = Procedural::Extruder().setCapped(false).setShapeToExtrude(&shape).setExtrusionPath(&line).realizeMesh("testMesh");*/

	// -- Road
	//// The path of the road, generated from a simple spline
	//Procedural::Path p = Procedural::CatmullRomSpline3().setNumSeg(8).addPoint(0, 0, 0).addPoint(0, 0, 10).addPoint(10, 0, 10).addPoint(20, 0, 0).close().realizePath().scale(2);
	//// The shape that will be extruded along the path
	//Procedural::Shape s = Procedural::Shape().addPoint(-1.2f, .2f).addPoint(-1.f, .2f).addPoint(-.9f, .1f).addPoint(.9f, .1f).addPoint(1.f, .2f).addPoint(1.2f, .2f).scale(2).setOutSide(Procedural::SIDE_LEFT);
	//// This is an example use of a shape texture track,
	//// which specifies how texture coordinates should be mapped relative to the shape points
	//Procedural::Track textureTrack = Procedural::Track(Procedural::Track::AM_POINT).addKeyFrame(0, 0).addKeyFrame(2, .2f).addKeyFrame(3, .8f).addKeyFrame(5, 1);
	//// The extruder actually creates the road mesh from all parameters
	//Ogre::v1::MeshPtr meshPtr = Procedural::Extruder().setExtrusionPath(&p).setShapeToExtrude(&s).setShapeTextureTrack(&textureTrack).setUTile(20.f).realizeMesh("testMesh");

	//// -- Jarre
	////
	//Procedural::TriangleBuffer tb;
	//// Body
	//Procedural::Shape jarreShape = Procedural::CubicHermiteSpline2().addPoint(Ogre::Vector2(0, 0), Ogre::Vector2::UNIT_X, Ogre::Vector2::UNIT_X)
	//	.addPoint(Ogre::Vector2(2, 3))
	//	.addPoint(Ogre::Vector2(.5, 5), Ogre::Vector2(-1, 1).normalisedCopy(), Ogre::Vector2::UNIT_Y)
	//	.addPoint(Ogre::Vector2(1, 7), Ogre::Vector2(1, 1).normalisedCopy()).realizeShape().thicken(.1f).getShape(0);
	//Procedural::Lathe().setShapeToExtrude(&jarreShape).addToTriangleBuffer(tb);
	//// Handle 1
	//Procedural::Shape jarreHandleShape = Procedural::CircleShape().setRadius(.2f).realizeShape();
	//Procedural::Path jarreHandlePath = Procedural::CatmullRomSpline3().addPoint(Ogre::Vector3(0, 6.5f, .75f))
	//	.addPoint(Ogre::Vector3(0, 6, 1.5f))
	//	.addPoint(Ogre::Vector3(0, 5, .55f)).setNumSeg(10).realizePath();
	//Procedural::Extruder().setShapeToExtrude(&jarreHandleShape).setExtrusionPath(&jarreHandlePath).addToTriangleBuffer(tb);
	//// Handle2
	//jarreHandlePath.reflect(Ogre::Vector3::UNIT_Z);
	//Procedural::Extruder().setShapeToExtrude(&jarreHandleShape).setExtrusionPath(&jarreHandlePath).addToTriangleBuffer(tb);
	//Ogre::v1::MeshPtr meshPtr = tb.transformToMesh("testMesh");

	// -- Pillar
	// The path of the pillar, just a straight line
	//Procedural::Path pillarBodyPath = Procedural::LinePath().betweenPoints(Vector3(0, 0, 0), Vector3(0, 5, 0)).realizePath();
	//// We're doing something custom for the shape to extrude
	//Procedural::Shape pillarBodyShape;
	//const int pillarSegs = 64;
	//for (int i = 0; i<pillarSegs; i++)
	//	pillarBodyShape.addPoint(.5*(1 - .15*Math::Abs(Math::Sin(i / (float)pillarSegs*8.*Math::TWO_PI))) *
	//		Vector2(Math::Cos(i / (float)pillarSegs*Math::TWO_PI), Math::Sin(i / (float)pillarSegs*Math::TWO_PI)));
	//pillarBodyShape.close();
	//// We're also setting up a scale track, as traditionnal pillars are not perfectly straight
	//Procedural::Track pillarTrack = Procedural::CatmullRomSpline2().addPoint(0, 1).addPoint(0.5f, .95f).addPoint(1, .8f).realizeShape().convertToTrack(Procedural::Track::AM_RELATIVE_LINEIC);
	//// Creation of the pillar body
	//Procedural::TriangleBuffer pillarTB;
	//Procedural::Extruder().setExtrusionPath(&pillarBodyPath).setShapeToExtrude(&pillarBodyShape).setScaleTrack(&pillarTrack).setCapped(false).setPosition(0, 1, 0).addToTriangleBuffer(pillarTB);
	//// Creation of the top and the bottom of the pillar
	//Procedural::Shape s3 = Procedural::RoundedCornerSpline2().addPoint(-1, -.25f).addPoint(-1, .25f).addPoint(1, .25f).addPoint(1, -.25f).close().realizeShape().setOutSide(Procedural::SIDE_LEFT);
	//Procedural::Path p3;
	//for (int i = 0; i<32; i++)
	//{
	//	Ogre::Radian r = (Ogre::Radian) (Ogre::Math::HALF_PI - (float)i / 32.*Ogre::Math::TWO_PI);
	//	p3.addPoint(0, -.5 + .5*i / 32.*Math::Sin(r), -1 + .5*i / 32.*Math::Cos(r));
	//}
	//p3.addPoint(0, 0, -1).addPoint(0, 0, 1);
	//for (int i = 1; i<32; i++)
	//{
	//	Ogre::Radian r = (Ogre::Radian) (Ogre::Math::HALF_PI - (float)i / 32.*Ogre::Math::TWO_PI);
	//	p3.addPoint(0, -.5 + .5*(1 - i / 32.)*Math::Sin(r), 1 + .5*(1 - i / 32.)*Math::Cos(r));
	//}
	//Procedural::Extruder().setExtrusionPath(&p3).setShapeToExtrude(&s3).setPosition(0, 6., 0).addToTriangleBuffer(pillarTB);
	//Procedural::BoxGenerator().setPosition(0, .5, 0).addToTriangleBuffer(pillarTB);
	//Ogre::v1::MeshPtr meshPtr = pillarTB.transformToMesh("testMesh");

	//NOWA::MathHelper::getInstance()->ensureHasTangents(meshPtr);

	//Ogre::v1::Entity* entity = this->sceneManager->createEntity("testMesh");
	//Ogre::SceneNode* sceneNode = this->sceneManager->getRootSceneNode()->createChildSceneNode();
	//sceneNode->attachObject(entity);
	//sceneNode->setPosition(Ogre::Vector3::ZERO);
	//entity->setDatablockOrMaterialName("GroundDirtPlane");
	//entity->setCastShadows(true);

	NOWA::ProcessManager::getInstance()->attachProcess(NOWA::ProcessPtr(new NOWA::FaderProcess(NOWA::FaderProcess::FadeOperation::FADE_IN, 10.0f, 1.0f, 0.0f, 1.0f)));
}

void DesignState::setupMyGUIWidgets(void)
{
	// Creates the manipulation window
	{
		this->widgetsManipulation = MyGUI::LayoutManager::getInstancePtr()->loadLayout("ManipulationWindow.layout");
		MyGUI::FloatPoint windowPosition;
		windowPosition.left = 0.0f;
		windowPosition.top = 0.93f;
		
		this->manipulationWindow = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Window>("manipulationWindow");
		this->manipulationWindow->setRealCoord(MyGUI::FloatCoord(0.0f, 0.93f, 1.0f, 0.1f));
		this->manipulationWindow->setTextAlign(MyGUI::Align::Left);
		this->manipulationWindow->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());

		this->gridButton = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("gridButton");
		this->gridValueComboBox = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ComboBox>("gridValueComboBox");
		this->gridValueComboBox->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
		this->categoriesComboBox = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ComboBox>("categoriesComboBox");
		this->categoriesComboBox->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
		this->selectModeCheck = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("selectModeCheck");
		this->placeModeCheck = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("placeModeCheck");
		this->translateModeCheck = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("translateModeCheck");
		this->pickModeCheck = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("pickModeCheck");
		this->scaleModeCheck = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("scaleModeCheck");
		this->rotate1ModeCheck = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("rotate1ModeCheck");
		this->rotate2ModeCheck = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("rotate2ModeCheck");
		this->terrainModifyModeCheck = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("terrainModifyModeCheck");
		this->terrainSmoothModeCheck = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("terrainSmoothModeCheck");
		this->terrainPaintModeCheck = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("terrainPaintModeCheck");

		this->wakeButton = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("wakeButton");
		this->sleepButton = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("sleepButton");
		this->removeButton = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("removeButton");
		this->copyButton = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("copyButton");
		this->focusButton = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("focusButton");
		this->findObjectEdit = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("findObjectEdit");
		this->findObjectEdit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
		this->findObjectEdit->setNeedKeyFocus(true);
		this->findObjectEdit->setNeedMouseFocus(true);
		this->constraintAxisEdit = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("constraintAxisEdit");
		this->constraintAxisEdit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
		this->constraintAxisEdit->setNeedKeyFocus(true);
		this->constraintAxisEdit->setNeedMouseFocus(true);

		this->cameraResetButton = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("cameraResetButton");
		this->gridButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		this->gridButton->eventMouseSetFocus += MyGUI::newDelegate(this, &DesignState::setFocus);
		this->categoriesComboBox->eventComboChangePosition += MyGUI::newDelegate(this, &DesignState::itemSelected);
		this->categoriesComboBox->eventEditSelectAccept += MyGUI::newDelegate(this, &DesignState::notifyEditSelectAccept);
		this->categoriesComboBox->eventMouseSetFocus += MyGUI::newDelegate(this, &DesignState::setFocus);
		this->gridValueComboBox->eventComboChangePosition += MyGUI::newDelegate(this, &DesignState::itemSelected);
		this->gridValueComboBox->eventMouseSetFocus += MyGUI::newDelegate(this, &DesignState::setFocus);
		this->selectModeCheck->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		this->selectModeCheck->eventMouseSetFocus += MyGUI::newDelegate(this, &DesignState::setFocus);
		this->translateModeCheck->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		this->translateModeCheck->eventMouseSetFocus += MyGUI::newDelegate(this, &DesignState::setFocus);
		this->placeModeCheck->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		this->placeModeCheck->eventMouseSetFocus += MyGUI::newDelegate(this, &DesignState::setFocus);
		this->pickModeCheck->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		this->pickModeCheck->eventMouseSetFocus += MyGUI::newDelegate(this, &DesignState::setFocus);
		this->scaleModeCheck->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		this->scaleModeCheck->eventMouseSetFocus += MyGUI::newDelegate(this, &DesignState::setFocus);
		this->rotate1ModeCheck->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		this->rotate1ModeCheck->eventMouseSetFocus += MyGUI::newDelegate(this, &DesignState::setFocus);
		this->rotate2ModeCheck->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		this->rotate2ModeCheck->eventMouseSetFocus += MyGUI::newDelegate(this, &DesignState::setFocus);
		this->terrainModifyModeCheck->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		this->terrainModifyModeCheck->eventMouseSetFocus += MyGUI::newDelegate(this, &DesignState::setFocus);
		this->terrainSmoothModeCheck->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		this->terrainSmoothModeCheck->eventMouseSetFocus += MyGUI::newDelegate(this, &DesignState::setFocus);
		this->terrainPaintModeCheck->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		this->terrainPaintModeCheck->eventMouseSetFocus += MyGUI::newDelegate(this, &DesignState::setFocus);
		this->wakeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		this->wakeButton->eventMouseSetFocus += MyGUI::newDelegate(this, &DesignState::setFocus);
		this->sleepButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		this->sleepButton->eventMouseSetFocus += MyGUI::newDelegate(this, &DesignState::setFocus);
		this->removeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		this->removeButton->eventMouseSetFocus += MyGUI::newDelegate(this, &DesignState::setFocus);
		this->copyButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		this->copyButton->eventMouseSetFocus += MyGUI::newDelegate(this, &DesignState::setFocus);
		this->focusButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		this->focusButton->eventMouseSetFocus += MyGUI::newDelegate(this, &DesignState::setFocus);
		this->findObjectEdit->eventEditSelectAccept += MyGUI::newDelegate(this, &DesignState::notifyEditSelectAccept);
		this->findObjectEdit->eventMouseSetFocus += MyGUI::newDelegate(this, &DesignState::setFocus);
		this->constraintAxisEdit->eventEditSelectAccept += MyGUI::newDelegate(this, &DesignState::notifyEditSelectAccept);
		this->constraintAxisEdit->eventMouseSetFocus += MyGUI::newDelegate(this, &DesignState::setFocus);

		this->cameraResetButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		this->cameraResetButton->eventMouseSetFocus += MyGUI::newDelegate(this, &DesignState::setFocus);

		this->gridButton->setUserString("Description", "Show grid");
		this->gridValueComboBox->setUserString("Description", "Set grid value");
		this->categoriesComboBox->setUserString("Description", "Selectable category. Its possible to combine categories.");
		this->selectModeCheck->setUserString("Description", "Select objects");
		this->translateModeCheck->setUserString("Description", "Translate selectedobjects");
		this->placeModeCheck->setUserString("Description", "Place object");
		this->pickModeCheck->setUserString("Description", "Pick object physically (simulation will be activated)");
		this->scaleModeCheck->setUserString("Description", "Scale selectedobjects");
		this->rotate1ModeCheck->setUserString("Description", "Each object own rotation");
		this->rotate2ModeCheck->setUserString("Description", "All object rotation around center");
		this->terrainModifyModeCheck->setUserString("Description", "Modifies the terrain");
		this->terrainSmoothModeCheck->setUserString("Description", "Smooths the terrain");
		this->terrainPaintModeCheck->setUserString("Description", "Paints on the terrain");

		this->sleepButton->setUserString("Description", "Set objects sleep for physics");
		this->wakeButton->setUserString("Description", "Wake select objects for physics");
		this->removeButton->setUserString("Description", "Remove selectedobjects");
		this->copyButton->setUserString("Description", "Copy selected objects");
		this->focusButton->setUserString("Description", "Focus selectedobject");
		this->findObjectEdit->setUserString("Description", "Find object");
		this->constraintAxisEdit->setUserString("Description", "Constraint axes, any axe that is not 0, will be constraint to that value");
		this->cameraResetButton->setUserString("Description", "Reset camera pos and orient (holding shift will only reset orient");
	}

	// Creates the simulation window
	{
		MyGUI::IntPoint windowPosition;
		this->widgetsSimulation = MyGUI::LayoutManager::getInstancePtr()->loadLayout("SimulationWindow.layout");
		windowPosition.left = 500;
		windowPosition.top = 0;
		this->simulationWindow = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Window>("simulationWindow");
		this->simulationWindow->setPosition(windowPosition);
		this->simulationWindow->getCaptionWidget()->setTextColour(MyGUIHelper::getInstance()->getImportantTextColour());
		this->simulationWindow->getCaptionWidget()->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::mouseClicked);

		this->playButton = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("playButton");
		this->playButton->setStateSelected(true);
		this->undoButton = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("undoButton");
		this->undoButton->setEnabled(false);
		this->redoButton = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("redoButton");
		this->redoButton->setEnabled(false);
		this->cameraUndoButton = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("cameraUndoButton");
		this->cameraUndoButton->setEnabled(false);
		this->cameraRedoButton = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("cameraRedoButton");
		this->cameraRedoButton->setEnabled(false);
		this->selectUndoButton = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("selectUndoButton");
		this->selectUndoButton->setEnabled(false);
		this->selectRedoButton = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("selectRedoButton");
		this->selectRedoButton->setEnabled(false);

		this->playButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		this->undoButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		this->redoButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		this->cameraUndoButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		this->cameraRedoButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		this->selectUndoButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		this->selectRedoButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
	}

	// MyGUI::EditPtr actionLabel = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::EditBox>("EditBoxEmpty", 0.6f, 0.9f, 0.3f, 0.05f, MyGUI::Align::Default, "Main", "DebugLabel");

	// Creates the popup menu for place modes
	{
		this->placeModePopupMenu = MyGUI::Gui::getInstancePtr()->createWidget<MyGUI::PopupMenu>(/*MyGUI::WidgetStyle::Popup, */"PopupMenu",
		this->placeModeCheck->getAbsoluteCoord() + MyGUI::IntCoord(0, -100, 0, 0), MyGUI::Align::Default, "Popup", "placePopupMenu");
		MyGUI::MenuItem* placeItem1 = this->placeModePopupMenu->addItem("placeItem1"/*, MyGUI::MenuItemType::Popup*/);
		placeItem1->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
		placeItem1->setCaption("Normal");
		placeItem1->hideItemChild();
		placeItem1->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		MyGUI::MenuItem* placeItem2 = this->placeModePopupMenu->addItem("placeItem2"/*, MyGUI::MenuItemType::Popup*/);
		placeItem2->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
		placeItem2->setCaptionWithReplacing("#{Stack}");
		placeItem2->hideItemChild();
		placeItem2->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		MyGUI::MenuItem* placeItem3 = this->placeModePopupMenu->addItem("placeItem3"/*, MyGUI::MenuItemType::Popup*/);
		placeItem3->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
		placeItem3->setCaptionWithReplacing("#{StackOrientated}");
		placeItem3->hideItemChild();
		placeItem3->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);

		// Set at button pos, but below the button
		this->placeModePopupMenu->hideMenu();
	}

	// Creates the popup menu for translate modes
	{
		this->translateModePopupMenu = MyGUI::Gui::getInstancePtr()->createWidget<MyGUI::PopupMenu>(/*MyGUI::WidgetStyle::Popup, */"PopupMenu",
			this->translateModeCheck->getAbsoluteCoord() + MyGUI::IntCoord(0, -100, 0, 0), MyGUI::Align::Default, "Popup", "translatePopupMenu");
		MyGUI::MenuItem* translateItem1 = this->translateModePopupMenu->addItem("translateItem1"/*, MyGUI::MenuItemType::Popup*/);
		translateItem1->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
		translateItem1->setCaption("Normal");
		translateItem1->hideItemChild();
		translateItem1->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		MyGUI::MenuItem* translateItem2 = this->translateModePopupMenu->addItem("translateItem2"/*, MyGUI::MenuItemType::Popup*/);
		translateItem2->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
		translateItem2->setCaptionWithReplacing("#{Stack}");
		translateItem2->hideItemChild();
		translateItem2->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);
		MyGUI::MenuItem* translateItem3 = this->translateModePopupMenu->addItem("translateItem3"/*, MyGUI::MenuItemType::Popup*/);
		translateItem3->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
		translateItem3->setCaptionWithReplacing("#{StackOrientated}");
		translateItem3->hideItemChild();
		translateItem3->eventMouseButtonClick += MyGUI::newDelegate(this, &DesignState::buttonHit);

		// Set at button pos, but below the button
		this->translateModePopupMenu->hideMenu();
	}

	ColourPanelManager::getInstance()->init();

	// Creates the main menu bar
	{
		this->mainMenuBar = new MainMenuBar(this->projectManager);
	}

	// Disable everything
	this->enableWidgets(false);
	this->simulationWindow->setVisible(false);
	this->manipulationWindow->setVisible(false);
}

void DesignState::wakeSleepGameObjects(bool allGameObjects, bool sleep)
{
	if (true == allGameObjects)
	{
		const auto& gameObjects = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();
		for (auto& it = gameObjects->begin(); it != gameObjects->end(); it++)
		{
			auto& gameObject = it->second;
			auto& physicsActiveComponent = NOWA::makeStrongPtr(gameObject->getComponent<NOWA::PhysicsActiveComponent>());
			if (nullptr != physicsActiveComponent)
			{
				physicsActiveComponent->setActivated(!sleep);
			}
		}
	}
	else
	{
		const auto& gameObjects = this->editorManager->getSelectionManager()->getSelectedGameObjects();
		for (auto& it = gameObjects.begin(); it != gameObjects.end(); it++)
		{
			auto& gameObject = it->second.gameObject;
			auto& physicsActiveComponent = NOWA::makeStrongPtr(gameObject->getComponent<NOWA::PhysicsActiveComponent>());
			if (nullptr != physicsActiveComponent)
			{
				physicsActiveComponent->setActivated(!sleep);
			}
		}
	}
	this->propertiesPanel->showProperties();
}

void DesignState::enableWidgets(bool enable)
{
	this->playButton->setStateSelected(enable);
	this->undoButton->setEnabled(enable);
	this->redoButton->setEnabled(enable);
	this->cameraUndoButton->setEnabled(enable);
	this->cameraRedoButton->setEnabled(enable);
	this->selectUndoButton->setEnabled(enable);
	this->selectRedoButton->setEnabled(enable);
	this->gridButton->setEnabled(enable);
	this->selectModeCheck->setEnabled(enable);
	this->placeModeCheck->setEnabled(enable);
	// this->wakeButton->setEnabled(enable);
	// this->sleepButton->setEnabled(enable);
	this->removeButton->setEnabled(enable);
	this->copyButton->setEnabled(enable);
	
	if (nullptr != this->propertiesPanel)
	{
		this->propertiesPanel->setVisible(enable);
	}
	if (nullptr != this->resourcesPanel)
	{
		this->resourcesPanel->setVisible(enable);
	}
	/*if (nullptr != this->simulationWindow)
	{
		this->simulationWindow->setVisible(enable);
	}*/
	/*if (nullptr != this->manipulationWindow)
	{
		this->manipulationWindow->setVisible(enable);
	}*/
}

void DesignState::simulate(bool pause, bool withUndo)
{
	// NOWA::Core::getSingletonPtr()->switchFullscreen(!pause, 0, 0, 0);

	this->mainMenuBar->enableFileMenu(pause);

	if (false == pause)
	{
		// Always first save before starting simulation, so that newest values are set, for later resetVariants etc.
		if (nullptr != this->projectManager)
		{
			// this->projectManager->saveProject();
		}

		if (false == NOWA::Core::getSingletonPtr()->getIsGame())
		{
			NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->snapshotGameObjects();
		}

		MyGUI::LayerManager::getInstance().detachFromLayer(this->manipulationWindow);
		MyGUI::LayerManager::getInstance().attachToLayerNode("Back", this->manipulationWindow);
		this->mainMenuBar->clearLuaErrors();
		// Internally a snapshot is made before anything is changed in simulation, to get the state back, when stopping the simulation
		// this->editorManager->stopSimulation(); Never do this!!!! Else because of internal undo, all set values are reset and fancy behavior will start!
		this->editorManager->startSimulation();
		this->playButton->setImageResource("StopImage");
		this->simulating = true;
		this->enableWidgets(false);
		this->editorManager->setViewportGridEnabled(false);
		this->editorManager->getGizmo()->setEnabled(false);

		// Strangly the sleep state must be actualized, according to the current sleep state of each game object, else a simulation will only work once
		// After that the user must click sleep and un-sleep
		const auto& gameObjects = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();
		for (auto& it = gameObjects->begin(); it != gameObjects->end(); it++)
		{
			auto& gameObject = it->second;
			auto& physicsActiveComponent = NOWA::makeStrongPtr(gameObject->getComponent<NOWA::PhysicsActiveComponent>());
			if (nullptr != physicsActiveComponent)
			{
				physicsActiveComponent->setActivated(physicsActiveComponent->isActivated());
				// physicsActiveComponent->resetForce();
				// Causes huge performance impact and high wire when user newton player controller!
				// physicsActiveComponent->setVelocity(Ogre::Vector3::ZERO);
			}
		}
	}
	else
	{
		MyGUI::LayerManager::getInstance().detachFromLayer(this->manipulationWindow);
		MyGUI::LayerManager::getInstance().attachToLayerNode("Popup", this->manipulationWindow);
		// Must be called first, so that in case of lua error, no update is called
		this->simulating = false;
		// this->ogreNewt->update(this->ogreNewt->getUpdateFPS());
		if (nullptr != editorManager)
		{
			this->editorManager->setManipulationMode(NOWA::EditorManager::EDITOR_SELECT_MODE);
			// Set the state before the simulation began
			this->editorManager->stopSimulation(withUndo);
			// Show panels
			this->playButton->setImageResource("PlayImage");
			this->playButton->setStateSelected(true);
			// this->simulationButton->setCaptionWithReplacing(MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{Pause}"));
			this->enableWidgets(true);

			this->mainMenuBar->activateTestSelectedGameObjects(false);

			boost::shared_ptr<EventDataRefreshResourcesPanel> eventDataRefreshResourcesPanel(new EventDataRefreshResourcesPanel());
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataRefreshResourcesPanel, 3.0f);
		}
	}

	// this->mainMenuBar->setVisible(pause);
}

void DesignState::generateCategories(void)
{
	this->categoriesComboBox->deleteAllItems();
	std::vector<Ogre::String> allCategories = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getAllCategoriesSoFar();
	for (auto& category : allCategories)
	{
		this->categoriesComboBox->addItem(category);
	}
	this->categoriesComboBox->addItem("All");
	Ogre::String selectedCategory = this->categoriesComboBox->getItemNameAt(this->categoriesComboBox->findItemIndexWith("All"));
	this->categoriesComboBox->setCaption(selectedCategory);
}

// Delegates handling

void DesignState::handleGenerateCategoriesDelegate(NOWA::EventDataPtr eventData)
{
	this->generateCategories();
}

void DesignState::handleExit(NOWA::EventDataPtr eventData)
{
	this->bQuit = true;
}

void DesignState::handleStopSimulation(NOWA::EventDataPtr eventData)
{
	boost::shared_ptr<NOWA::EventDataStopSimulation> castEventData = boost::static_pointer_cast<NOWA::EventDataStopSimulation>(eventData);

	// true = pause
	this->simulate(true, true);

	// this->luaErrorInSimulation = true;

	// MyGUI::Message* messageBox = MyGUI::Message::createMessageBox("Runtime Error", MyGUI::LanguageManager::getInstancePtr()->replaceTags(castEventData->getFeedbackMessage()),
	// 		MyGUI::MessageBoxStyle::IconWarning | MyGUI::MessageBoxStyle::Ok, "Popup", true);
}

void DesignState::handleProjectManipulation(NOWA::EventDataPtr eventData)
{
	boost::shared_ptr<EventDataProjectManipulation> castEventData = boost::static_pointer_cast<EventDataProjectManipulation>(eventData);

	ProjectManager::eProjectMode projectMode = static_cast<ProjectManager::eProjectMode>(castEventData->getMode());
	// When not save scene
	if (ProjectManager::eProjectMode::SAVE != projectMode)
	{
		this->camera = NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
		this->ogreNewt = NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->getOgreNewt();
		this->validScene = true;
		this->mainMenuBar->enableMenuEntries(this->validScene);
		this->mainMenuBar->drawNavigationMap(false);
		NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->enableOgreNewtCollisionLines(this->sceneManager, false);

		if (nullptr != this->editorManager)
		{
			delete this->editorManager;
		}
		
		// Editor manager init
		this->editorManager = new NOWA::EditorManager();
		this->editorManager->init(this->sceneManager, this->camera, Ogre::String("All"), OIS::MB_Left, new SelectionObserver(this->editorManager));
		this->editorManager->setManipulationMode(NOWA::EditorManager::EDITOR_SELECT_MODE);
		// Do not push commands for picker mode, since a simulation is active
		this->editorManager->setUseUndoRedoForPicker(false);
		this->generateCategories();

		this->projectManager->setEditorManager(this->editorManager);
		// When a new world has been loaded e.g. via GameProgressModule, the project manager must get the new ogre newt and must not work with the old one!
		this->projectManager->setOgreNewt(this->ogreNewt);

		// Creates the properties panel
		{
			if (nullptr == this->propertiesPanel)
			{
				this->propertiesPanel = new PropertiesPanel(MyGUI::FloatCoord(0.78f, 0.0f, 0.23f, 0.93f));
			}
			this->propertiesPanel->clearProperties();
			// Actualize the editor manager pointer, because e.g. when a new world is created, the editor manager will be destroyed first, so the pointer is no more valid
			this->propertiesPanel->setEditorManager(this->editorManager);
		}

		// Creates the resources panel
		{
			if (nullptr == this->resourcesPanel)
			{
				this->resourcesPanel = new ResourcesPanel(MyGUI::FloatCoord(0.0f, 0.03f, 0.18f, 0.90f));
			}
			this->resourcesPanel->clear();
			// Actualize the editor manager pointer, because e.g. when a new world is created, the editor manager will be destroyed first, so the pointer is no more valid
			this->resourcesPanel->setEditorManager(this->editorManager);
		}

		// Creates the components panel
		{
			if (nullptr == this->componentsPanel)
			{
				this->componentsPanel = new ComponentsPanel(MyGUI::FloatCoord(0.6f, 0.03f, 0.18f, 0.90f));
			}
			// Actualize the editor manager pointer, because e.g. when a new world is created, the editor manager will be destroyed first, so the pointer is no more valid
			this->componentsPanel->setEditorManager(this->editorManager);
			this->componentsPanel->setVisible(false);
		}

		this->enableWidgets(true);
		this->simulationWindow->setVisible(true);
		this->manipulationWindow->setVisible(true);
		this->simulationWindow->setCaption(NOWA::Core::getSingletonPtr()->getWorldName());

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, NOWA::Core::getSingletonPtr()->dumpGraphicsMemory());
	}
}

void DesignState::handleEditorMode(NOWA::EventDataPtr eventData)
{
	boost::shared_ptr<NOWA::EventDataEditorMode> castEventData = boost::static_pointer_cast<NOWA::EventDataEditorMode>(eventData);

	if (NOWA::EditorManager::EDITOR_SELECT_MODE == castEventData->getManipulationMode())
	{
		this->selectModeCheck->setStateSelected(true);
		this->placeModeCheck->setStateSelected(false);
		this->translateModeCheck->setStateSelected(false);
		this->pickModeCheck->setStateSelected(false);
		this->scaleModeCheck->setStateSelected(false);
		this->rotate1ModeCheck->setStateSelected(false);
		this->rotate2ModeCheck->setStateSelected(false);
		this->terrainSmoothModeCheck->setStateSelected(false);
		this->terrainModifyModeCheck->setStateSelected(false);
		this->terrainPaintModeCheck->setStateSelected(false);
	}
	else if (NOWA::EditorManager::EDITOR_PLACE_MODE == castEventData->getManipulationMode())
	{
		this->selectModeCheck->setStateSelected(false);
		this->placeModeCheck->setStateSelected(true);
		this->translateModeCheck->setStateSelected(false);
		this->pickModeCheck->setStateSelected(false);
		this->scaleModeCheck->setStateSelected(false);
		this->rotate1ModeCheck->setStateSelected(false);
		this->rotate2ModeCheck->setStateSelected(false);
		this->terrainSmoothModeCheck->setStateSelected(false);
		this->terrainModifyModeCheck->setStateSelected(false);
		this->terrainPaintModeCheck->setStateSelected(false);
	}
}

void DesignState::handleSceneValid(NOWA::EventDataPtr eventData)
{
	boost::shared_ptr<EventDataSceneValid> castEventData = boost::static_pointer_cast<EventDataSceneValid>(eventData);
	this->validScene = castEventData->getSceneValid();
}

void DesignState::handleFeedback(NOWA::EventDataPtr eventData)
{
	boost::shared_ptr<NOWA::EventDataFeedback> castEventData = boost::static_pointer_cast<NOWA::EventDataFeedback>(eventData);

	if (false == castEventData->isPositive())
	{
		MyGUI::Message* messageBox = MyGUI::Message::createMessageBox("Feedback", MyGUI::LanguageManager::getInstancePtr()->replaceTags(castEventData->getFeedbackMessage()),
			MyGUI::MessageBoxStyle::IconWarning | MyGUI::MessageBoxStyle::Ok, "Popup", true);
	}
}

void DesignState::handlePlayerInControl(NOWA::EventDataPtr eventData)
{
	boost::shared_ptr<NOWA::EventDataActivatePlayerController> castEventData = boost::static_pointer_cast<NOWA::EventDataActivatePlayerController>(eventData);

	this->playerInControl = castEventData->getIsActive();
}

void DesignState::handleWorldLoaded(NOWA::EventDataPtr eventData)
{
	boost::shared_ptr<NOWA::EventDataWorldLoaded> castEventData = boost::static_pointer_cast<NOWA::EventDataWorldLoaded>(eventData);

	// Event not for this state
	if (castEventData->getSceneParameter().appStateName != this->appStateName)
	{
		return;
	}
	
	boost::shared_ptr<EventDataProjectManipulation> eventDataProjectManipulation(new EventDataProjectManipulation(ProjectManager::eProjectMode::NEW));
	this->handleProjectManipulation(eventDataProjectManipulation);

	// Note: When world has been changed, send that flag. E.g. in DesignState, if true, also call GameObjectController::start, so that when in simulation
	// and the world has been changed, remain in simulation and maybe activate player controller, so that the player may continue his game play
	if (true == castEventData->getWorldChanged())
	{
		this->simulate(false, false); // With undo false, how to solve this: when stopping, that the old world is loaded?

		// Set project parameter for project manager when a new world has been loaded in running simulation
		if (nullptr != this->projectManager)
		{
			this->projectManager->setProjectParameter(castEventData->getProjectParameter());
		}

		NOWA::AppStateManager::getSingletonPtr()->getGameProgressModule()->determinePlayerStartLocation(castEventData->getProjectParameter().sceneName);

		NOWA::GameObjectPtr player = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromName(NOWA::AppStateManager::getSingletonPtr()->getGameProgressModule()->getPlayerName());
		if (nullptr != player)
		{
			NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->activatePlayerController(true, player->getId(), true);
		}
	}
}

void DesignState::handleTestSelectedGameObjects(NOWA::EventDataPtr eventData)
{
	boost::shared_ptr<EventDataTestSelectedGameObjects> castEventData = boost::static_pointer_cast<EventDataTestSelectedGameObjects>(eventData);
	this->simulate(!castEventData->isActive(), true);
}

void DesignState::itemSelected(MyGUI::ComboBox* sender, size_t index)
{
	if (this->categoriesComboBox == sender)
	{
		if (index <= categoriesComboBox->getItemCount())
		{
			this->activeCategory = this->categoriesComboBox->getItemNameAt(index);
			this->editorManager->getSelectionManager()->filterCategories(this->activeCategory);
		}
	}
	else if (this->gridValueComboBox == sender)
	{
		if (index <= gridValueComboBox->getItemCount())
		{
			Ogre::String selectedGridValue = this->gridValueComboBox->getItemNameAt(index);
			this->editorManager->setGridStep(Ogre::StringConverter::parseReal(selectedGridValue));
		}
	}
}

void DesignState::notifyEditSelectAccept(MyGUI::EditBox* sender)
{
	// Its possible to create a custom complex combination of categories, so set the text for selections
	if (sender == this->categoriesComboBox)
	{
		this->activeCategory = this->categoriesComboBox->getOnlyText();
		this->editorManager->getSelectionManager()->filterCategories(this->activeCategory);
	}
	else if (sender == this->findObjectEdit)
	{
		// Find game object for guid and focus it
		Ogre::String text = sender->getOnlyText();

		text.erase(std::remove(text.begin(), text.end(), '#'), text.end());
		NOWA::GameObjectPtr gameObjectPtr = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(Ogre::StringConverter::parseUnsignedLong(text));
		if (nullptr == gameObjectPtr)
		{
			NOWA::JointCompPtr jointCompPtr = NOWA::makeStrongPtr(NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getJointComponent(Ogre::StringConverter::parseUnsignedLong(text)));
			if (nullptr != jointCompPtr)
			{
				gameObjectPtr = jointCompPtr->getOwner();
			}
		}
		if (nullptr != gameObjectPtr)
		{
			this->editorManager->focusCameraGameObject(gameObjectPtr.get());
			this->editorManager->getSelectionManager()->clearSelection();
			this->editorManager->getSelectionManager()->select(gameObjectPtr->getId());
		}
	}
	else if (sender == this->constraintAxisEdit)
	{
		Ogre::Vector3 constraintAxis = Ogre::StringConverter::parseVector3(sender->getOnlyText());
		this->editorManager->setConstraintAxis(constraintAxis);
	}
}

void DesignState::buttonHit(MyGUI::Widget* sender)
{
	this->selectModeCheck->setStateSelected(false);
	this->placeModeCheck->setStateSelected(false);
	this->translateModeCheck->setStateSelected(false);
	this->pickModeCheck->setStateSelected(false);
	this->scaleModeCheck->setStateSelected(false);
	this->rotate1ModeCheck->setStateSelected(false);
	this->rotate2ModeCheck->setStateSelected(false);
	this->terrainModifyModeCheck->setStateSelected(false);
	this->terrainSmoothModeCheck->setStateSelected(false);
	this->terrainPaintModeCheck->setStateSelected(false);

	if (this->playButton == sender)
	{
		this->simulate(false == this->playButton->getStateSelected(), true);
	}

	if (this->gridButton == sender)
	{
		this->editorManager->setViewportGridEnabled(!this->editorManager->getViewportGridEnabled());
	}

	if (this->focusButton == sender)
	{
		if (this->editorManager->getSelectionManager()->getSelectedGameObjects().size() > 0)
		{
			// Focus the first selected object no matter how many objects are selected
			this->editorManager->focusCameraGameObject(this->editorManager->getSelectionManager()->getSelectedGameObjects().begin()->second.gameObject);
		}
	}

	if (this->selectModeCheck == sender)
	{
		this->selectModeCheck->setStateSelected(true);
		this->editorManager->setManipulationMode(NOWA::EditorManager::EDITOR_SELECT_MODE);
	}
	else if (this->placeModeCheck == sender)
	{
		this->placeModeCheck->setStateSelected(true);
		this->editorManager->setManipulationMode(NOWA::EditorManager::EDITOR_PLACE_MODE);
		this->placeModePopupMenu->showMenu();
		for (size_t i = 0; i < this->placeModePopupMenu->getChildCount(); i++)
		{
			MyGUI::MenuItem* placeModeItem = this->placeModePopupMenu->getChildAt(i)->castType<MyGUI::MenuItem>();
			if (nullptr != placeModeItem)
			{
				placeModeItem->showItemChild();
			}
		}
	}
	else if (this->translateModeCheck == sender)
	{
		this->translateModeCheck->setStateSelected(true);
		this->editorManager->setManipulationMode(NOWA::EditorManager::EDITOR_TRANSLATE_MODE);
		this->translateModePopupMenu->showMenu();
		for (size_t i = 0; i < this->translateModePopupMenu->getChildCount(); i++)
		{
			MyGUI::MenuItem* translateModeItem = this->translateModePopupMenu->getChildAt(i)->castType<MyGUI::MenuItem>();
			if (nullptr != translateModeItem)
			{
				translateModeItem->showItemChild();
			}
		}
	}
	else if (this->pickModeCheck == sender)
	{
		// If already in picker mode, to not re-start simulation again, since another undo command is pushed, so when stopped, the first undo is gone (2x undo would be required)
		if (NOWA::EditorManager::EDITOR_PICKER_MODE != this->editorManager->getManipulationMode())
		{
			this->pickModeCheck->setStateSelected(true);
			this->editorManager->setManipulationMode(NOWA::EditorManager::EDITOR_PICKER_MODE);
			this->editorManager->getGizmo()->setEnabled(false);
			this->simulate(false, true);
			this->propertiesPanel->clearProperties();

			this->playButton->setStateSelected(false);
		}
	}
	else if (this->scaleModeCheck == sender)
	{
		this->scaleModeCheck->setStateSelected(true);
		this->editorManager->setManipulationMode(NOWA::EditorManager::EDITOR_SCALE_MODE);
	}
	else if (this->rotate1ModeCheck == sender)
	{
		this->rotate1ModeCheck->setStateSelected(true);
		this->editorManager->setManipulationMode(NOWA::EditorManager::EDITOR_ROTATE_MODE1);
	}
	else if (this->rotate2ModeCheck == sender)
	{
		this->rotate2ModeCheck->setStateSelected(true);
		this->editorManager->setManipulationMode(NOWA::EditorManager::EDITOR_ROTATE_MODE2);
	}
	else if (this->terrainModifyModeCheck == sender)
	{
		this->terrainSmoothModeCheck->setStateSelected(false);
		this->terrainModifyModeCheck->setStateSelected(true);
		this->terrainPaintModeCheck->setStateSelected(false);
		this->editorManager->setManipulationMode(NOWA::EditorManager::EDITOR_TERRAIN_MODIFY_MODE);
	}
	else if (this->terrainSmoothModeCheck == sender)
	{
		this->terrainSmoothModeCheck->setStateSelected(true);
		this->terrainModifyModeCheck->setStateSelected(false);
		this->terrainPaintModeCheck->setStateSelected(false);
		this->editorManager->setManipulationMode(NOWA::EditorManager::EDITOR_TERRAIN_SMOOTH_MODE);
	}
	else if (this->terrainPaintModeCheck == sender)
	{
		this->terrainSmoothModeCheck->setStateSelected(false);
		this->terrainModifyModeCheck->setStateSelected(false);
		this->terrainPaintModeCheck->setStateSelected(true);
		this->editorManager->setManipulationMode(NOWA::EditorManager::EDITOR_TERRAIN_PAINT_MODE);
	}
	else if (this->wakeButton == sender)
	{
		this->wakeSleepGameObjects(false, false);
	}
	else if (this->sleepButton == sender)
	{
		this->wakeSleepGameObjects(false, true);
	}
	else if (this->undoButton == sender)
	{
		this->editorManager->undo();
		// Show properties
		this->propertiesPanel->showProperties();
		this->resourcesPanel->refresh();
	}
	else if (this->redoButton == sender)
	{
		this->editorManager->redo();
		// Show properties
		this->propertiesPanel->showProperties();
		this->resourcesPanel->refresh();
	}
	else if (this->cameraUndoButton == sender)
	{
		this->editorManager->cameraUndo();
	}
	else if (this->cameraRedoButton == sender)
	{
		this->editorManager->cameraRedo();
	}
	else if (this->selectUndoButton == sender)
	{
		this->editorManager->getSelectionManager()->selectionUndo();
		// Show properties
		this->propertiesPanel->showProperties();
		this->resourcesPanel->refresh();
	}
	else if (this->selectRedoButton == sender)
	{
		this->editorManager->getSelectionManager()->selectionRedo();
		// Show properties
		this->propertiesPanel->showProperties();
		this->resourcesPanel->refresh();
	}
	else if (this->removeButton == sender)
	{
		this->removeGameObjects();
	}
	else if (this->copyButton == sender)
	{
		this->cloneGameObjects();
	}
	else if (this->cameraResetButton == sender)
	{
		if (!GetAsyncKeyState(VK_LSHIFT))
		{
			this->camera->setPosition(0.0f, 1.0f, 0.0f);
		}
		this->camera->setOrientation(Ogre::Quaternion::IDENTITY);
		this->cameraMoveSpeed = 10.0f;
		auto cameraBehavior = NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCameraBehavior();
		if (nullptr != cameraBehavior)
		{
			cameraBehavior->reset();
			cameraBehavior->setMoveSpeed(this->cameraMoveSpeed);
		}
	}
	
	// Check if some place mode has been pressed
	for (size_t i = 0; i < this->placeModePopupMenu->getChildCount(); i++)
	{
		MyGUI::MenuItem* placeModeItem = this->placeModePopupMenu->getChildAt(i)->castType<MyGUI::MenuItem>();
		placeModeItem->setStateSelected(false);
		placeModeItem->setStateCheck(false);
		if (placeModeItem == sender)
		{
			this->editorManager->setManipulationMode(NOWA::EditorManager::EDITOR_PLACE_MODE);
			size_t index = placeModeItem->getItemIndex();
			if (0 == index)
			{
				this->editorManager->setPlaceMode(NOWA::EditorManager::EDITOR_PLACE_MODE_NORMAL);
				placeModeItem->setStateSelected(true);
				placeModeItem->setStateCheck(true);
			}
			else if (1 == index)
			{
				this->editorManager->setPlaceMode(NOWA::EditorManager::EDITOR_PLACE_MODE_STACK);
				placeModeItem->setStateSelected(true);
				placeModeItem->setStateCheck(true);
			}
			else if (2 == index)
			{
				this->editorManager->setPlaceMode(NOWA::EditorManager::EDITOR_PLACE_MODE_STACK_ORIENTATED);
				placeModeItem->setStateSelected(true);
				placeModeItem->setStateCheck(true);
			}
			placeModeItem->hideItemChild();
			break;
		}
	}

	// Check if some translate mode has been pressed
	for (size_t i = 0; i < this->translateModePopupMenu->getChildCount(); i++)
	{
		MyGUI::MenuItem* translateModeItem = this->translateModePopupMenu->getChildAt(i)->castType<MyGUI::MenuItem>();
		translateModeItem->setStateSelected(false);
		translateModeItem->setStateCheck(false);
		if (translateModeItem == sender)
		{
			this->editorManager->setManipulationMode(NOWA::EditorManager::EDITOR_TRANSLATE_MODE);
			size_t index = translateModeItem->getItemIndex();
			if (0 == index)
			{
				this->editorManager->setTranslateMode(NOWA::EditorManager::EDITOR_TRANSLATE_MODE_NORMAL);
				translateModeItem->setStateSelected(true);
				translateModeItem->setStateCheck(true);
			}
			else if (1 == index)
			{
				this->editorManager->setTranslateMode(NOWA::EditorManager::EDITOR_TRANSLATE_MODE_STACK);
				translateModeItem->setStateSelected(true);
				translateModeItem->setStateCheck(true);
			}
			else if (2 == index)
			{
				this->editorManager->setTranslateMode(NOWA::EditorManager::EDITOR_TRANSLATE_MODE_STACK_ORIENTATED);
				translateModeItem->setStateSelected(true);
				translateModeItem->setStateCheck(true);
			}
			translateModeItem->hideItemChild();
			break;
		}
	}
}

void DesignState::mouseClicked(MyGUI::Widget* sender)
{
	if (this->simulationWindow->getCaptionWidget() == sender)
	{
		if (true == this->mainMenuBar->hasLuaErrors())
		{
			this->mainMenuBar->showLuaAnalysisWindow();
		}
	}
}

void DesignState::notifyToolTip(MyGUI::Widget* sender, const MyGUI::ToolTipInfo& info)
{

}

void DesignState::notifyMessageBoxEnd(MyGUI::Message* _sender, MyGUI::MessageBoxStyle _result)
{
	if (_result == MyGUI::MessageBoxStyle::Yes)
	{
		this->bQuit = true;
	}
}

void DesignState::setFocus(MyGUI::Widget* sender, MyGUI::Widget* oldWidget)
{
	this->description.clear();
	// Send the text box change to the game object and internally actualize the data
	Ogre::String description = sender->getUserString("Description");
	this->description = description;
}

void DesignState::updateInfo(Ogre::Real dt)
{
	if (this->nextInfoUpdate <= 0.0f)
	{
		Ogre::String info = "Selected : " + Ogre::StringConverter::toString(this->editorManager->getSelectionManager()->getSelectedGameObjects().size());

		if (false == this->selectedMovableObjectInfo.empty())
		{
			info += " " + this->selectedMovableObjectInfo;
		}

		if (1 == this->editorManager->getSelectionManager()->getSelectedGameObjects().size())
		{
			NOWA::GameObject* gameObject = this->editorManager->getSelectionManager()->getSelectedGameObjects().cbegin()->second.gameObject;
			if (nullptr != gameObject)
				info += " Object Pos: " + Ogre::StringConverter::toString(gameObject->getPosition());
		}

		info += " Camera pos: " + Ogre::StringConverter::toString(NOWA::MathHelper::getInstance()->round(NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getPosition(), 2));
		info += " orient: " + Ogre::StringConverter::toString(NOWA::MathHelper::getInstance()->round(
			NOWA::MathHelper::getInstance()->quatToDegrees(NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getOrientation()), 2));
		info += " speed: " + Ogre::StringConverter::toString(this->cameraMoveSpeed);

		if (NOWA::EditorManager::EDITOR_PICKER_MODE == this->editorManager->getManipulationMode())
		{
			info += " Pick force: " + Ogre::StringConverter::toString(this->editorManager->getPickForce());
		}

		// Do it with tooltips etc.
		/*if (false == this->description.empty())
		{
			info += " | " + this->description;
		}*/

		const Ogre::FrameStats* frameStats = NOWA::Core::getSingletonPtr()->getOgreRoot()->getFrameStats();
		// Does not work in 2.1
		// const Ogre::RenderTarget::FrameStats& stats = NOWA::Core::getSingletonPtr()->getOgreRenderWindow()->getStatistics();

		const RenderingMetrics& metrics = Ogre::Root::getSingletonPtr()->getRenderSystem()->getMetrics();

		float avgTime = frameStats->getAvgTime();
		char m[32];
		sprintf(m, "%.2fms - %.2ffps", avgTime, 1000.0f / avgTime);
		info += " FPS: " + Ogre::String(m);
		// Is always 0??
		/*info += " Faces: " + Ogre::StringConverter::toString(metrics.mFaceCount);
		info += " Batches: " + Ogre::StringConverter::toString(metrics.mBatchCount);
		info += " Vertices: " + Ogre::StringConverter::toString(metrics.mVertexCount);
		info += " Drawings: " + Ogre::StringConverter::toString(metrics.mDrawCount);
		info += " Instances: " + Ogre::StringConverter::toString(metrics.mInstanceCount);*/
		// info += " Threadcount: " + Ogre::StringConverter::toString(NOWA::Core::getSingletonPtr()->getCurrentThreadCount());

		this->manipulationWindow->setCaption(info);

		if (false == this->simulating)
		{
			this->undoButton->setEnabled(this->editorManager->canUndo());
			this->redoButton->setEnabled(this->editorManager->canRedo());
			this->cameraUndoButton->setEnabled(this->editorManager->canCameraUndo());
			this->cameraRedoButton->setEnabled(this->editorManager->canCameraRedo());
			this->selectUndoButton->setEnabled(this->editorManager->getSelectionManager()->canSelectionUndo());
			this->selectRedoButton->setEnabled(this->editorManager->getSelectionManager()->canSelectionRedo());
		}
		this->nextInfoUpdate = 1.0f;
	}
	else
	{
		this->nextInfoUpdate -= dt;
	}
}

void DesignState::removeGameObjects(void)
{
	if (true == this->simulating)
	{
		return;
	}
	std::vector<unsigned long> gameObjectIds;
	// Do not delete directly via selection manager, because when internally deleted, an event is sent out to selection manager to remove from map
	for (auto& it = this->editorManager->getSelectionManager()->getSelectedGameObjects().begin(); it != this->editorManager->getSelectionManager()->getSelectedGameObjects().end(); ++it)
	{
		// SunLight must not be deleted by the user!
		if (it->second.gameObject->getId() != NOWA::GameObjectController::MAIN_LIGHT_ID && it->second.gameObject->getId() != NOWA::GameObjectController::MAIN_CAMERA_ID 
				&& it->second.gameObject->getId() != NOWA::GameObjectController::MAIN_GAMEOBJECT_ID)
		{
			gameObjectIds.emplace_back(it->second.gameObject->getId());
		}
	}
	if (gameObjectIds.size() > 0)
	{
		this->editorManager->deleteGameObjects(gameObjectIds);
		this->editorManager->getSelectionManager()->clearSelection();
		boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshPropertiesPanel);

		boost::shared_ptr<EventDataRefreshResourcesPanel> eventDataRefreshResourcesPanel(new EventDataRefreshResourcesPanel());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshResourcesPanel);

		this->generateCategories();
	}
}

void DesignState::cloneGameObjects(void)
{
	if (true == this->simulating)
	{
		return;
	}
	std::vector<unsigned long> gameObjectIds(this->editorManager->getSelectionManager()->getSelectedGameObjects().size());
	size_t i = 0;
	// Do not delete directly via selection manager, because when internally deleted, an event is sent out to selection manager to remove from map
	for (auto& it = this->editorManager->getSelectionManager()->getSelectedGameObjects().begin(); it != this->editorManager->getSelectionManager()->getSelectedGameObjects().end(); ++it)
	{
		// Prohibit cloning of mandatory game objects
		if (it->second.gameObject->getId() != NOWA::GameObjectController::MAIN_GAMEOBJECT_ID 
			&& it->second.gameObject->getId() != NOWA::GameObjectController::MAIN_CAMERA_ID  
			&& it->second.gameObject->getId() != NOWA::GameObjectController::MAIN_LIGHT_ID)
			gameObjectIds[i++] = it->second.gameObject->getId();
	}
	if (0 == gameObjectIds.size())
	{
		return;
	}

	this->editorManager->cloneGameObjects(gameObjectIds);
	boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshPropertiesPanel);
}

void DesignState::update(Ogre::Real dt)
{
	this->processUnbufferedKeyInput(dt);
	this->processUnbufferedMouseInput(dt);

	NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->update(dt);
	// NOWA::LuaScriptApi::getInstance()->update(dt);

	if (true == this->validScene && false == NOWA::AppStateManager::getSingletonPtr()->getGameProgressModule()->isWorldLoading())
	{
		if (true == this->simulating)
		{
			this->ogreNewt->update(dt);
			NOWA::AppStateManager::getSingletonPtr()->getOgreRecastModule()->update(dt);
			NOWA::AppStateManager::getSingletonPtr()->getParticleUniverseModule()->update(dt);
		}

		// Update the GameObjects
		NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->update(dt, false == this->simulating);

		// this->dotSceneImportModule->update(dt);
		
		if (nullptr != this->editorManager)
			this->editorManager->update(dt);

		this->updateInfo(dt);
	}

	if (true == this->bQuit)
	{
		this->shutdown();
	}
}

void DesignState::lateUpdate(Ogre::Real dt)
{
	const OIS::MouseState& ms = NOWA::InputDeviceCore::getSingletonPtr()->getMouse()->getMouseState();

	if (false == this->simulating && nullptr != MyGUI::InputManager::getInstance().getMouseFocusWidget() && false == ms.buttonDown(OIS::MB_Right) && nullptr == NOWA::InputDeviceCore::getSingletonPtr()->getJoystick())
	{
		return;
	}

	NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->lateUpdate(dt, false == this->simulating);

	// Prevent rotation, when user does something in GUI, or control is pressed
	if (true == validScene && 0 == GetAsyncKeyState(VK_LCONTROL)/* && false == this->playerInControl*/)
	{
		if (ms.buttonDown(OIS::MB_Right) && GetAsyncKeyState(VK_LSHIFT) && this->editorManager->getSelectionManager()->getSelectedGameObjects().size() > 0)
		{
			this->orbitCamera(dt);
		}
		else if (ms.buttonDown(OIS::MB_Right))
		{
			this->firstTimeValueSet = true;
			NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->rotateCamera(dt, false);
		}
		else if (nullptr != NOWA::InputDeviceCore::getSingletonPtr()->getJoystick(0))
		{
			this->firstTimeValueSet = true;
			NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->rotateCamera(dt, true);
		}

		NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->moveCamera(dt);

		if (GetAsyncKeyState(VK_LSHIFT))
		{
			this->cameraMoveSpeed += static_cast<Ogre::Real>(ms.Z.rel) / 1000.0f;
			if (this->cameraMoveSpeed < 2.0f)
				this->cameraMoveSpeed = 2.0f;
			if (this->cameraMoveSpeed > 50.0f)
				this->cameraMoveSpeed = 50.0f;

			auto cameraBehavior = NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCameraBehavior();
			if (nullptr != cameraBehavior)
			{
				cameraBehavior->setMoveSpeed(this->cameraMoveSpeed);
			}
		}
	}
}

void DesignState::orbitCamera(Ogre::Real dt)
{
	const OIS::MouseState& ms = NOWA::InputDeviceCore::getSingletonPtr()->getMouse()->getMouseState();

	Ogre::Vector2 rotationValue;
	rotationValue.x = -ms.X.rel * 0.25f;
	rotationValue.y = ms.Y.rel * 0.25f;

	if (this->firstTimeValueSet)
	{
		this->lastOrbitValue = rotationValue;
		this->firstTimeValueSet = false;
	}

	rotationValue.x = NOWA::MathHelper::getInstance()->lowPassFilter(rotationValue.x, this->lastOrbitValue.x, 0.01f);
	rotationValue.y = NOWA::MathHelper::getInstance()->lowPassFilter(rotationValue.y, this->lastOrbitValue.y, 0.01f);

	// Start orbit mode
	// Same as camera->moveRelative
	Ogre::Vector3 trans = this->camera->getOrientation() * Ogre::Vector3(rotationValue.x, rotationValue.y, 0.0f);
	this->camera->move(trans);

	//this->pCamera->moveRelative(this->pSelectNode->getPosition() + (this->pCamera->getOrientation() * offset));
	//this->pCamera->moveRelative(this->pSelectNode->getPosition());
	// Same as: camera->lookAt
	this->camera->setDirection(this->editorManager->getGizmo()->getSelectedNode()->getPosition() - this->camera->getPosition());

	this->lastOrbitValue = rotationValue;
}

void DesignState::showDebugCollisionLines(bool show)
{
	const auto& gameObjects = this->editorManager->getSelectionManager()->getSelectedGameObjects();
	for (auto& it = gameObjects.begin(); it != gameObjects.end(); it++)
	{
		auto& gameObject = it->second.gameObject;
		// Issue here: PhysicsRagDollComponent is derived from PhysicsActiveComponent, and PhysicsActiveComponent is derived from GameObjectComponent, but the link from
		// PhysicsRagDollComponent to GameObjectComponent is somehow missing ?!?
		auto& gameObjectComponent = NOWA::makeStrongPtr(gameObject->getComponent<NOWA::GameObjectComponent>());
		if (nullptr != gameObjectComponent)
		{
			gameObjectComponent->showDebugData();
		}
		else
		{
			auto& physicsActiveComponent = NOWA::makeStrongPtr(gameObject->getComponent<NOWA::PhysicsActiveComponent>());
			if (nullptr != physicsActiveComponent)
			{
				physicsActiveComponent->showDebugData();
			}
		}
	}
}

bool DesignState::keyPressed(const OIS::KeyEvent &keyEventRef)
{
	// Prevent scene manipulation, when user does something in GUI
	/*if (nullptr != MyGUI::InputManager::getInstance().getMouseFocusWidget())
	{
		return true;
	}*/

	if (false == this->simulating)
	{
		if (GetAsyncKeyState(VK_LCONTROL) && keyEventRef.key == MyGUI::KeyCode::C)
		{
			if (false == this->selectedMovableObjectInfo.empty())
			{
				Ogre::String toFind = "local pos: ";
				size_t foundStart = this->selectedMovableObjectInfo.find(toFind);
				if (foundStart != Ogre::String::npos)
				{
					size_t foundEnd = this->selectedMovableObjectInfo.find(" normal: ", foundStart + toFind.length());
					if (foundEnd != Ogre::String::npos)
					{
						Ogre::String strPosition = this->selectedMovableObjectInfo.substr(foundStart + toFind.length(), foundEnd - (foundStart + toFind.length()));
						NOWA::Core::getSingletonPtr()->copyTextToClipboard(strPosition);
					}
				}

				size_t foundPos = this->selectedMovableObjectInfo.find("pos: ");
				if (foundPos != Ogre::String::npos)
				{
					
				}
			}
		}

		if (keyEventRef.key == MyGUI::KeyCode::Add)
		{
			this->cameraMoveSpeed += 2.0f;
			if (this->cameraMoveSpeed > 50.0f)
				this->cameraMoveSpeed = 50.0f;
			auto cameraBehavior = NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCameraBehavior();
			if (nullptr != cameraBehavior)
			{
				cameraBehavior->setMoveSpeed(this->cameraMoveSpeed);
			}
		}
		else if (keyEventRef.key == MyGUI::KeyCode::Subtract)
		{
			this->cameraMoveSpeed -= 2.0f;
			if (this->cameraMoveSpeed < 2.0f)
				this->cameraMoveSpeed = 2.0f;
			auto cameraBehavior = NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCameraBehavior();
			if (nullptr != cameraBehavior)
			{
				cameraBehavior->setMoveSpeed(this->cameraMoveSpeed);
			}
		}
	}

	if (NOWA::LuaConsole::getSingletonPtr() && NOWA::LuaConsole::getSingletonPtr()->isVisible())
	{
		return true;
	}

	bool handled = false;

	if (true == validScene)
	{
		handled = this->editorManager->handleKeyPress(keyEventRef);
	}

	if (false == handled)
	{
		switch (keyEventRef.key)
		{
			case OIS::KC_ESCAPE:
			{
				// Stop simulation if simulating
				if (true == this->simulating)
				{
					if (GetAsyncKeyState(VK_LCONTROL))
						this->simulate(true, false); // Stop without undo
					else
						this->simulate(true, true);
				}
				else
				{
					// Ask user whether he really wants to quit the application
					MyGUI::Message* messageBox = MyGUI::Message::createMessageBox("Menue", MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{Quit_Application}"),
						MyGUI::MessageBoxStyle::IconWarning | MyGUI::MessageBoxStyle::Yes | MyGUI::MessageBoxStyle::No, "Popup", true);

					messageBox->eventMessageBoxResult += MyGUI::newDelegate(this, &DesignState::notifyMessageBoxEnd);
				}
				return true;
			}
			/*case OIS::KC_N:
			{
				if (GetAsyncKeyState(VK_LCONTROL) && nullptr != this->projectManager)
				{
					this->mainMenuBar->callNewProject();
				}
				return true;
			}*/
			case OIS::KC_O:
			{
				if (nullptr == MyGUI::InputManager::getInstance().getMouseFocusWidget())
				{
					if (GetAsyncKeyState(VK_LCONTROL) && nullptr != this->projectManager)
					{
						this->projectManager->showFileOpenDialog("LoadProject", "*.scene");
					}
				}
				return true;
			}
			case OIS::KC_F5:
			{
				if (GetAsyncKeyState(VK_LCONTROL))
				{
					NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->createAllGameObjectsForShaderCacheGeneration(this->sceneManager);
				}
				NOWA::Core::getSingletonPtr()->getOgreRoot()->getRenderSystem()->validateDevice(true);

				return true;
			}
			case OIS::KC_F6:
			{
				NOWA::Core::getSingletonPtr()->dumpNodes(this->sceneManager->getRootSceneNode());
				return true;
			}
			case OIS::KC_F7:
			{
				NOWA::DeployResourceModule::getInstance()->createAndStartExecutable(NOWA::Core::getSingletonPtr()->getProjectName(), NOWA::Core::getSingletonPtr()->getWorldName());
				// NOWA::AppStateManager::getSingletonPtr()->pause();
				NOWA::Core::getSingletonPtr()->moveWindowToTaskbar();
				return true;
			}
		}
	}

	if (false == handled && nullptr == MyGUI::InputManager::getInstance().getMouseFocusWidget() && true == validScene)
	{
		switch (keyEventRef.key)
		{
			case OIS::KC_A: // Select all that matching the filter
			{
				
				if (GetAsyncKeyState(VK_LCONTROL))
				{
					auto& affectedGameObjects = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromCategory(this->activeCategory);
					for (size_t i = 0; i < affectedGameObjects.size(); i++)
					{
						this->editorManager->getSelectionManager()->select(affectedGameObjects[i]->getId());
					}
					this->propertiesPanel->showProperties();
				}
				return true;
			}
			case OIS::KC_C:
			{
				if (GetAsyncKeyState(VK_LCONTROL) && GetAsyncKeyState(VK_LSHIFT))
				{
					if (nullptr != this->componentsPanel)
					{
						this->componentsPanel->showComponents(-1);
					}
				}
				else if (GetAsyncKeyState(VK_LCONTROL))
				{
					// Only clone if the mouse is in the scene and not in a property, that may be also copied
					this->cloneGameObjects();
				}
				return true;
			}
			case OIS::KC_T:
			{
				if (GetAsyncKeyState(VK_LCONTROL))
				{
					this->buttonHit(this->translateModeCheck);
				}
				return true;
			}
			case OIS::KC_S:
			{
				if (GetAsyncKeyState(VK_LCONTROL))
				{
					if (false == GetAsyncKeyState(VK_LSHIFT))
					{
						this->buttonHit(this->scaleModeCheck);
					}
					else
					{
						if (nullptr != this->projectManager)
						{
							this->projectManager->saveProject();
						}
					}
				}
				return true;
			}
			case OIS::KC_R:
			{
				if (GetAsyncKeyState(VK_LCONTROL))
				{
					if (GetAsyncKeyState(VK_LSHIFT))
					{
						this->buttonHit(this->rotate2ModeCheck);
					}
					else
					{
						this->buttonHit(this->rotate1ModeCheck);
					}
				}
				return true;
			}
			case OIS::KC_Y:
			{
				if (GetAsyncKeyState(VK_LCONTROL))
				{
					this->buttonHit(this->undoButton);
				}
				return true;
			}
			case OIS::KC_Z:
			{
				if (GetAsyncKeyState(VK_LCONTROL))
				{
					this->buttonHit(this->redoButton);
				}
				return true;
			}
			case OIS::KC_SPACE:
			{
				if (GetAsyncKeyState(VK_LCONTROL))
				{
					this->simulating = !this->simulating;
					this->buttonHit(this->playButton);
				}
				return true;
			}
			case OIS::KC_DELETE:
			{
				if (true == validScene)
				{
					this->removeGameObjects();
				}
				return true;
			}
			case OIS::KC_TAB:
			{
				if (GetAsyncKeyState(KF_ALTDOWN))
				{
					NOWA::Core::getSingletonPtr()->moveWindowToTaskbar();
				}
				return true;
			}
			case OIS::KC_7:
			{
				// NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->showOgreNewtCollisionLines(true);
				this->showDebugCollisionLines(true);
				return true;
			}
			case OIS::KC_8:
			{
				// NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->showOgreNewtCollisionLines(false);
				this->showDebugCollisionLines(false);
				return true;
			}
			case OIS::KC_F9:
			{
				NOWA::AppStateManager::getSingletonPtr()->getOgreRecastModule()->debugDrawNavMesh(true);
				return true;
			}
			case OIS::KC_F10:
			{
				NOWA::AppStateManager::getSingletonPtr()->getOgreRecastModule()->debugDrawNavMesh(false);
				return true;
			}
			case OIS::KC_F11:
			{
				// NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->showOgreNewtCollisionLines(true);
				NOWA::Core::getSingletonPtr()->getOgreRenderWindow()->requestFullscreenSwitch(true, false, 0, 1920, 1080, 0, 0);
				return true;
			}
		}

		// Snapshot camera position when used pressed a camera key
		if (NOWA_K_CAMERA_LEFT == keyEventRef.key || NOWA_K_CAMERA_RIGHT == keyEventRef.key
			|| NOWA_K_CAMERA_BACKWARD == keyEventRef.key || NOWA_K_CAMERA_FORWARD == keyEventRef.key
			|| NOWA_K_CAMERA_UP == keyEventRef.key || NOWA_K_CAMERA_DOWN == keyEventRef.key && (false == this->playerInControl))
		{
			this->editorManager->snapshotCameraPosition();
		}
	}

	return true;
}

bool DesignState::keyReleased(const OIS::KeyEvent &keyEventRef)
{
	// Prevent scene manipulation, when user does something in GUI
	/*if (nullptr != MyGUI::InputManager::getInstance().getMouseFocusWidget())
	{
		return true;
	}*/

	if (NOWA::LuaConsole::getSingletonPtr() && NOWA::LuaConsole::getSingletonPtr()->isVisible())
	{
		return true;
	}
	switch (keyEventRef.key)
	{
		case OIS::KC_F4:
		{
			NOWA::Core::getSingletonPtr()->setPolygonMode(3);
			// this->camera->setPolygonMode(Ogre::PM_SOLID);
			break;
		}
	}

	if (true == validScene)
	{
		this->editorManager->handleKeyRelease(keyEventRef);
	}
	return true;
}

void DesignState::processUnbufferedKeyInput(Ogre::Real dt)
{
	if (NOWA::LuaConsole::getSingletonPtr() && NOWA::LuaConsole::getSingletonPtr()->isVisible())
	{
		return;
	}
}

void DesignState::processUnbufferedMouseInput(Ogre::Real dt)
{

}

bool DesignState::mouseMoved(const OIS::MouseEvent& evt)
{
	if (false == this->simulating)
	{
		if (evt.state.buttonDown(OIS::MB_Middle))
		{
			Ogre::MovableObject* movableObject = nullptr;
			Ogre::Item* item = nullptr;
			Ogre::Vector3 result = Ogre::Vector3::ZERO;
			Ogre::Real closestDistance = 0.0f;
			Ogre::Vector3 normal = Ogre::Vector3::ZERO;

			if (NOWA::MathHelper::getInstance()->getRaycastFromPoint(evt.state.X.abs, evt.state.Y.abs, this->camera, NOWA::Core::getSingletonPtr()->getOgreRenderWindow(),
				this->selectQuery, result, (size_t&)movableObject, closestDistance, normal, nullptr, false))
			{
				this->selectedMovableObjectInfo = "GameObject: " + movableObject->getName() + " global pos: " + Ogre::StringConverter::toString(result) + " local pos: " + Ogre::StringConverter::toString(result - movableObject->getParentNode()->_getDerivedPositionUpdated()) + " normal: " + Ogre::StringConverter::toString(normal);
			}
		}
	}

	// Prevent scene manipulation, when user does something in GUI
	MyGUI::Widget* widget = MyGUI::InputManager::getInstance().getMouseFocusWidget();
	if (nullptr != widget && false == this->simulating)
	{
		// If mouse wheel has been pressed, search for the scroll view in panel view parenting and simulate scrolling manually, because it does not work in MyGUI :(
		if (evt.state.Z.rel != 0)
		{
			// If the user was inside an editbox and scroll away, erase possible text selection
			/*MyGUI::TextBox* textBox = widget->castType<MyGUI::TextBox>(false);
			if (nullptr != textBox)
			{
				textBox->getSubWidgetText()->setTextSelection(0, 0);
			}*/

			MyGUI::ComboBox* comboBox = MyGUIHelper::getInstance()->findParentWidgetByType<MyGUI::ComboBox>(widget);
			if (nullptr != comboBox)
			{
				// If its a combo box, scrolling here would effect, that listbox list is scrolled and the panel
				// Note: widget->parent->parent->listbox->combobox
				return true;
			}
			MyGUI::TreeControl* treeControl = MyGUIHelper::getInstance()->findParentWidgetByType<MyGUI::TreeControl>(widget);
			if (nullptr != treeControl)
			{
				// If its a tree, scrolling here would effect, that tree list is scrolled and the panel
				return true;
			}
			MyGUI::ItemBox* itemBox = MyGUIHelper::getInstance()->findParentWidgetByType<MyGUI::ItemBox>(widget);
			if (nullptr != itemBox)
			{
				// If its an item, scrolling here would effect, that item list is scrolled and the panel
				return true;
			}
			
			MyGUI::ScrollView* scrollView = MyGUIHelper::getInstance()->findParentWidgetByType<MyGUI::ScrollView>(widget);

			if (nullptr != scrollView && nullptr != this->editorManager)
			{
				int scrollAmount = scrollView->getViewOffset().top + evt.state.Z.rel;
				scrollView->setViewOffset(MyGUI::IntPoint(scrollView->getViewOffset().left, scrollAmount));

				unsigned long id = -1;
				
				if (this->editorManager->getSelectionManager()->getSelectedGameObjects().size() > 0)
				{
					id = this->editorManager->getSelectionManager()->getSelectedGameObjects().cbegin()->second.gameObject->getId();
				}

				MyGUIHelper::getInstance()->setScrollPosition(id, scrollAmount);
			}
		}
		return true;
	}

	if (true == validScene)
	{
		if (nullptr != this->editorManager)
		{
			this->editorManager->handleMouseMove(evt);
		}
	}
	return true;
}

bool DesignState::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
{
	// Prevent scene manipulation, when user does something in GUI
	if (nullptr != MyGUI::InputManager::getInstance().getMouseFocusWidget()/* && false == this->simulating*/)
	{
		NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setMoveCameraWeight(0.0f);
		NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setRotateCameraWeight(0.0f);
		return true;
	}

	NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setMoveCameraWeight(1.0f);
	NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setRotateCameraWeight(1.0f);

	if (true == validScene)
	{
		// Snapshot camera orientation when used pressed the right mouse key
		if (evt.state.buttonDown(OIS::MB_Right))
		{
			this->editorManager->snapshotCameraOrientation();
		}

		this->editorManager->handleMousePress(evt, id);

		//if (false == this->simulating && false == this->bQuit && true == validScene && this->editorManager->getSelectionManager()->getSelectedGameObjects().size() > 0)
		//{
		//	// Show properties
		//	this->propertiesPanel->showProperties();
		//}
	}
	return true;
}

bool DesignState::mouseReleased(const OIS::MouseEvent &evt, OIS::MouseButtonID id)
{
	this->selectedMovableObjectInfo.clear();

	// Prevent scene manipulation, when user does something in GUI
	MyGUI::Widget* widget = MyGUI::InputManager::getInstance().getMouseFocusWidget();
	if (nullptr != widget/* && true == this->simulating*/) // causes ugly gui behavior
	{
		return true;
	}

	NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setMoveCameraWeight(1.0f);
	NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setRotateCameraWeight(1.0f);

	if (nullptr != this->editorManager)
	{
		this->editorManager->handleMouseRelease(evt, id);
	}
	if (false == this->simulating && false == this->bQuit && true == validScene)
	{
		if (id == OIS::MB_Left && nullptr != editorManager 
			&& this->editorManager->getManipulationMode() != NOWA::EditorManager::EDITOR_TERRAIN_MODIFY_MODE 
			&& this->editorManager->getManipulationMode() != NOWA::EditorManager::EDITOR_TERRAIN_SMOOTH_MODE 
			&& this->editorManager->getManipulationMode() != NOWA::EditorManager::EDITOR_TERRAIN_PAINT_MODE)
		{
			if (false == MyGUIHelper::getInstance()->isIdPairingActive())
			{
				if (this->editorManager->getSelectionManager()->getSelectedGameObjects().size() > 0)
				{
					/*bool selectedGameObjectsChanged = false;
					for (auto it = this->editorManager->getSelectionManager()->getSelectedGameObjects().cbegin(); it != this->editorManager->getSelectionManager()->getSelectedGameObjects().cend(); ++it)
					{
						auto found = oldSelectedGameObjectIds.find(it->first);
						if (found == oldSelectedGameObjectIds.cend())
						{
							selectedGameObjectsChanged = true;
							break;
						}
					}*/

					// if (true == selectedGameObjectsChanged)
					{
						// Show properties (only when selection changed, because showProperties is an heavy operation!)
						this->propertiesPanel->showProperties();
					}
					// Attention: To early here, better, when everything is loaded
					/*if (-1 != MyGUIHelper::getInstance()->getScrollPosition())
					{
						MyGUI::ScrollView* scrollView = MyGUIHelper::getInstance()->findParentWidgetByType<MyGUI::ScrollView>(widget);
						if (nullptr != scrollView)
						{
							scrollView->setViewOffset(MyGUI::IntPoint(scrollView->getViewOffset().left, MyGUIHelper::getInstance()->getScrollPosition()));
						}
					}*/

					/*oldSelectedGameObjectIds.clear();
					for (auto it = this->editorManager->getSelectionManager()->getSelectedGameObjects().cbegin(); it != this->editorManager->getSelectionManager()->getSelectedGameObjects().cend(); ++it)
					{
						oldSelectedGameObjectIds.emplace(it->first);
					}*/
				}
			}
			else
			{
				// When id to edit box pairing is active, pair the id
				if (1 == this->editorManager->getSelectionManager()->getSelectedGameObjects().size())
				{
					NOWA::GameObject* gameObject = this->editorManager->getSelectionManager()->getSelectedGameObjects().cbegin()->second.gameObject;
					if (nullptr != gameObject)
					{
						MyGUIHelper::getInstance()->pairIdWithEditBox(Ogre::StringConverter::toString(gameObject->getId()));
					}
				}
			}
			MyGUIHelper::getInstance()->clearIdPairing();
		}
	}
	return true;
}

bool DesignState::axisMoved(const OIS::JoyStickEvent& evt, int axis)
{
	//int abs = evt.state.mAxes[axis].abs;
	//int rel = evt.state.mAxes[axis].rel; // is always 0
	//Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: Axis " + Ogre::StringConverter::toString(axis));

	//switch (axis)
	//{
	//case 0: //Forward/Back pitch up + /pitch down -
	//	Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: Pitch Abs " + Ogre::StringConverter::toString(abs));
	//	break;
	//case 1: //Turn right + /left -
	//	Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: Yaw Abs " + Ogre::StringConverter::toString(abs));
	//	break;
	//case 2: //Swivel right + / left -
	//	Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: Swivel Abs " + Ogre::StringConverter::toString(abs));
	//	break;
	//	//case 3: //Left
	//		//joyAxes.swivelL=x;
	//	//break;
	//}

	return true;
}

bool DesignState::povMoved(const OIS::JoyStickEvent& evt, int pov)
{
	// this code goes in the function calls of the joysticks, axisMoved, buttonPressed, buttonReleassed, etc.
	// the variable joyID gets the id number as follows, 0 - for the first input device(joystick one), 1 - second device(joystick two), an so on..
	// const OIS::Object* mJoyObj = evt.device;
	// unsigned int joyID = mJoyObj->getID();
	// Identfy which joystick has been handled

	Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: Vendor: " +  evt.device->vendor() + ". POV" + Ogre::StringConverter::toString(pov));

	if (evt.state.mPOV[pov].direction & OIS::Pov::North) //Going up
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: North");
	else if (evt.state.mPOV[pov].direction & OIS::Pov::South) //Going down
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: South");

	if (evt.state.mPOV[pov].direction & OIS::Pov::East) //Going right
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: East");
	else if (evt.state.mPOV[pov].direction & OIS::Pov::West) //Going left
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: West");

	if (evt.state.mPOV[pov].direction == OIS::Pov::Centered) //stopped/centered out
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: Centered");

	if (evt.state.mPOV[pov].direction & OIS::Pov::NorthEast) //Going right
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: North-East");
	else if (evt.state.mPOV[pov].direction & OIS::Pov::SouthEast) //Going left
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: South-East");

	if (evt.state.mPOV[pov].direction & OIS::Pov::NorthWest) //Going right
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: North-West");
	else if (evt.state.mPOV[pov].direction & OIS::Pov::SouthWest) //Going left
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: South-West");

	return true;
}

bool DesignState::vector3Moved(const OIS::JoyStickEvent & evt, int index)
{
	Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: vector3Moved Vendor: " + evt.device->vendor() + ". Orientation" 
		+ Ogre::StringConverter::toString(Ogre::Vector3(evt.state.mVectors[index].x, evt.state.mVectors[index].y, evt.state.mVectors[index].z)));

	return false;
}

bool DesignState::buttonPressed(const OIS::JoyStickEvent& evt, int button)
{
	Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: Button " + Ogre::StringConverter::toString(button) + " pressed");
	
	switch (button)
	{
	case 0:
		
		break;

	case 2:
		
		break;
	}

	return true;
}

bool DesignState::buttonReleased(const OIS::JoyStickEvent& evt, int button)
{
	return true;
}

bool DesignState::sliderMoved(const OIS::JoyStickEvent & evt, int index)
{
	return true;
}
