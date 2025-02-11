/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef SPEECHBUBBLECOMPONENT_H
#define SPEECHBUBBLECOMPONENT_H

#include "gameobject/GameObjectComponent.h"

#include "main/Events.h"
#include "OgrePlugin.h"

namespace NOWA
{
	class GameObjectTitleComponent;
	class SimpleSoundComponent;

	/**
	 * @class 	SpeechBubbleComponent
	 * @brief 	This component can be used to draw an speech bubble at the game object with an offset.
	 */
	class EXPORTED SpeechBubbleComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<SpeechBubbleComponent> SpeechBubbleComponentPtr;
	public:

		SpeechBubbleComponent();

		virtual ~SpeechBubbleComponent();

		/**
		* @see		Ogre::Plugin::install
		*/
		virtual void install(const Ogre::NameValuePairList* options) override;

		/**
		* @see		Ogre::Plugin::initialise
		*/
		virtual void initialise() override;

		/**
		* @see		Ogre::Plugin::shutdown
		*/
		virtual void shutdown() override;

		/**
		* @see		Ogre::Plugin::uninstall
		*/
		virtual void uninstall() override;

		/**
		* @see		Ogre::Plugin::getName
		*/
		virtual const Ogre::String& getName() const override;
		
		/**
		* @see		Ogre::Plugin::getAbiCookie
		*/
		virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		* @see		GameObjectComponent::postInit
		*/
		virtual bool postInit(void) override;

		/**
		* @see		GameObjectComponent::connect
		*/
		virtual bool connect(void) override;

		/**
		* @see		GameObjectComponent::disconnect
		*/
		virtual bool disconnect(void) override;

		/**
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void);

		/**
		* @see		GameObjectComponent::getClassName
		*/
		virtual Ogre::String getClassName(void) const override;

		/**
		* @see		GameObjectComponent::getParentClassName
		*/
		virtual Ogre::String getParentClassName(void) const override;

		/**
		* @see		GameObjectComponent::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		/**
		* @see		GameObjectComponent::update
		*/
		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("SpeechBubbleComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "SpeechBubbleComponent";
		}
	
		/**
		* @see		GameObjectComponent::canStaticAddComponent
		*/
		static bool canStaticAddComponent(GameObject* gameObject);

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "This component can be used to draw an speech bubble at the game object with an offset. A GameObjectTitle is required and optionally for sound, a SimpleSoundComponent is required.";
		}
		
		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

	public:

		/**
		 * @brief Sets whether to show the value bar.
		 * @param[in] activated if set to true the value bar will be rendered.
		 */
		virtual void setActivated(bool activated) override;

		/**
		 * @brief gets whether the value bar is renderd.
		 * @return activated true if the value bar is rendered rendered
		 */
		virtual bool isActivated(void) const override;

		/**
		 * @brief Sets the text caption.
		 * @param[in] caption The text caption to set
		 */
		void setCaption(const Ogre::String& caption);

		/**
		 * @brief Gets the text caption.
		 * @return caption The text caption to get
		 */
		Ogre::String getCaption(void) const;

		/**
		 * @brief Sets whether the speech text shall appear char by char running.
		 * @param[in] runSpeech The flag to set.
		 */
		void setRunSpeech(bool runSpeech);

		/**
		 * @brief Gets whether the speech text shall appear char by char running.
		 * @return runSpeech
		 */
		bool getRunSpeech(void) const;

		/**
		 * @brief Sets the speed duration. That is how long the bubble shall remain in seconds.
		 * @param[in] speedDurationSec The speech duration to set.
		 */
		void setSpeechDuration(Ogre::Real speechDurationSec);

		/**
		 * @brief Gets the speed duration. That is how long the bubble shall remain in seconds.
		 * @return speedDurationSec
		 */
		Ogre::Real getSpeechDuration(void) const;

		/**
		 * @brief Sets whether to use a sound if the speech is running char by char.
		 * @param[in] runSpeechSound The flag set.
		 */
		void setRunSpeechSound(bool runSpeechSound);

		/**
		 * @brief Gets whether to use a sound if the speech is running char by char.
		 * @return runSpeechSound
		 */
		bool getRunSpeechSound(void) const;

		/**
		 * @brief Sets whether the caption should remain after the speech run.
		 * @param[in] keepCaption The flag set.
		 */
		void setKeepCaption(bool keepCaption);

		/**
		 * @brief Gets whether the caption should is remained after the speech run.
		 * @return keepCaption
		 */
		bool getKeepCaption(void) const;

		/**
		 * @brief Lua closure function gets called if the speech is done.
		 * @param[in] closureFunction The closure function set.
		 */
		void reactOnSpeechDone(luabind::object closureFunction);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrCaption(void) { return "Caption"; }
		static const Ogre::String AttrRunSpeech(void) { return "Run Speech"; }
		static const Ogre::String AttrSpeechDuration(void) { return "Speech Duration"; }
		static const Ogre::String AttrRunSpeechSound(void) { return "Run Speech Sound"; }
		static const Ogre::String AttrKeepCaption(void) { return "Keep Caption"; }
	protected:
		virtual void drawSpeechBubble(Ogre::Real dt);

		void createSpeechBubble(void);

		void destroySpeechBubble(void);
	protected:
		Ogre::String name;

		Ogre::SceneNode* lineNode;
		Ogre::ManualObject* manualObject;
		GameObjectTitleComponent* gameObjectTitleComponent;
		SimpleSoundComponent* simpleSoundComponent;
		unsigned long indices;
		Ogre::Real currentCaptionWidth;
		Ogre::Real currentCaptionHeight;
		unsigned short currentCharIndex;
		Ogre::Real timeSinceLastRun;
		Ogre::Real timeSinceLastChar;
		bool couldDraw;
		bool speechDone;
		bool bIsInSimulation;

		luabind::object closureFunction;
		Variant* activated;
		Variant* caption;
		Variant* runSpeech;
		Variant* speechDuration;
		Variant* runSpeechSound;
		Variant* keepCaption;
	};

}; // namespace end

#endif

