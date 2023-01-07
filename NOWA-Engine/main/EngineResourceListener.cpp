#include "NOWAPrecompiled.h"
#include "EngineResourceListener.h"
#include "Core.h"
#include "utilities/MyGUIUtilities.h"

namespace NOWA
{
	EngineResourceRotateListener::EngineResourceRotateListener(Ogre::Window* renderWindow)
		: EngineResourceListener(),
		renderWindow(renderWindow),
		groupInitProportion(0.0f),
		groupLoadProportion(0.0f),
		loadInc(0.0f)
	{
		this->backgroundImage = MyGUI::Gui::getInstancePtr()->createWidget<MyGUI::ImageBox>("RotatingSkin", MyGUI::IntCoord(0, 0, renderWindow->getWidth(), renderWindow->getHeight()), 
			MyGUI::Align::Default, "Overlapped");
		this->backgroundImage->setImageTexture("BackgroundShadeBlue.png");
		this->backgroundImage->setVisible(false);

		this->actionLabel = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::EditBox>("EditBoxEmpty", 0.4f, 0.4f, 0.3f, 0.05f, MyGUI::Align::Default, "Main", "ActionLabel");
		this->actionLabel->setVisible(false);
		// actionLabel->setColour(MyGUI::Colour::White);
		this->actionLabel->setEditStatic(true);
		MyGUIUtilities::getInstance()->setFontSize(this->actionLabel, 14);
		// actionLabel->setTextShadow(true);

		this->progressLabel = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::EditBox>("EditBoxEmpty", 0.05f, 0.95f, 0.3f, 0.05f, MyGUI::Align::Default, "Main", "ProgressLabel");
		this->progressLabel->setVisible(false);
		// this->progressLabel->setColour(MyGUI::Colour::White);
		this->progressLabel->setEditStatic(true);
		MyGUIUtilities::getInstance()->setFontSize(this->progressLabel, 14);

		// actionLabel->setTextShadow(true);

		int logoWidth = static_cast<int>(260.0f * 0.5f);
		int logoHeight = static_cast<int>(160.0f * 0.5);
		this->rotatingImage = MyGUI::Gui::getInstancePtr()->createWidget<MyGUI::ImageBox>("RotatingSkin", 
			MyGUI::IntCoord(renderWindow->getWidth() - logoWidth - 20, renderWindow->getHeight() - logoHeight - 20, logoWidth, logoHeight), MyGUI::Align::Default, "Overlapped");
		this->rotatingImage->setImageTexture("NOWA_Logo3.png");
		this->rotatingImage->setVisible(false);
		// this->rotatingImage->setAlpha(1.0f);

		MyGUI::ISubWidget* main = this->rotatingImage->getSubWidgetMain();
		if (nullptr == main)
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[EngineResourceRotateListener] Error: Could not get MyGUI object. Check the resources.cfg, whether 'FileSystem=../../media/MyGUI_Media' is missing!");
			throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND, "[EngineResourceRotateListener] Error: Could not get MyGUI object. Check the resources.cfg, whether 'FileSystem=../../media/MyGUI_Media' is missing!\n", "NOWA");
		}
		this->rotatingSkin = main->castType<MyGUI::RotatingSkin>();
	}

	EngineResourceRotateListener::~EngineResourceRotateListener()
	{
		MyGUI::Gui::getInstancePtr()->destroyWidget(this->backgroundImage);
		MyGUI::Gui::getInstancePtr()->destroyWidget(this->rotatingImage);
		MyGUI::Gui::getInstancePtr()->destroyWidget(this->actionLabel);
		MyGUI::Gui::getInstancePtr()->destroyWidget(this->progressLabel);
	}

	void EngineResourceRotateListener::showLoadingBar(unsigned int numGroupsInit, unsigned int numGroupsLoad, Ogre::Real initProportion)
	{
		this->backgroundImage->setVisible(true);
		this->rotatingImage->setVisible(true);
		this->actionLabel->setVisible(true);
		this->progressLabel->setVisible(true);

		Ogre::ResourceGroupManager::getSingleton().addResourceGroupListener(this);
		MyGUI::PointerManager::getInstancePtr()->setVisible(false);

		// calculate the proportion of job required to init/load one group
		if (numGroupsInit == 0 && numGroupsLoad != 0)
		{
			this->groupInitProportion = 0;
			this->groupLoadProportion = 1;
		}
		else if (numGroupsLoad == 0 && numGroupsInit != 0)
		{
			this->groupLoadProportion = 0;
			if (numGroupsInit != 0)
			{
				this->groupInitProportion = 1;
			}
		}
		else if (numGroupsInit == 0 && numGroupsLoad == 0)
		{
			this->groupInitProportion = 0;
			this->groupLoadProportion = 0;
		}
		else
		{
			this->groupInitProportion = initProportion / numGroupsInit;
			this->groupLoadProportion = (1 - initProportion) / numGroupsLoad;
		}
	}

	std::pair<Ogre::Real, Ogre::Real> EngineResourceRotateListener::hideLoadingBar(void)
	{
		MyGUI::PointerManager::getInstancePtr()->setVisible(true);
		this->actionLabel->setVisible(false);
		this->progressLabel->setVisible(false);
		this->backgroundImage->setVisible(false);
		this->rotatingImage->setVisible(false);
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("ActionLabel")->setVisible(false);
		Ogre::ResourceGroupManager::getSingleton().removeResourceGroupListener(this);

		return std::make_pair(0.0f, 0.0f);
	}

	void EngineResourceRotateListener::resourceGroupScriptingStarted(const Ogre::String& resourceGroupName, size_t scriptCount)
	{
		this->loadInc = this->groupInitProportion / scriptCount;
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("ActionLabel")->setCaptionWithReplacing("#00ff00#{Finished}!");
		Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
	}

	void EngineResourceRotateListener::scriptParseStarted(const Ogre::String& scriptName, bool& skipThisScript)
	{
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("ActionLabel")->setCaptionWithReplacing("#00ff00#{Parsing}: " + scriptName);
		Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
	}

	void EngineResourceRotateListener::scriptParseEnded(const Ogre::String& scriptName, bool skipped)
	{
		this->rotatingSkin->setAngle(this->rotatingSkin->getAngle() + this->loadInc * Ogre::Math::TWO_PI);

		this->progressLabel->setCaptionWithReplacing("#00ff00" +
			Ogre::StringConverter::toString(static_cast<int>(this->rotatingSkin->getAngle() / Ogre::Math::TWO_PI * 100.0f)) + " %");

		// this->rotatingImage->setAlpha(0.2f);
		this->rotatingSkin->setCenter(MyGUI::IntPoint(this->rotatingSkin->getWidth() / 2, this->rotatingSkin->getHeight() / 2));
		Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
	}

	void EngineResourceRotateListener::resourceGroupScriptingEnded(const Ogre::String& resourceGroupName)
	{

	}

	void EngineResourceRotateListener::resourceGroupLoadStarted(const Ogre::String& resourceGroupName, size_t resourceCount)
	{
		this->loadInc = this->groupLoadProportion / resourceCount;
		this->actionLabel->setCaptionWithReplacing("#00ff00#{Loading}...");
		Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
	}

	void EngineResourceRotateListener::resourceLoadStarted(const Ogre::ResourcePtr& resource)
	{
		this->actionLabel->setCaptionWithReplacing("#00ff00#{Loading}: " + resource->getName());
		Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
	}

	void EngineResourceRotateListener::resourceLoadEnded()
	{
		Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
	}

	/*void EngineResourceRotateListener::worldGeometryStageStarted(const Ogre::String& description)
	{
		this->actionLabel->setCaptionWithReplacing("#00ff00" + description);
		Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
	}

	void EngineResourceRotateListener::worldGeometryStageEnded()
	{
		Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
	}*/

	void EngineResourceRotateListener::resourceGroupLoadEnded(const Ogre::String& resourceGroupName)
	{

	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////

	EngineResourceFadeListener::EngineResourceFadeListener(Ogre::Window* renderWindow)
		: EngineResourceListener(),
		renderWindow(renderWindow),
		groupInitProportion(0.0f),
		groupLoadProportion(0.0f),
		loadInc(0.0f),
		faderProcess(nullptr)
	{
		
	}

	EngineResourceFadeListener::~EngineResourceFadeListener()
	{
		if (nullptr != this->faderProcess)
		{
			delete this->faderProcess;
			this->faderProcess = nullptr;
		}
	}

	void EngineResourceFadeListener::showLoadingBar(unsigned int numGroupsInit, unsigned int numGroupsLoad, Ogre::Real initProportion)
	{
		if (nullptr != this->faderProcess)
		{
			delete this->faderProcess;
			this->faderProcess = nullptr;
		}

		Ogre::ResourceGroupManager::getSingleton().addResourceGroupListener(this);
		MyGUI::PointerManager::getInstancePtr()->setVisible(false);

		// calculate the proportion of job required to init/load one group
		if (numGroupsInit == 0 && numGroupsLoad != 0)
		{
			this->groupInitProportion = 0;
			this->groupLoadProportion = 1;
		}
		else if (numGroupsLoad == 0 && numGroupsInit != 0)
		{
			this->groupLoadProportion = 0;
			if (numGroupsInit != 0)
			{
				this->groupInitProportion = 1;
			}
		}
		else if (numGroupsInit == 0 && numGroupsLoad == 0)
		{
			this->groupInitProportion = 0;
			this->groupLoadProportion = 0;
		}
		else
		{
			this->groupInitProportion = initProportion / numGroupsInit;
			this->groupLoadProportion = (1 - initProportion) / numGroupsLoad;
		}

		this->faderProcess = new FaderProcess(FaderProcess::FadeOperation::FADE_IN, 10.0f);
	}

	std::pair<Ogre::Real, Ogre::Real> EngineResourceFadeListener::hideLoadingBar(void)
	{
		MyGUI::PointerManager::getInstancePtr()->setVisible(true);
		auto continueData = std::make_pair(this->faderProcess->getCurrentAlpha(), this->faderProcess->getCurrentDuration());
		if (nullptr != this->faderProcess)
		{
			this->faderProcess->finished();

			delete this->faderProcess;
			this->faderProcess = nullptr;
		}
		return continueData;
	}

	void EngineResourceFadeListener::resourceGroupScriptingStarted(const Ogre::String& resourceGroupName, size_t scriptCount)
	{
		this->loadInc = this->groupInitProportion / scriptCount;
		this->faderProcess->onUpdate(0.016f);
		// Causes, that world is loaded to long and maybe a crash
		 Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
	}

	void EngineResourceFadeListener::scriptParseStarted(const Ogre::String& scriptName, bool& skipThisScript)
	{
		this->faderProcess->onUpdate(0.016f);
		Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
	}

	void EngineResourceFadeListener::scriptParseEnded(const Ogre::String& scriptName, bool skipped)
	{
		this->faderProcess->onUpdate(0.016f);
		Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
	}

	void EngineResourceFadeListener::resourceGroupScriptingEnded(const Ogre::String& resourceGroupName)
	{
		this->faderProcess->onUpdate(0.016f);
		Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
	}

	void EngineResourceFadeListener::resourceGroupLoadStarted(const Ogre::String& resourceGroupName, size_t resourceCount)
	{
		this->loadInc = this->groupLoadProportion / resourceCount;
		this->faderProcess->onUpdate(0.016f);
		Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
	}

	void EngineResourceFadeListener::resourceLoadStarted(const Ogre::ResourcePtr& resource)
	{
		this->faderProcess->onUpdate(0.016f);
		Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
	}

	void EngineResourceFadeListener::resourceLoadEnded()
	{
		this->faderProcess->onUpdate(0.016f);
		Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
	}

	/*void EngineResourceFadeListener::worldGeometryStageStarted(const Ogre::String& description)
	{
		this->faderProcess->onUpdate(0.016f);
		Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
	}

	void EngineResourceFadeListener::worldGeometryStageEnded()
	{
		this->faderProcess->onUpdate(0.016f);
		Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
	}*/

	void EngineResourceFadeListener::resourceGroupLoadEnded(const Ogre::String& resourceGroupName)
	{
		this->faderProcess->onUpdate(0.016f);
		Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
	}

}; // namespace end
