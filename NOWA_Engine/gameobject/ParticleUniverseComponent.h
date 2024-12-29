#ifndef PARTICLE_UNIVERSE_COMPONENT_H
#define PARTICLE_UNIVERSE_COMPONENT_H

#include "GameObjectComponent.h"
#include "ParticleUniverseSystem.h"
#include "modules/ParticleUniverseModule.h"

namespace NOWA
{
	class EXPORTED ParticleUniverseComponent : public GameObjectComponent
	{
	public:
		typedef boost::shared_ptr<GameObject> GameObjectPtr;
		typedef boost::shared_ptr<ParticleUniverseComponent> ParticleUniverseCompPtr;
	public:

		ParticleUniverseComponent();

		virtual ~ParticleUniverseComponent();

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("ParticleUniverseComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "ParticleUniverseComponent";
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
			return "Usage: This Component is in order for playing nice particle effects using the ParticleUniverse-Engine.";
		}

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		virtual void setActivated(bool activated) override;

		virtual bool isActivated(void) const override;

		void setParticleTemplateName(const Ogre::String& particleTemplateName);

		Ogre::String getParticleTemplateName(void) const;

		void setRepeat(bool repeat);

		bool getRepeat(void) const;

		void setParticlePlayTimeMS(Ogre::Real playTime);

		Ogre::Real getParticlePlayTimeMS(void) const;

		void setParticlePlaySpeed(Ogre::Real playSpeed);

		Ogre::Real getParticlePlaySpeed(void) const;

		void setParticleOffsetPosition(const Ogre::Vector3& particleOffsetPosition);

		Ogre::Vector3 getParticleOffsetPosition(void);

		void setParticleOffsetOrientation(const Ogre::Vector3& particleOffsetOrientation);

		Ogre::Vector3 getParticleOffsetOrientation(void);

		void setParticleScale(const Ogre::Vector3& particleScale);

		Ogre::Vector3 getParticleScale(void);

		ParticleUniverse::ParticleSystem* getParticle(void) const;

		bool isPlaying(void) const;

		void setGlobalPosition(const Ogre::Vector3& particlePosition);

		void setGlobalOrientation(const Ogre::Vector3& particleOrientation);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrParticleName(void) { return "Particle Name"; }
		static const Ogre::String AttrRepeat(void) { return "Repeat"; }
		static const Ogre::String AttrPlayTime(void) { return "Play Time"; }
		static const Ogre::String AttrPlaySpeed(void) { return "Play Speed"; }
		static const Ogre::String AttrOffsetPosition(void) { return "Offset Position";}
		static const Ogre::String AttrOffsetOrientation(void) { return "Offset Orientation"; }
		static const Ogre::String AttrScale(void) { return "Scale"; }
	private:
		bool createParticleEffect(void);
		void destroyParticleEffect(void);
	private:
		ParticleUniverse::ParticleSystem* particle;
		Ogre::SceneNode* particleNode;
		Variant* activated;
		Variant* particleTemplateName;
		Variant* repeat;
		Ogre::Real particlePlayTime;
		Variant* particleInitialPlayTime;
		Variant* particlePlaySpeed;
		Variant* particleOffsetPosition;
		Variant* particleOffsetOrientation;
		Variant* particleScale;
		Ogre::String oldParticleTemplateName;
		bool oldActivated;
		bool bIsInSimulation;
	};

}; //namespace end

#endif