/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef NOWAASSISTANTCOMPONENT_H
#define NOWAASSISTANTCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace MyGUI
{
	class Widget;
	class Window;
	class EditBox;
	class Button;
}

namespace NOWA
{
	/**
	  * @brief  Simple AI assistant UI for NOWA-Design.
	  *
	  * Shows a MyGUI window with a prompt and output area. On "Ask",
	  * runs a Python script (ask_once.py) that queries your local RAG (Qdrant + Ollama).
	  *
	  * Optionally starts services via a .bat file when activated.
	  */
	class EXPORTED NOWAAssistantComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<NOWAAssistantComponent> NOWAAssistantComponentPtr;

	public:
		NOWAAssistantComponent();
		virtual ~NOWAAssistantComponent();

		// Ogre::Plugin
		virtual void install(const Ogre::NameValuePairList* options) override;
		virtual void initialise() override;
		virtual void shutdown() override;
		virtual void uninstall() override;
		virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;
		virtual const Ogre::String& getName() const override;

		// GameObjectComponent
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;
		virtual bool postInit(void) override;
		virtual bool connect(void) override;
		virtual bool disconnect(void) override;
		virtual bool onCloned(void) override;
		virtual void onRemoveComponent(void) override;
		virtual Ogre::String getClassName(void) const override;
		virtual Ogre::String getParentClassName(void) const override;
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;
		virtual void update(Ogre::Real dt, bool notSimulating = false) override;
		virtual void actualizeValue(Variant* attribute) override;
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		virtual void setActivated(bool activated) override;
		virtual bool isActivated(void) const override;

		// Attributes (UI placement)
		void setRelativePosition(const Ogre::Vector2& relativePosition);
		Ogre::Vector2 getRelativePosition(void) const;

		void setParentId(unsigned long parentId);
		unsigned long getParentId(void) const;

		// Attributes (AI invocation)
		void setStartServicesOnActivate(bool enabled);
		bool getStartServicesOnActivate(void) const;

		void setServicesBatPath(const Ogre::String& path);
		Ogre::String getServicesBatPath(void) const;

		void setPythonExe(const Ogre::String& exe);
		Ogre::String getPythonExe(void) const;

		void setAskScriptPath(const Ogre::String& path);
		Ogre::String getAskScriptPath(void) const;

		void setTopK(int topK);
		int getTopK(void) const;

		MyGUI::Window* getWindow(void) const;

	public:
		// Static class info
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("NOWAAssistantComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "NOWAAssistantComponent";
		}

		static bool canStaticAddComponent(GameObject* gameObject);

		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: Shows a MyGUI prompt for local RAG (Qdrant + Ollama). "
				"Click 'Ask' to run a Python script that queries your indexed NOWA files. "
				"Optional: auto-start services via a .bat when activated.";
		}

		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

	public:
		static const Ogre::String AttrActivated(void)
		{
			return "Activated";
		}
		static const Ogre::String AttrRelativePosition(void)
		{
			return "RelativePosition";
		}
		static const Ogre::String AttrParentId(void)
		{
			return "ParentId";
		}

		static const Ogre::String AttrStartServicesOnActivate(void)
		{
			return "StartServicesOnActivate";
		}
		static const Ogre::String AttrServicesBatPath(void)
		{
			return "ServicesBatPath";
		}

		static const Ogre::String AttrPythonExe(void)
		{
			return "PythonExe";
		}
		static const Ogre::String AttrAskScriptPath(void)
		{
			return "AskScriptPath";
		}
		static const Ogre::String AttrTopK(void)
		{
			return "TopK";
		}

	private:
		void createMyGuiWidgets(void);
		void destroyMyGUIWidgets(void);

		void buttonHit(MyGUI::Widget* sender);

		void runAskOnce(void);
		bool startServicesIfRequested(void);

		static Ogre::String sanitizeForCmdArg(const Ogre::String& in);
		static Ogre::String execAndCaptureStdout(const Ogre::String& commandLine);

		Ogre::String getPluginsRootFolder(void) const;

		Ogre::String resolveToolPath(const Ogre::String& relativeToolPath) const;
	private:
		Ogre::String name;

		// UI
		MyGUI::VectorWidgetPtr widgets;
		MyGUI::Window* widget;
		MyGUI::EditBox* promptEdit;
		MyGUI::EditBox* outputEdit;
		MyGUI::EditBox* statusLabel;
		MyGUI::Button* askButton;
		MyGUI::Button* clearButton;

		bool hasParent;
		bool servicesStartedThisSession;

		// Variants
		Variant* activated;
		Variant* relativePosition;
		Variant* parentId;

		Variant* startServicesOnActivate;
		Variant* servicesBatPath;

		Variant* pythonExe;
		Variant* askScriptPath;
		Variant* topK;
	};

}; // namespace end

#endif
