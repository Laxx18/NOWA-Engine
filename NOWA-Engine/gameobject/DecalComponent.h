#ifndef DECAL_COMPONENT_H
#define DECAL_COMPONENT_H

#include "GameObjectComponent.h"
#include "OgreDecal.h"

namespace NOWA
{
	class EXPORTED DecalComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<NOWA::DecalComponent> DecalCompPtr;
	public:
	
		DecalComponent();

		virtual ~DecalComponent();

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("DecalComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "DecalComponent";
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
			return "Usage: Renders a decal.";
		}

		/**
		 * @see		GameObjectComponent::update
		 */
		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		void setDiffuseTextureName(const Ogre::String& diffuseTextureName);

		Ogre::String getDiffuseTextureName(void) const;
		
		void setNormalTextureName(const Ogre::String& normalTextureName);

		Ogre::String getNormalTextureName(void) const;
		
		void setEmissiveTextureName(const Ogre::String& emissiveTextureName);

		Ogre::String getEmissiveTextureName(void) const;
		
		void setIgnoreAlpha(bool ignoreAlpha);

		bool getIgnoreAlpha(void) const;
		
		void setMetalness(Ogre::Real metalness);
		
        Ogre::Real getMetalness(void) const;

        void setRoughness(Ogre::Real roughness);
		
        Ogre::Real getRoughness(void) const;
		
		void setRectSize(const Ogre::Vector3& rectSizeAndDimensions);
		
		Ogre::Vector3 getRectSize(void) const;
	public:
		static const Ogre::String AttrDiffuseTextureName(void) { return "Diffuse Tex. Name:"; }
		static const Ogre::String AttrNormalTextureName(void) { return "Normal Tex. Name:"; }
		static const Ogre::String AttrEmissiveTextureName(void) { return "Emissive Tex. Name:"; }
		static const Ogre::String AttrIgnoreAlpha(void) { return "Ignore Alpha:"; }
		static const Ogre::String AttrMetalness(void) { return "Metalness:"; }
		static const Ogre::String AttrRoughness(void) { return "Roughness:"; }
		static const Ogre::String AttrRectSize(void) { return "Rect size x, y, depth:"; }
	private:
		void createDecal(void);
	private:
		Ogre::Decal* decal;

		Variant* diffuseTextureName;
		Variant* normalTextureName;
		Variant* emissiveTextureName;
		Variant* ignoreAlpha;
		Variant* metalness;
		Variant* roughness;
		Variant* rectSize;
	};

}; //namespace end

#endif