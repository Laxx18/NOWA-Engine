#ifndef PLANAR_REFLECTION_COMPONENT_H
#define PLANAR_REFLECTION_COMPONENT_H

#include "PlaneComponent.h"
#include "OgrePlanarReflections.h"

namespace NOWA
{
	class EXPORTED PlanarReflectionComponent : public GameObjectComponent
	{
	public:

		friend class WorkspaceBaseComponent;

		typedef boost::shared_ptr<NOWA::PlanarReflectionComponent> PlanarReflectionCompPtr;
	public:
	
		PlanarReflectionComponent();

		virtual ~PlanarReflectionComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		* @see		GameObjectComponent::postInit
		*/
		virtual bool postInit(void) override;

		/**
		*	 @see		GameObjectComponent::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		/**
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void) override;

		/**
		* @see		GameObjectComponent::getClassName
		*/
		virtual Ogre::String getClassName(void) const override;

		/**
		*	 @see		GameObjectComponent::getParentClassName
		*/
		virtual Ogre::String getParentClassName(void) const override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("PlanarReflectionComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "PlanarReflectionComponent";
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
			return "Usage: This component is used to create a mirror effect. ";
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
		*	 @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		void setUseAccurateLighting(bool accurateLighting);

		bool getUseAccurateLighting(void) const;
		
		void setImageSize(const Ogre::Vector2& imageSize);

		Ogre::Vector2 getImageSize(void) const;
		
		void setUseMipmaps(bool useMipmaps);

		bool getUseMipmaps(void) const;
		
		void setUseMipmapMethodCompute(bool useMipmapMethodCompute);

		bool getUseMipmapMethodCompute(void) const;
		
		void setMirrorSize(const Ogre::Vector2& mirrorSize);
		
		Ogre::Vector2 getMirrorSize(void) const;

		void setDatablockName(const Ogre::String& datablockName);
		
		Ogre::String getDatablockName(void) const;

		void setTransformUpdateRateSec(Ogre::Real transformUpdateRateSec);

		Ogre::Real getTransformUpdateRateSec(void) const;
		
	public:
		static const Ogre::String AttrAccurateLighting(void) { return "Accurate Lighting:"; }
		static const Ogre::String AttrImageSize(void) { return "Image Size:"; }
		static const Ogre::String AttrMipmaps(void) { return "Mipmaps:"; }
		static const Ogre::String AttrMipmapMethodCompute(void) { return "Mipmaps Method Compute:"; }
		static const Ogre::String AttrMirrorSize(void) { return "Mirror Size:"; }
		static const Ogre::String AttrDatablockName(void) { return "Datablock Name:"; }
		static const Ogre::String AttrTransformUpdateRateSec(void) { return "Transform Update Rate (Sec):"; }
	private:
		void createPlane(void);

		void actualizePlanarReflection(void);
	private:
		Variant* useAccurateLighting;
		Variant* imageSize;
		Variant* useMipmaps;
		Variant* useMipmapMethodCompute;
		Variant* mirrorSize;
		Variant* datablockName;
		Variant* transformUpdateRateSec;
		Ogre::Real transformUpdateTimer;
		Ogre::Vector3 oldPosition;
		Ogre::Quaternion oldOrientation;
		Ogre::PlanarReflections::TrackedRenderable* trackedRenderable;
		bool planarReflectionMeshCreated;
	};

}; //namespace end

#endif