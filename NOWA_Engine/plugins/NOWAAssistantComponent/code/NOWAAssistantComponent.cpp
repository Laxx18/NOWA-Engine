#include "NOWAPrecompiled.h"
#include "NOWAAssistantComponent.h"

#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"

#include "OgreAbiUtils.h"

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <sstream>

// Module anchor
static void nowaAssistantModuleAnchor()
{
}

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	NOWAAssistantComponent::NOWAAssistantComponent()
		: GameObjectComponent(),
		name("NOWAAssistantComponent"),
		widget(nullptr),
		promptEdit(nullptr),
		outputEdit(nullptr),
		statusLabel(nullptr),
		askButton(nullptr),
		clearButton(nullptr),
		hasParent(false),
		servicesStartedThisSession(false),
		activated(new Variant(NOWAAssistantComponent::AttrActivated(), true, this->attributes)),
		relativePosition(new Variant(NOWAAssistantComponent::AttrRelativePosition(), Ogre::Vector2(0.2f, 0.2f), this->attributes)),
		parentId(new Variant(NOWAAssistantComponent::AttrParentId(), static_cast<unsigned long>(0), this->attributes, true)),
		startServicesOnActivate(new Variant(NOWAAssistantComponent::AttrStartServicesOnActivate(), false, this->attributes)),
		servicesBatPath(new Variant(NOWAAssistantComponent::AttrServicesBatPath(), Ogre::String("Ai\\start_ai_services.bat"), this->attributes)),
		pythonExe(new Variant(NOWAAssistantComponent::AttrPythonExe(), Ogre::String("python"), this->attributes)),
		askScriptPath(new Variant(NOWAAssistantComponent::AttrAskScriptPath(), Ogre::String("Ai\\ask_once.py"), this->attributes)),
		topK(new Variant(NOWAAssistantComponent::AttrTopK(), 6, this->attributes))
	{
		this->activated->setDescription("Shows the AI assistant window if activated.");
		this->startServicesOnActivate->setDescription("If true, runs Services Bat Path once when activated.");
		this->servicesBatPath->setDescription("Path to .bat that starts Qdrant/Ollama (optional).");
		this->askScriptPath->setDescription("Path to ask_once.py.");
	}

	NOWAAssistantComponent::~NOWAAssistantComponent()
	{
		
	}

	void NOWAAssistantComponent::initialise()
	{
	}

	const Ogre::String& NOWAAssistantComponent::getName() const
	{
		return this->name;
	}

	void NOWAAssistantComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<NOWAAssistantComponent>(NOWAAssistantComponent::getStaticClassId(), NOWAAssistantComponent::getStaticClassName());
	}

	void NOWAAssistantComponent::shutdown()
	{
	}

	void NOWAAssistantComponent::uninstall()
	{
	}

	void NOWAAssistantComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	Ogre::String NOWAAssistantComponent::getClassName(void) const
	{
		return NOWAAssistantComponent::getStaticClassName();
	}

	Ogre::String NOWAAssistantComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	bool NOWAAssistantComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Can only be added once
		if (gameObject->getComponentCount<NOWAAssistantComponent>() < 2)
		{
			return true;
		}
	}

	void NOWAAssistantComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
	{
		// Optional later. Keep empty for now.
	}

	bool NOWAAssistantComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RelativePosition")
		{
			this->relativePosition->setValue(XMLConverter::getAttribVector2(propertyElement, "data", Ogre::Vector2(0.2f, 0.2f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ParentId")
		{
			this->parentId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data", 0));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "StartServicesOnActivate")
		{
			this->startServicesOnActivate->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ServicesBatPath")
		{
			this->servicesBatPath->setValue(XMLConverter::getAttrib(propertyElement, "data", "Ai\\start_ai_services.bat"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PythonExe")
		{
			this->pythonExe->setValue(XMLConverter::getAttrib(propertyElement, "data", "python"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AskScriptPath")
		{
			this->askScriptPath->setValue(XMLConverter::getAttrib(propertyElement, "data", "Ai\\ask_once.py"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TopK")
		{
			this->topK->setValue(XMLConverter::getAttribInt(propertyElement, "data", 6));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr NOWAAssistantComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		// Not useful to clone assistant UI.
		return nullptr;
	}

	bool NOWAAssistantComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[NOWAAssistantComponent] Init component for game object: " + this->gameObjectPtr->getName());

		this->setActivated(this->activated->getBool());
		return true;
	}

	bool NOWAAssistantComponent::connect(void)
	{
		GameObjectComponent::connect();

		if (this->widget)
		{
			this->askButton->eventMouseButtonClick += MyGUI::newDelegate(this, &NOWAAssistantComponent::buttonHit);
			this->clearButton->eventMouseButtonClick += MyGUI::newDelegate(this, &NOWAAssistantComponent::buttonHit);
		}

		return true;
	}

	bool NOWAAssistantComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		if (this->askButton)
		{
			this->askButton->eventMouseButtonClick -= MyGUI::newDelegate(this, &NOWAAssistantComponent::buttonHit);
		}
		if (this->clearButton)
		{
			this->clearButton->eventMouseButtonClick -= MyGUI::newDelegate(this, &NOWAAssistantComponent::buttonHit);
		}
		return true;
	}

	bool NOWAAssistantComponent::onCloned(void)
	{
		return true;
	}

	void NOWAAssistantComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();
		this->destroyMyGUIWidgets();
	}

	void NOWAAssistantComponent::update(Ogre::Real dt, bool notSimulating)
	{
		// No per-frame logic needed.
	}

	void NOWAAssistantComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (NOWAAssistantComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (NOWAAssistantComponent::AttrRelativePosition() == attribute->getName())
		{
			this->setRelativePosition(attribute->getVector2());
		}
		else if (NOWAAssistantComponent::AttrParentId() == attribute->getName())
		{
			this->setParentId(attribute->getULong());
		}
		else if (NOWAAssistantComponent::AttrStartServicesOnActivate() == attribute->getName())
		{
			this->setStartServicesOnActivate(attribute->getBool());
		}
		else if (NOWAAssistantComponent::AttrServicesBatPath() == attribute->getName())
		{
			this->setServicesBatPath(attribute->getString());
		}
		else if (NOWAAssistantComponent::AttrPythonExe() == attribute->getName())
		{
			this->setPythonExe(attribute->getString());
		}
		else if (NOWAAssistantComponent::AttrAskScriptPath() == attribute->getName())
		{
			this->setAskScriptPath(attribute->getString());
		}
		else if (NOWAAssistantComponent::AttrTopK() == attribute->getName())
		{
			this->setTopK(attribute->getInt());
		}
	}

	void NOWAAssistantComponent::writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc)
	{
		GameObjectComponent::writeXML(propertiesXML, doc);

		using namespace rapidxml;

		xml_node<>* propertyXML = nullptr;

		// Activated (bool) -> type 12
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, Ogre::StringConverter::toString(this->activated->getBool()))));
		propertiesXML->append_node(propertyXML);

		// RelativePosition (vector2) -> type 8
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RelativePosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, Ogre::StringConverter::toString(this->relativePosition->getVector2()))));
		propertiesXML->append_node(propertyXML);

		// ParentId (ulong/int) -> type 2
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ParentId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, Ogre::StringConverter::toString(this->parentId->getULong()))));
		propertiesXML->append_node(propertyXML);

		// StartServicesOnActivate (bool) -> type 12
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "StartServicesOnActivate"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, Ogre::StringConverter::toString(this->startServicesOnActivate->getBool()))));
		propertiesXML->append_node(propertyXML);

		// ServicesBatPath (string) -> type 7
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ServicesBatPath"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->servicesBatPath->getString())));
		propertiesXML->append_node(propertyXML);

		// PythonExe (string) -> type 7
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PythonExe"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pythonExe->getString())));
		propertiesXML->append_node(propertyXML);

		// AskScriptPath (string) -> type 7
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AskScriptPath"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->askScriptPath->getString())));
		propertiesXML->append_node(propertyXML);

		// TopK (int) -> type 2
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TopK"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, Ogre::StringConverter::toString(this->topK->getInt()))));
		propertiesXML->append_node(propertyXML);
	}

	void NOWAAssistantComponent::setActivated(bool activatedValue)
	{
		this->activated->setValue(activatedValue);

		if (activatedValue)
		{
			if (!this->widget)
			{
				this->createMyGuiWidgets();
			}

			if (this->widget)
			{
				this->widget->setVisible(true);
			}

			this->startServicesIfRequested();
		}
		else
		{
			if (this->widget)
			{
				this->widget->setVisible(false);
			}
		}
	}

	bool NOWAAssistantComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void NOWAAssistantComponent::setRelativePosition(const Ogre::Vector2& relativePos)
	{
		this->relativePosition->setValue(relativePos);

		if (this->widget && !this->hasParent)
		{
			// Basic reposition (relative 0..1). Keep simple.
			const int w = this->widget->getWidth();
			const int h = this->widget->getHeight();
			const int screenW = (int)MyGUI::RenderManager::getInstancePtr()->getViewSize().width;
			const int screenH = (int)MyGUI::RenderManager::getInstancePtr()->getViewSize().height;

			int x = (int)(relativePos.x * (screenW - w));
			int y = (int)(relativePos.y * (screenH - h));
			this->widget->setPosition(x, y);
		}
	}

	Ogre::Vector2 NOWAAssistantComponent::getRelativePosition(void) const
	{
		return this->relativePosition->getVector2();
	}

	void NOWAAssistantComponent::setParentId(unsigned long pid)
	{
		this->parentId->setValue(pid);
	}

	unsigned long NOWAAssistantComponent::getParentId(void) const
	{
		return this->parentId->getULong();
	}

	void NOWAAssistantComponent::setStartServicesOnActivate(bool enabled)
	{
		this->startServicesOnActivate->setValue(enabled);
	}

	bool NOWAAssistantComponent::getStartServicesOnActivate(void) const
	{
		return this->startServicesOnActivate->getBool();
	}

	void NOWAAssistantComponent::setServicesBatPath(const Ogre::String& path)
	{
		this->servicesBatPath->setValue(path);
	}

	Ogre::String NOWAAssistantComponent::getServicesBatPath(void) const
	{
		return this->servicesBatPath->getString();
	}

	void NOWAAssistantComponent::setPythonExe(const Ogre::String& exe)
	{
		this->pythonExe->setValue(exe);
	}

	Ogre::String NOWAAssistantComponent::getPythonExe(void) const
	{
		return this->pythonExe->getString();
	}

	void NOWAAssistantComponent::setAskScriptPath(const Ogre::String& path)
	{
		this->askScriptPath->setValue(path);
	}

	Ogre::String NOWAAssistantComponent::getAskScriptPath(void) const
	{
		return this->askScriptPath->getString();
	}

	void NOWAAssistantComponent::setTopK(int k)
	{
		if (k < 1)
		{
			k = 1;
		}
		if (k > 20)
		{
			k = 20;
		}
		this->topK->setValue(k);
	}

	int NOWAAssistantComponent::getTopK(void) const
	{
		return this->topK->getInt();
	}

	MyGUI::Window* NOWAAssistantComponent::getWindow(void) const
	{
		return this->widget;
	}

	void NOWAAssistantComponent::createMyGuiWidgets(void)
	{
		this->destroyMyGUIWidgets();

		// If parentId is set, you can attach to an existing widget in your system.
		// For now we keep it simple: create root window.
		this->hasParent = false;

		MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstancePtr()->getViewSize();

		const int wndW = 640;
		const int wndH = 420;

		int x = (int)(this->relativePosition->getVector2().x * (viewSize.width - wndW));
		int y = (int)(this->relativePosition->getVector2().y * (viewSize.height - wndH));

		this->widget = MyGUI::Gui::getInstancePtr()->createWidget<MyGUI::Window>("WindowCS", MyGUI::IntCoord(x, y, wndW, wndH), MyGUI::Align::Default, "Overlapped");

		this->widget->setCaption("NOWA Assistant (Local RAG)");
		this->widgets.push_back(this->widget);

		// Prompt label
		{
			MyGUI::EditBox* lbl = this->widget->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(10, 32, wndW - 20, 22), MyGUI::Align::Default);
			lbl->setCaption("Prompt:");
			lbl->setEditReadOnly(true);
			lbl->setNeedMouseFocus(false);
			this->widgets.push_back(lbl);
		}

		// Prompt input
		this->promptEdit = this->widget->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(10, 58, wndW - 140, 26), MyGUI::Align::Default);
		this->promptEdit->setCaption("How do I enable Terra + roads in a scene?");
		this->widgets.push_back(this->promptEdit);

		// Ask button
		this->askButton = this->widget->createWidget<MyGUI::Button>("Button", MyGUI::IntCoord(wndW - 120, 58, 50, 26), MyGUI::Align::Default);
		this->askButton->setCaption("Ask");
		this->widgets.push_back(this->askButton);

		// Clear button
		this->clearButton = this->widget->createWidget<MyGUI::Button>("Button", MyGUI::IntCoord(wndW - 65, 58, 55, 26), MyGUI::Align::Default);
		this->clearButton->setCaption("Clear");
		this->widgets.push_back(this->clearButton);

		// Status label
		this->statusLabel = this->widget->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(10, 92, wndW - 20, 22), MyGUI::Align::Default);
		this->statusLabel->setCaption("Ready.");
		this->statusLabel->setEditReadOnly(true);
		this->statusLabel->setNeedMouseFocus(false);
		this->widgets.push_back(this->statusLabel);

		// Output (multi-line feel using EditBox)
		this->outputEdit = this->widget->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(10, 118, wndW - 20, wndH - 128), MyGUI::Align::Stretch);
		this->outputEdit->setEditReadOnly(true);
		this->outputEdit->setNeedMouseFocus(true);
		this->outputEdit->setCaption("Ask something to search your indexed NOWA files.\n");
		this->widgets.push_back(this->outputEdit);

		// Initial visibility
		this->widget->setVisible(this->activated->getBool());
	}

	void NOWAAssistantComponent::destroyMyGUIWidgets(void)
	{
		if (this->askButton)
		{
			this->askButton->eventMouseButtonClick -= MyGUI::newDelegate(this, &NOWAAssistantComponent::buttonHit);
		}
		if (this->clearButton)
		{
			this->clearButton->eventMouseButtonClick -= MyGUI::newDelegate(this, &NOWAAssistantComponent::buttonHit);
		}

		this->promptEdit = nullptr;
		this->outputEdit = nullptr;
		this->statusLabel = nullptr;
		this->askButton = nullptr;
		this->clearButton = nullptr;

		if (this->widget && !this->hasParent)
		{
			MyGUI::Gui::getInstancePtr()->destroyWidget(this->widget);
			this->widget = nullptr;
		}

		this->widgets.clear();
	}

	void NOWAAssistantComponent::buttonHit(MyGUI::Widget* sender)
	{
		if (sender == this->askButton)
		{
			this->runAskOnce();
		}
		else if (sender == this->clearButton)
		{
			if (this->outputEdit)
			{
				this->outputEdit->setCaption("");
			}
			if (this->statusLabel)
			{
				this->statusLabel->setCaption("Cleared.");
			}
		}
	}

	Ogre::String NOWAAssistantComponent::getPluginsRootFolder(void) const
	{
		// folder where THIS plugin module lives (likely .../bin/Release/plugins)
		return NOWA::Core::getSingletonPtr()->getModuleDirectoryFromAddress((const void*)&nowaAssistantModuleAnchor);
	}

	Ogre::String NOWAAssistantComponent::resolveToolPath(const Ogre::String& relativeToolPath) const
	{
		return NOWA::Core::getSingletonPtr()->resolveToolPathFromModule((const void*)&nowaAssistantModuleAnchor, relativeToolPath);
	}

	bool NOWAAssistantComponent::startServicesIfRequested(void)
	{
		if (false == this->startServicesOnActivate->getBool())
		{
			return false;
		}

		if (true == this->servicesStartedThisSession)
		{
			return true;
		}

		this->servicesStartedThisSession = true;

		if (this->statusLabel)
		{
			this->statusLabel->setCaption("Starting services...");
		}

		const Ogre::String batRel = this->servicesBatPath->getString();
		if (batRel.empty())
		{
			if (this->statusLabel)
			{
				this->statusLabel->setCaption("Services path empty.");
			}
			return false;
		}

		const Ogre::String batAbs = this->resolveToolPath(batRel);

#ifdef _WIN32
		HINSTANCE res = ShellExecuteA(nullptr, "open", batAbs.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
		if ((INT_PTR)res <= 32)
		{
			if (this->statusLabel)
			{
				this->statusLabel->setCaption("Failed to start services (ShellExecute error).");
			}
			return false;
		}
#else
		// ship .sh on linux
		Ogre::String cmd = "bash \"" + batAbs + "\"";
		int rc = std::system(cmd.c_str());
		if (rc != 0)
		{
			if (this->statusLabel)
			{
				this->statusLabel->setCaption("Failed to start services (system() error).");
			}
			return false;
		}
#endif

		if (this->statusLabel)
		{
			this->statusLabel->setCaption("Services start requested.");
		}
		return true;
	}


	void NOWAAssistantComponent::runAskOnce(void)
	{
		if (nullptr == this->promptEdit || nullptr == this->outputEdit)
		{
			return;
		}

		this->startServicesIfRequested();

		Ogre::String question = this->promptEdit->getCaption();
		if (question.empty())
		{
			question = "What is this project?";
		}

		if (this->statusLabel)
		{
			this->statusLabel->setCaption("Asking...");
		}

		Ogre::String py = this->pythonExe->getString();
		if (py.empty())
		{
			py = "python";
		}

		Ogre::String scriptRel = this->askScriptPath->getString();
		if (true == scriptRel.empty())
		{
			this->outputEdit->setCaption("ask_once.py path empty. Set AskScriptPath variant.");
			if (this->statusLabel)
			{
				this->statusLabel->setCaption("Error.");
			}
			return;
		}

		Ogre::String scriptAbs = this->resolveToolPath(scriptRel);

		// sanitize question for command line
		for (size_t i = 0; i < question.size(); ++i)
		{
			if (question[i] == '"')
			{
				question[i] = '\'';
			}
			if (question[i] == '\n' || question[i] == '\r')
			{
				question[i] = ' ';
			}
		}

		const int k = this->topK->getInt();

		std::ostringstream oss;
		oss << "\"" << py << "\" "
			<< "\"" << scriptAbs << "\" "
			<< "--question \"" << question << "\" "
			<< "--k " << k;

		Ogre::String output;
		int exitCode = -1;

		bool ok = NOWA::Core::getSingletonPtr()->execAndCaptureStdout(oss.str(), output, exitCode, true);

		if (false == ok)
		{
			output = "Failed to start process (pipe open failed).\n";
		}
		else
		{
			if (output.empty())
			{
				output = "No output.\n";
			}

			if (exitCode != 0)
			{
				output += "\n\n[Process failed] exitCode = ";
				output += Ogre::StringConverter::toString(exitCode);
				output += "\n(Scroll up for stderr/stdout. Most python errors are now captured.)\n";
			}
		}

		this->outputEdit->setCaption(output);

		if (this->statusLabel)
		{
			this->statusLabel->setCaption("Done.");
		}
	}

	Ogre::String NOWAAssistantComponent::sanitizeForCmdArg(const Ogre::String& in)
	{
		// Minimal sanitizing for command-line argument:
		// - replace " with '
		// - replace newlines with spaces
		Ogre::String s = in;
		for (size_t i = 0; i < s.size(); ++i)
		{
			if (s[i] == '"')
			{
				s[i] = '\'';
			}
			if (s[i] == '\n' || s[i] == '\r')
			{
				s[i] = ' ';
			}
		}
		return s;
	}

	Ogre::String NOWAAssistantComponent::execAndCaptureStdout(const Ogre::String& commandLine)
	{
		Ogre::String output;

#ifdef _WIN32
		FILE* pipe = _popen(commandLine.c_str(), "r");
#else
		FILE* pipe = popen(commandLine.c_str(), "r");
#endif
		if (!pipe)
		{
			return output;
		}

		char buffer[4096];
		while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
		{
			output += buffer;
		}

#ifdef _WIN32
		_pclose(pipe);
#else
		pclose(pipe);
#endif

		return output;
	}

} // namespace NOWA
