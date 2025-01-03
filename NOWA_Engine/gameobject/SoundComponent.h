#ifndef SOUND_COMPONENT_H
#define SOUND_COMPONENT_H

#include "GameObjectComponent.h"
#include "OgreAL.h"

namespace NOWA
{

	class SoundComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<SoundComponent> SoundCompPtr;
	public:
		/*EXPORTED*/ SoundComponent();

		/*EXPORTED*/ virtual ~SoundComponent();

		/**
		 * @see		GameObjectComponent::init
		 */
		/*EXPORTED*/ virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		 * @see		GameObjectComponent::postInit
		 */
		/*EXPORTED*/ virtual bool postInit(void) override;

		/**
		* @see		GameObjectComponent::connect
		*/
		virtual bool connect(void) override;

		/**
		* @see		GameObjectComponent::disconnect
		*/
		virtual bool disconnect(void) override;

		/**
		 * @see		GameObjectComponent::getClassName
		 */
		/*EXPORTED*/ virtual Ogre::String getClassName(void) const override;

		/**
		 * @see		GameObjectComponent::getParentClassName
		 */
		virtual Ogre::String getParentClassName(void) const override;

		/**
		 * @see		GameObjectComponent::clone
		 */
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("SoundComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "SoundComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: With this component a dolby surround is created. More attributes are used for calibration.";
		}

		/**
		 * @see		GameObjectComponent::update
		 */
		/*EXPORTED*/ virtual void update(Ogre::Real dt, bool notSimulating = false) override { };

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		/*EXPORTED*/ virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		/*EXPORTED*/ virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		 * @see		GameObjectComponent::setActivated
		 */
		virtual void setActivated(bool activated);

		virtual bool isActivated(void) const override;

		void setSoundName(const Ogre::String& soundName);

		Ogre::String getSoundName(void) const;

		void setVolume(Ogre::Real volume);

		Ogre::Real getVolume(void) const;

		void setLoop(bool loop);

		bool getIsLooped(void) const;

		void setStream(bool stream);

		bool getIsStreamed(void) const;

		void setRelativeToListener(bool relativeToListener);

		bool getRelativeToListener(void) const;

		void setSoundTrigger(const Ogre::String& soundTrigger);

		Ogre::String getSoundTrigger(void) const;

		void setFadeInOutTime(const Ogre::Vector2& fadeInOutTime);

		Ogre::Vector2 getFadeInOutTime(void) const;

		void setInnerOuterConeAngle(const Ogre::Vector2& innerOuterConeAngle);

		Ogre::Vector2 getInnerOuterConeAngle(void) const;

		void setOuterConeGain(Ogre::Real outerConeGain);

		Ogre::Real getOuterConeGain(void) const;

		void setPitch(Ogre::Real pitch);

		Ogre::Real getPitch(void) const;

		void setPriority(int priority);

		int getPriority(void) const;

		void setDistanceValues(const Ogre::Vector3& distanceValues);

		Ogre::Vector3 getDistanceValues(void) const;

		void setSecondOffset(Ogre::Real secondOffset);

		Ogre::Real getSecondOffset(void) const;

		OgreAL::Sound* getSound(void) const;

	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrSoundName(void) { return "Sound Name"; }
		static const Ogre::String AttrVolume(void) { return "Volume"; }
		static const Ogre::String AttrLoop(void) { return "Loop"; }
		static const Ogre::String AttrStream(void) { return "Stream"; }
		static const Ogre::String AttrRelativeToListener(void) { return "Relative To Listener"; }
		static const Ogre::String AttrFadeInOutTime(void) { return "Fade In/Out Time"; }
		static const Ogre::String AttrSoundTrigger(void) { return "Trigger"; }
		static const Ogre::String AttrInnerOuterConeAngle(void) { return "Inner/Outer Cone Angle"; }
		static const Ogre::String AttrOuterConeGain(void) { return "Outer Cone Gain"; }
		static const Ogre::String AttrPitch(void) { return "Pitch"; }
		static const Ogre::String AttrPriority(void) { return "Priority"; }
		static const Ogre::String AttrDistanceValues(void) { return "Distance Values"; }
		static const Ogre::String AttrSecondOffset(void) { return "Second Offset"; }
	private:
		bool createSound(void);
		void destroySound(void);
	private:
		OgreAL::SoundManager* soundManager;
		OgreAL::Sound* sound;
		Ogre::String oldSoundName;
		Variant* activated;
		Variant* soundName;
		Variant* volume;
		Variant* loop;
		Variant* stream;
		Variant* relativeToListener;
		Variant* soundTrigger;
		Variant* fadeInOutTime;
		Variant* innerOuterConeAngle;
		Variant* outerConeGain;
		Variant* pitch;
		Variant* priority;
		Variant* distanceValues;
		Variant* secondOffset;
	};

}; //namespace end

#endif