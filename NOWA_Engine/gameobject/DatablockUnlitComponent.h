#ifndef DATA_BLOCK_UNLIT_COMPONENT_H
#define DATA_BLOCK_UNLIT_COMPONENT_H

#include "GameObjectComponent.h"

namespace NOWA
{
	class EXPORTED DatablockUnlitComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<DatablockUnlitComponent> DatablockUnlitCompPtr;
	public:
		DatablockUnlitComponent();

		virtual ~DatablockUnlitComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		* @see		GameObjectComponent::postInit
		*/
		virtual bool postInit(void) override;

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
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void);

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("DatablockUnlitComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "DatablockUnlitComponent";
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
			return "Usage: Enables more detailed rendering customization for the mesh without lighting.";
		}

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		void setSubEntityIndex(unsigned int subEntityIndex);

		unsigned int getSubEntityIndex(void) const;
		
		void setUseColor(bool useColor);

		bool getUseColor(void) const;

		void setBackgroundColor(const Ogre::Vector4& backgroundColor);

		Ogre::Vector4 getBackgroundColor(void) const;

		void setTextureCount(unsigned int itemCount);

		unsigned int getTextureCount(void) const;
		
		/**
		 * @brief Sets the detail 0 texture Name
		 * @param[in] detail0TextureName	The detail 0 texture name to set
		 * @note Similar: detail_map1, detail_map2, detail_map3 Name of the detail map to be used on top of the diffuse colur. There can be gaps (i.e. set detail maps 0 and 2 but not 1)
		 */
		void setDetailTextureName(unsigned int index, const Ogre::String& detailTextureName);

		const Ogre::String getDetailTextureName(unsigned int index) const;

		/**
		 * @brief Sets the blend mode of a detail map.
		 * @param[in]	blendMode The blend mode to set as string
		 * 				The following values are possible:
		 * 					'UNLIT_BLEND_NORMAL_NON_PREMUL'
		 * 					'UNLIT_BLEND_NORMAL_PREMUL'
		 * 					'UNLIT_BLEND_ADD'
		 * 					'UNLIT_BLEND_SUBTRACT'
		 * 					'UNLIT_BLEND_MULTIPLY'
		 * 					'UNLIT_BLEND_MULTIPLY2X'
		 * 					'UNLIT_BLEND_SCREEN'
		 * 					'UNLIT_BLEND_OVERLAY'
		 * 					'UNLIT_BLEND_LIGHTEN'
		 * 					'UNLIT_BLEND_DARKEN'
		 * 					'UNLIT_BLEND_GRAIN_EXTRACT'
		 * 					'UNLIT_BLEND_GRAIN_MERGE'
		 * 					'UNLIT_BLEND_DIFFERENCE'
		 */
		void setBlendMode(unsigned int index, const Ogre::String& blendMode);

		Ogre::String getBlendMode(unsigned int index) const;
		
		void setTextureSwizzle(unsigned int index, const Ogre::Vector4& texureSwizzle);

		Ogre::Vector4 getTextureSwizzle(unsigned int index) const;
		
		void setUseAnimation(unsigned int index, bool useAnimation);

		bool getUseAnimation(unsigned int index) const;
		
		void setAnimationTranslation(unsigned int index, const Ogre::Vector3& animationTranslation);
		
		Ogre::Vector3 getAnimationTranslation(unsigned int index) const;
		
		void setAnimationScale(unsigned int index, const Ogre::Vector3& animationScale);
		
		Ogre::Vector3 getAnimationScale(unsigned int index) const;
		
		void setAnimationRotation(unsigned int index, const Ogre::Vector3& animationRotation);
		
		Ogre::Vector3 getAnimationRotation(unsigned int index) const;
		
		// Attention: What about: void setTextureUvSource( uint8 sourceType, uint8 uvSet ); uint8 getTextureUvSource( uint8 sourceType ) const; ??
		
		Ogre::HlmsDatablock* getDataBlock(void) const;

		void doNotCloneDataBlock(void);
	public:
		static const Ogre::String AttrSubEntityIndex(void) { return "Sub-Entity Index"; }
		static const Ogre::String AttrUseColor(void) { return "Use Color"; }
		static const Ogre::String AttrBackgroundColor(void) { return "Background Color"; }
		static const Ogre::String AttrTextureCount(void) { return "Texture Count"; }
		static const Ogre::String AttrDetailTexture(void) { return "Detail Texture "; }
		static const Ogre::String AttrBlendMode(void) { return "Blend Mode "; }
		static const Ogre::String AttrTextureSwizzle(void) { return "Texture Swizzle (r, g, b, a) "; }
		static const Ogre::String AttrUseAnimation(void) { return "Use Animation "; }
		static const Ogre::String AttrAnimationTranslation(void) { return "Animation Translation "; }
		static const Ogre::String AttrAnimationScale(void) { return "Animation Scale "; }
		static const Ogre::String AttrAnimationRotation(void) { return "Animation Rotation "; }
	private:
		void preReadDatablock(void);
		bool readDatablockEntity(Ogre::v1::Entity* entity);
		void postReadDatablock(void);
		Ogre::String getUnlitTextureName(Ogre::HlmsUnlitDatablock* datablock, unsigned char textureIndex);
		void internalSetTextureName(unsigned char textureIndex, Ogre::CommonTextureTypes::CommonTextureTypes textureType, Variant* attribute, const Ogre::String& textureName);
		Ogre::String mapBlendModeToString(Ogre::UnlitBlendModes blendMode);
		Ogre::UnlitBlendModes mapStringToBlendMode(const Ogre::String& strBlendMode);
	protected:
		Variant* subEntityIndex;
		Variant* useColor;
		Variant* backgroundColor;
		Variant* textureCount;
		std::vector<Variant*> detailTextureNames;
		// Up to 16 textures are supported, but who the heck needs 16 textures layered??
		std::vector<Variant*> blendModes; // List selection
		std::vector<Variant*> textureSwizzles;
		std::vector<Variant*> useAnimations;
		std::vector<Variant*> animationTranslations;
		std::vector<Variant*> animationScales;
		std::vector<Variant*> animationRotations;
		
		Ogre::HlmsUnlitDatablock* datablock;
		Ogre::HlmsUnlitDatablock* originalDatablock;
		bool alreadyCloned;
		bool isCloned;
		bool newlyCreated;
		unsigned int oldSubIndex;
	};

}; //namespace end

#endif