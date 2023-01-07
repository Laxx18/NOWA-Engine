#ifndef LENS_FLARE_COMPONENT_H
#define LENS_FLARE_COMPONENT_H

#include "GameObjectComponent.h"
#include "main/Events.h"

namespace NOWA
{
	class EXPORTED LensFlareComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<NOWA::LensFlareComponent> LensFlareCompPtr;
	public:
	
		LensFlareComponent();

		virtual ~LensFlareComponent();

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

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("LensFlareComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "LensFlareComponent";
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
			return "Usage: Creates a lens flare effect.";
		}

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

		/**
		 * @see		GameObjectComponent::setActivated
		 */
		virtual void setActivated(bool activated) override;

		/**
		* @brief Gets whether the component behaviour is activated or not.
		* @return activated True, if the components behaviour is activated, else false
		*/
		virtual bool isActivated(void) const override;

		void setLightId(unsigned long lightId);

		unsigned long getLightId(void) const;
		
		void setCameraId(unsigned long cameraId);

		unsigned long getCameraId(void) const;

		void setHaloColor(const Ogre::Vector3& haloColor);

		Ogre::Vector3 getHaloColor(void) const;

		void setBurstColor(const Ogre::Vector3& burstColor);

		Ogre::Vector3 getBurstColor(void) const;

		Ogre::v1::BillboardSet* getHaloBillboard(void) const;
		
		Ogre::v1::BillboardSet* getBurstBillboard(void) const;
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrLightId(void) { return "Light Id"; }
		static const Ogre::String AttrCameraId(void) { return "Light Id"; }
		static const Ogre::String AttrHaloColor(void) { return "Halo Color"; }
		static const Ogre::String AttrBurstColor(void) { return "Burst Color"; }
	private:
		void createLensFlare(void);
		
		void handleRemoveCamera(NOWA::EventDataPtr eventData);
	private:
		Ogre::Light* light;
		Ogre::Camera* camera;
		Ogre::HlmsUnlitDatablock* datablock;
		
		Ogre::ColourValue color;
		Ogre::v1::BillboardSet* haloSet;
		Ogre::v1::BillboardSet* burstSet;
		bool hidden;

		Variant* activated;
		Variant* lightId;
		Variant* cameraId;
		Variant* haloColor;
		Variant* burstColor;
	};

}; //namespace end

#endif