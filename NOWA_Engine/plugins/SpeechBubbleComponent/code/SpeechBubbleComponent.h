/*
Copyright (c) 2022 Lukas Kalinowski

The Massachusetts Institute of Technology ("MIT"), a non-profit institution of higher education, agrees to make the downloadable software and documentation, if any, 
(collectively, the "Software") available to you without charge for demonstrational and non-commercial purposes, subject to the following terms and conditions.

Restrictions: You may modify or create derivative works based on the Software as long as the modified code is also made accessible and for non-commercial usage. 
You may copy the Software. But you may not sublicense, rent, lease, or otherwise transfer rights to the Software. You may not remove any proprietary notices or labels on the Software.

No Other Rights. MIT claims and reserves title to the Software and all rights and benefits afforded under any available United States and international 
laws protecting copyright and other intellectual property rights in the Software.

Disclaimer of Warranty. You accept the Software on an "AS IS" basis.

MIT MAKES NO REPRESENTATIONS OR WARRANTIES CONCERNING THE SOFTWARE, 
AND EXPRESSLY DISCLAIMS ALL SUCH WARRANTIES, INCLUDING WITHOUT 
LIMITATION ANY EXPRESS OR IMPLIED WARRANTY OF MERCHANTABILITY, FITNESS 
FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS. 

MIT has no obligation to assist in your installation or use of the Software or to provide services or maintenance of any type with respect to the Software.
The entire risk as to the quality and performance of the Software is borne by you. You acknowledge that the Software may contain errors or bugs. 
You must determine whether the Software sufficiently meets your requirements. This disclaimer of warranty constitutes an essential part of this Agreement.
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
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

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
		 * @brief Sets the speed of the speech run.
		 * @param[in] runSpeed The run speed to set.
		 */
		void setRunSpeed(Ogre::Real runSpeed);

		/**
		 * @brief Gets the speed of the speech run.
		 * @return runSpeed
		 */
		Ogre::Real getRunSpeed(void) const;

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
		 * @brief Lua closure function gets called if the speech is done.
		 * @param[in] closureFunction The closure function set.
		 */
		void reactOnSpeechDone(luabind::object closureFunction);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrCaption(void) { return "Caption"; }
		static const Ogre::String AttrRunSpeech(void) { return "Run Speech"; }
		static const Ogre::String AttrRunSpeed(void) { return "Run Speed"; }
		static const Ogre::String AttrSpeechDuration(void) { return "Speech Duration"; }
		static const Ogre::String AttrRunSpeechSound(void) { return "Run Speech Sound"; }
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
		Variant* runSpeed;
		Variant* speechDuration;
		Variant* runSpeechSound;
	};

}; // namespace end

#endif

