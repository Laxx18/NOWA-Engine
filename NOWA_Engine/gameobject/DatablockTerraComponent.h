#ifndef DATA_BLOCK_TERRA_COMPONENT_H
#define DATA_BLOCK_TERRA_COMPONENT_H

#include "GameObjectComponent.h"
#include "OgreHlmsTerraDatablock.h"

namespace NOWA
{
	class TerraComponent;

	class EXPORTED DatablockTerraComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<DatablockTerraComponent> DatablockTerraCompPtr;
	public:
		DatablockTerraComponent();

		virtual ~DatablockTerraComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		/**
		* @see		GameObjectComponent::postInit
		*/
		virtual bool postInit(void) override;

		/**
		 * @see		GameObjectComponent::clone
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
		 * @see		GameObjectComponent::getParentClassName
		 */
		virtual Ogre::String getParentClassName(void) const override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("DatablockTerraComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "DatablockTerraComponent";
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
			return "Usage: Enables more detailed rendering customization for the mesh according physically based shading.";
		}

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
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		/**
		 * @brief Sets the brdf mode. The following values are possible:
		 *
		 * Most physically accurate BRDF we have. Good for representing
		 * the majority of materials.
		 * Uses:
		 *     * Roughness/Distribution/NDF term: GGX
		 *     * Geometric/Visibility term: Smith GGX Height-Correlated
		 *     * Normalized Disney Diffuse BRDF,see
		 *         "Moving Frostbite to Physically Based Rendering" from
		 *         Sebastien Lagarde & Charles de Rousiers
		 * Default
		 *
		 * Implements Cook-Torrance BRDF.
		 * Uses:
		 *     * Roughness/Distribution/NDF term: Beckmann
		 *     * Geometric/Visibility term: Cook-Torrance
		 *     * Lambertian Diffuse.
		 *
		 * Ideal for silk (use high roughness values), synthetic fabric
		 * CookTorrance
		 *
		 * Implements Normalized Blinn Phong using a normalization
		 * factor of (n + 8) / (8 * pi)
		 * The main reason to use this BRDF is performance. It's cheaper,
		 * while still looking somewhat similar to Default.
		 * If you still need more performance, see BlinnPhongLegacy
		 * BlinnPhong
		 *
		 * Same as Default, but the geometry term is not height-correlated
		 * which most notably causes edges to be dimmer and is less correct.
		 * Unity (Marmoset too?) use an uncorrelated term, so you may want to
		 * use this BRDF to get the closest look for a nice exchangeable
		 * pipeline workflow.
		 * DefaultUncorrelated
		 *
		 * Same as Default but the fresnel of the diffuse is calculated
		 * differently. Normally the diffuse component would be multiplied against
		 * the inverse of the specular's fresnel to maintain energy conservation.
		 * This has the nice side effect that to achieve a perfect mirror effect,
		 * you just need to raise the fresnel term to 1; which is very intuitive
		 * to artists (specially if using colored fresnel)
		 *
		 * When using this BRDF, the diffuse fresnel will be calculated differently,
		 * causing the diffuse component to still affect the color even when
		 * the fresnel = 1 (although subtly). To achieve a perfect mirror you will
		 * have to set the fresnel to 1 *and* the diffuse color to black;
		 * which can be unintuitive for artists.
		 *
		 * This BRDF is very useful for representing surfaces with complex refractions
		 * and reflections like glass, transparent plastics, fur, and surface with
		 * refractions and multiple rescattering that cannot be represented well
		 * with the default BRDF.
		 * DefaultSeparateDiffuseFresnel
		 *
		 * @see DefaultSeparateDiffuseFresnel. This is the same
		 * but the Cook Torrance model is used instead.
		 *
		 * Ideal for shiny objects like glass toy marbles, some types of rubber.
		 * silk, synthetic fabric.
		 * CookTorranceSeparateDiffuseFresnel
		 *
		 * Like DefaultSeparateDiffuseFresnel, but uses BlinnPhong as base.
		 * BlinnPhongSeparateDiffuseFresnel
		 *
		 * Implements traditional / the original non-PBR blinn phong:
		 *     * Looks more like a 2000-2005's game
		 *     * Ignores fresnel completely.
		 *     * Works with Roughness in range (0; 1]. We automatically convert
		 *       this parameter for you to shininess.
		 *     * Assumes your Light power is set to PI (or a multiple) like with
		 *       most other Brdfs.
		 *     * Diffuse & Specular will automatically be
		 *       multiplied/divided by PI for you (assuming you
		 *       set your Light power to PI).
		 * The main scenario to use this BRDF is:
		 *     * Performance. This is the fastest BRDF.
		 *     * You were using Default, but are ok with how this one looks,
		 *       so you switch to this one instead.
		 * BlinnPhongLegacyMath
		 *
		 * Implements traditional / the original non-PBR blinn phong:
		 *     * Looks more like a 2000-2005's game
		 *     * Ignores fresnel completely.
		 *     * Roughness is actually the shininess parameter; which is in range (0; inf)
		 *       although most used ranges are in (0; 500].
		 *     * Assumes your Light power is set to 1.0.
		 *     * Diffuse & Specular is unmodified.
		 * There are two possible reasons to use this BRDF:
		 *     * Performance. This is the fastest BRDF.
		 *     * You're porting your app from Ogre 1.x and want to maintain that
		 *       Fixed-Function look for some odd reason, and your materials
		 *       already dealt in shininess, and your lights are already calibrated.
		 *
		 * Important: If switching from Default to BlinnPhongFullLegacy, you'll probably see
		 * that your scene is too bright. This is probably because Default divides diffuse
		 * by PI and you usually set your lights' power to a multiple of PI to compensate.
		 * If your scene is too bright, kist divide your lights by PI.
		 * BlinnPhongLegacyMath performs that conversion for you automatically at
		 * material level instead of doing it at light level.
		 * BlinnPhongFullLegacy
		 *
		 * @param[in] brdf Sets the brdf to use
		 */
		void setBrdf(const Ogre::String& brdf);

		Ogre::String getBrdf(void) const;
		
		void setDiffuseColor(const Ogre::Vector3& diffuseColor);

		Ogre::Vector3 getDiffuseColor(void) const;

		void setDiffuseTextureName(const Ogre::String& diffuseTextureName);

		const Ogre::String getDiffuseTextureName(void) const;

		void setDetail0TextureName(const Ogre::String& detail0TextureName);

		const Ogre::String getDetail0TextureName(void) const;

		void setOffsetScale0(const Ogre::Vector4& offsetScale0);

		Ogre::Vector4 getOffsetScale0(void) const;
		
		void setDetail1TextureName(const Ogre::String& detail1TextureName);

		const Ogre::String getDetail1TextureName(void) const;

		void setOffsetScale1(const Ogre::Vector4& offsetScale1);

		Ogre::Vector4 getOffsetScale1(void) const;

		void setDetail2TextureName(const Ogre::String& detail2TextureName);

		const Ogre::String getDetail2TextureName(void) const;

		void setOffsetScale2(const Ogre::Vector4& offsetScale2);

		Ogre::Vector4 getOffsetScale2(void) const;
		
		void setDetail3TextureName(const Ogre::String& detail3TextureName);

		const Ogre::String getDetail3TextureName(void) const;

		void setOffsetScale3(const Ogre::Vector4& offsetScale3);

		Ogre::Vector4 getOffsetScale3(void) const;
		
		void setDetail0NMTextureName(const Ogre::String& detail0NMTextureName);

		const Ogre::String getDetail0NMTextureName(void) const;
		
		void setDetail1NMTextureName(const Ogre::String& detail1NMTextureName);

		const Ogre::String getDetail1NMTextureName(void) const;
		
		void setDetail2NMTextureName(const Ogre::String& detail2NMTextureName);

		const Ogre::String getDetail2NMTextureName(void) const;
		
		void setDetail3NMTextureName(const Ogre::String& detail3NMTextureName);

		const Ogre::String getDetail3NMTextureName(void) const;
		
		void setDetail0RTextureName(const Ogre::String& detail0RTextureName);

		const Ogre::String getDetail0RTextureName(void) const;
		
		void setRoughness0(Ogre::Real roughness0);

		Ogre::Real getRoughness0(void) const;
		
		void setDetail1RTextureName(const Ogre::String& detail1RTextureName);

		const Ogre::String getDetail1RTextureName(void) const;
		
		void setRoughness1(Ogre::Real roughness1);

		Ogre::Real getRoughness1(void) const;
		
		void setDetail2RTextureName(const Ogre::String& detail2RTextureName);

		const Ogre::String getDetail2RTextureName(void) const;
		
		void setRoughness2(Ogre::Real roughness2);

		Ogre::Real getRoughness2(void) const;
		
		void setDetail3RTextureName(const Ogre::String& detail3RTextureName);

		const Ogre::String getDetail3RTextureName(void) const;
		
		void setRoughness3(Ogre::Real roughness3);

		Ogre::Real getRoughness3(void) const;
		
		void setDetail0MTextureName(const Ogre::String& detail0MTextureName);

		const Ogre::String getDetail0MTextureName(void) const;
		
		void setMetalness0(Ogre::Real metalness0);

		Ogre::Real getMetalness0(void) const;
		
		void setDetail1MTextureName(const Ogre::String& detail1MTextureName);

		const Ogre::String getDetail1MTextureName(void) const;
		
		void setMetalness1(Ogre::Real metalness1);

		Ogre::Real getMetalness1(void) const;
		
		void setDetail2MTextureName(const Ogre::String& detail2MTextureName);

		const Ogre::String getDetail2MTextureName(void) const;
		
		void setMetalness2(Ogre::Real metalness2);

		Ogre::Real getMetalness2(void) const;
		
		void setDetail3MTextureName(const Ogre::String& detail3MTextureName);

		const Ogre::String getDetail3MTextureName(void) const;
		
		void setMetalness3(Ogre::Real metalness3);

		Ogre::Real getMetalness3(void) const;

		void setReflectionTextureName(const Ogre::String& reflectionTextureName);

		const Ogre::String getReflectionTextureName(void) const;

		Ogre::HlmsTerraDatablock* getDataBlock(void) const;

		void doNotCloneDataBlock(void);
	public:
		static const Ogre::String AttrBrdf(void) { return "Brdf"; }
		static const Ogre::String AttrDiffuseColor(void) { return "Diffuse Color"; }
		static const Ogre::String AttrDiffuse(void) { return "Diffuse Texture"; }
		static const Ogre::String AttrDetail0(void) { return "Detail 0 Texture"; }
		static const Ogre::String AttrDetail1(void) { return "Detail 1 Texture"; }
		static const Ogre::String AttrDetail2(void) { return "Detail 2 Texture"; }
		static const Ogre::String AttrDetail3(void) { return "Detail 3 Texture"; }
		static const Ogre::String AttrOffsetScale0(void) { return "Offset Scale 0"; }
		static const Ogre::String AttrOffsetScale1(void) { return "Offset Scale 1"; }
		static const Ogre::String AttrOffsetScale2(void) { return "Offset Scale 2"; }
		static const Ogre::String AttrOffsetScale3(void) { return "Offset Scale 3"; }
		static const Ogre::String AttrDetail0NM(void) { return "Detail 0 NM Texture"; }
		static const Ogre::String AttrDetail1NM(void) { return "Detail 1 NM Texture"; }
		static const Ogre::String AttrDetail2NM(void) { return "Detail 2 NM Texture"; }
		static const Ogre::String AttrDetail3NM(void) { return "Detail 3 NM Texture"; }
		static const Ogre::String AttrDetail0R(void) { return "Detail 0 R Texture"; }
		static const Ogre::String AttrRoughness0(void) { return "Roughness 0"; }
		static const Ogre::String AttrDetail1R(void) { return "Detail 1 R Texture"; }
		static const Ogre::String AttrRoughness1(void) { return "Roughness 1"; }
		static const Ogre::String AttrDetail2R(void) { return "Detail 2 R Texture"; }
		static const Ogre::String AttrRoughness2(void) { return "Roughness 2"; }
		static const Ogre::String AttrDetail3R(void) { return "Detail 3 R Texture"; }
		static const Ogre::String AttrRoughness3(void) { return "Roughness 3"; }
		static const Ogre::String AttrDetail0M(void) { return "Detail 0 M Texture"; }
		static const Ogre::String AttrMetalness0(void) { return "Metalness 0"; }
		static const Ogre::String AttrDetail1M(void) { return "Detail 1 M Texture"; }
		static const Ogre::String AttrMetalness1(void) { return "Metalness 1"; }
		static const Ogre::String AttrDetail2M(void) { return "Detail 2 M Texture"; }
		static const Ogre::String AttrMetalness2(void) { return "Metalness 2"; }
		static const Ogre::String AttrDetail3M(void) { return "Detail 3 M Texture"; }
		static const Ogre::String AttrMetalness3(void) { return "Metalness 3"; }
		static const Ogre::String AttrReflection(void) { return "Reflection Texture"; }
	private:
		void preReadDatablock(void);
		bool readDatablockTerra(Ogre::Terra* terra);
		bool readDefaultDatablockTerra(void);
		void postReadDatablock(void);
		Ogre::String getTerraTextureName(Ogre::HlmsTerraDatablock* datablock, Ogre::TerraTextureTypes type);
		void internalSetTextureName(Ogre::TerraTextureTypes terraTextureType, Ogre::CommonTextureTypes::CommonTextureTypes textureType, Variant* attribute, const Ogre::String& textureName);
		Ogre::String mapBrdfToString(Ogre::TerraBrdf::TerraBrdf brdf);
		Ogre::TerraBrdf::TerraBrdf mapStringToBrdf(const Ogre::String& strBrdf);
	protected:
		Variant* brdf; // List selection
		Variant* diffuseColor;
		Variant* diffuseTextureName;
		
		Variant* detail0TextureName;
		Variant* detail1TextureName;
		Variant* detail2TextureName;
		Variant* detail3TextureName;

		Variant* offsetScale0;
		Variant* offsetScale1;
		Variant* offsetScale2;
		Variant* offsetScale3;
		
		Variant* detail0NMTextureName;
		Variant* detail1NMTextureName;
		Variant* detail2NMTextureName;
		Variant* detail3NMTextureName;
		Variant* detail0RTextureName;
		
		Variant* roughness0;
		Variant* detail1RTextureName;
		Variant* roughness1;
		Variant* detail2RTextureName;
		Variant* roughness2;
		Variant* detail3RTextureName;
		Variant* roughness3;
		
		Variant* detail0MTextureName;
		Variant* metalness0;
		Variant* detail1MTextureName;
		Variant* metalness1;
		Variant* detail2MTextureName;
		Variant* metalness2;
		Variant* detail3MTextureName;
		Variant* metalness3;
		Variant* reflectionTextureName;
		
		Ogre::HlmsTerraDatablock* datablock;
		Ogre::HlmsTerraDatablock* originalDatablock;
		bool alreadyCloned;
		bool newlyCreated;
	};

}; //namespace end

#endif