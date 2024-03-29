#ifndef DATA_BLOCK_PBS_COMPONENT_H
#define DATA_BLOCK_PBS_COMPONENT_H

#include "GameObjectComponent.h"

// TODO: Print info for each GO, which kind of datablock is uses, separate also unlit in own datablock component and terra

namespace NOWA
{
	class EXPORTED DatablockPbsComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<DatablockPbsComponent> DatablockPbsCompPtr;
	public:
		DatablockPbsComponent();

		virtual ~DatablockPbsComponent();

		/**
		* @see		GameObjectComponent::init(rapidxml
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
			return NOWA::getIdFromName("DatablockPbsComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "DatablockPbsComponent";
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
			return "Usage: Enables more detailed rendering customization for the mesh according physically based shading. "
				   "Requirements: A kind of game object with mesh.";
		}

		/**
		* @see		GameObjectComponent::update(rapidxml
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

		void setSubEntityIndex(unsigned int subEntityIndex);

		unsigned int getSubEntityIndex(void) const;

		/**
		 *   @brief Sets whether to use a specular workflow, or a metallic workflow.
		 *   When in metal workflow, the texture is used as a metallic texture,
		 *   and is expected to be a monochrome texture. Global specularity's strength
		 *   can still be affected via setSpecular. The fresnel settings should not
		 *   be used (metalness is stored where fresnel used to).
		 *
		 *	Specular workflow where the specular texture is addressed to the fresnel
		 *  instead of kS. This is normally referred as simply Specular workflow
		 *  in many other PBRs.
		 *
		 *  When in specular workflow, the texture is used as a specular texture,
		 *  and is expected to be either colored or monochrome.
		 *  setMetalness should not be called in this mode.
		 *  @param[in] workflow The workflow as string to set. Possible values are: 'SpecularWorkflow', 'SpecularAsFresnelWorkflow', 'MetallicWorkflow'
		 */
		void setWorkflow(const Ogre::String& workflow);

		Ogre::String getWorkflow(void) const;

		/**
		 * @brief Sets the metalness in a metallic workflow. Overrides any fresnel value. Only visible/usable when metallic workflow is active.
		 * @param[in] metalness Value in range [0; 1]
		 */
		void setMetalness(Ogre::Real metalness);

		Ogre::Real getMetalness(void) const;

		/**
		 * @brief Specifies the roughness value. Should be in range (0; inf)
		 * @param[in] roughness Value in range (0; inf)
		 * @note	  Values extremely close to zero could cause NaNs and INFs in the pixel shader, also depends on the GPU's precision.
		 */
		void setRoughness(Ogre::Real roughness);

		Ogre::Real getRoughness(void) const;

		/**
		 * @brief 		Sets the fresnel (F0) directly, instead of using the IOR. @See setIndexOfRefraction. If "separateFresnel" was different from the current setting, it will call.
		 * 				Only visible/usable when fresnel workflow is active.
		 * @param[in] 	fresnel The fresnel of the material for each color component. When separateFresnel = false, the Y and Z components are ignored.
		 * @param[in] 	separateFresnel Whether to use the same fresnel term for RGB channel, or individual ones.
		*/
		void setFresnel(const Ogre::Vector3& fresnel, bool separateFresnel);

		/**
		 * @brief 		Gets the fresnel configuration.
		 * 				Only visible/usable when fresnel workflow is active.
		 * @return	 	fresnelConfiguration The x,y,z of the vector describing the r,g,b color components and the w value whether separateFresnel is on or off.
		 */
		Ogre::Vector4 getFresnel(void) const;

		 /** Calculates fresnel (F0 in most books) based on the IOR.
            The formula used is ( (1 - idx) / (1 + idx) )²
        @remarks
            If "separateFresnel" was different from the current setting, it will call
            @see HlmsDatablock::flushRenderables. If the another shader must be created,
            it could cause a stall.
        @param refractionIdx
            The index of refraction of the material for each colour component.
            When separateFresnel = false, the Y and Z components are ignored.
        @param separateFresnel
            Whether to use the same fresnel term for RGB channel, or individual ones.
        */
        void setIndexOfRefraction(const Ogre::Vector3& refractionIdx, bool separateFresnel);

		/**
		 * @brief 		Gets the index of refraction configuration.
		 * 				Only visible/usable when fresnel workflow is active.
		 * @return	 	indexOfRefractionConfiguration The x,y,z of the vector describing the r,g,b color components and the w value whether indexOfRefraction is on or off.
		 */
		Ogre::Vector4 getIndexOfRefraction(void) const;

		/** 
		 * @brief Sets the strength of the refraction, i.e. how much displacement in screen space.
         *  This value is not physically based.
         *  Only used when HlmsPbsDatablock::setTransparency was set to HlmsPbsDatablock::Refractive
         * @param strength
         *   Refraction strength. Useful range is often (0; 1) but any value is valid (even negative),
         *   but the bigger the number, the more likely glitches will appear (with large values
         *   we have to fallback to regular alpha blending due to the screen space pixel landing
         *   outside the screen)
         */
        void setRefractionStrength(Ogre::Real refractionStrength);

		/** 
		 * @brief Returns the refraction strength.
		 * @return refractionStrength	The refraction strength to get
		 */
		Ogre::Real getRefractionStrength(void) const;

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

		/** Sets the scale and offset of the detail map.
		@remarks
			A value of Vector4( 0, 0, 1, 1 ) will cause a flushRenderables as we
			remove the code from the shader.
		@param detailMap
			Value in the range [0; 4)
		@param offsetScale
			XY = Contains the UV offset.
			ZW = Constains the UV scale.
			Default value is Vector4( 0, 0, 1, 1 )
		*/
		// void setDetailMapOffsetScale( uint8 detailMap, const Vector4& offsetScale );

		/**
		 * @brief Allows support for two sided lighting. Disabled by default (faster)
		 *
		 * @param[in] twoSided 				Whether to enable or disable.
		 */
		void setTwoSidedLighting(bool twoSided);

		bool getTwoSidedLighting(void) const;

		/**
		 * @brief Sets the one sided shadow culling mode
		 *
		 * @param[in] oneSidedShadowCast	If changeMacroblock is set to true, this parameter controls the culling mode of the shadow caster (the setting of HlmsManager::setShadowMappingUseBackFaces is ignored!).
		 *   								While oneSidedShadowCast == CULL_NONE is usually the "correct" option, setting oneSidedShadowCast = CULL_ANTICLOCKWISE can prevent ugly self-shadowing on interiors.
		 */
		void setOneSidedShadowCastCullingMode(const Ogre::String& cullingMode);

		Ogre::String getOneSidedShadowCastCullingMode(void) const;

	   /* @brief Sets the alpha test to the given compare function. CMPF_ALWAYS_PASS means disabled.
		*	@see mAlphaTestThreshold.
		*	Calling this function triggers a HlmsDatablock::flushRenderables
		* @remarks
		*	It is to the derived implementation to actually implement the alpha test.
		* @param[in] compareFunction
		*	Compare function to use. Default is CMPF_ALWAYS_PASS, which means disabled.
		*	Note: CMPF_ALWAYS_FAIL is not supported. Set a negative threshold to
		*	workaround this issue.
		*/
		void setAlphaTest(const Ogre::String& alphaTest);

		Ogre::String getAlphaTest(void) const;

		/**
		 * @brief 	Alpha testing works on the alpha channel of the diffuse texture. If there is no diffuse texture, the first diffuse detail map after
		 *  		applying the blend weights (texture + params) is used. If there are no diffuse nor detail-diffuse maps, the alpha test is compared against the value 1.0.
		 * @param[in] threshold The threshold to set (max. 1.0)
		 */
		void setAlphaTestThreshold(Ogre::Real threshold);

		Ogre::Real getAlphaTestThreshold(void) const;
		
		/**
		 * @brief Sets whether to receive shadows for this data block or not. When false, objects with this material will not receive shadows (independent of whether they case shadows or not)
		 * @param[in] receiveShadows	Whether to enable or disable receiving shadows
		 */
		void setReceiveShadows(bool receiveShadows);

		bool getReceiveShadows(void) const;

		/** Manually set a probe to affect this particular material.
		@remarks
			PCC (Parallax Corrected Cubemaps) have two main forms of operation: Auto and manual.
			They both have advantages and disadvantages. This method allows you to enable
			the manual mode of operation.
		@par
			Manual Advantages:
				* It's independent of camera position.
				* The reflections are always visible and working on the object.
				* Works best for static objects
				* Also works well on dynamic objects that you can guarantee are going
				  to be constrained to the probe's area.
			Manual Disadvantages:
				* Needs to be manually applied on the material by the user.
				* Can produce harsh lighting/reflection seams when two objects affected
				  by different probes are close together.
				* Sucks for dynamic objects.

			To use manual probes just call:
				datablock->setCubemapProbe( probe );
		@par
			Auto Advantages:
				* Smoothly blends between probes, making smooth transitions
				* Avoids seams.
				* No need to change anything on the material or the object,
				  you don't need to do anything.
				* Works best for dynamic objects (eg. characters)
				* Also works well on static objects if the camera is inside rooms/corridors, thus
				  blocking the view from distant rooms that aren't receiving reflections.
			Auto Disadvantages:
				* Objects that are further away won't have reflections as
				  only one probe can be active.
				* It depends on the camera's position.
				* Doesn't work well if the user can see many distant objects at once, as they
				  won't have reflections until you get close.

			To use Auto you don't need to do anything. Just enable PCC:
				hlmsPbs->setParallaxCorrectedCubemap( mParallaxCorrectedCubemap );
			and leave PBSM_REFLECTION empty and don't enable manual mode.
		@par
			When other reflection methods can be used as fallback (Planar reflections, SSR),
			then usually auto will be the preferred choice.
			But if multiple reflections/fallbacks aren't available, you'll likely have to make
			good use of manual and auto
		@param probe
			The probe that should affect this material to enable manual mode.
			Null pointer to disable manual mode and switch to auto.
		*/
		// void setCubemapProbe( CubemapProbe *probe );
		
		/**
		 * @brief Sets the diffuse color (final multiplier). The color will be divided by PI for energy conservation.
		 * @param[in] diffuseColor	The (r, g, b) diffuse color to set.
		 * @note "kD" in most books about PBS. Internally the diffuse color is divided by PI.
		 */
		void setDiffuseColor(const Ogre::Vector3& diffuseColor);

		Ogre::Vector3 getDiffuseColor(void) const;

		/**
		 * @brief Sets the diffuse background colour. When no diffuse texture is present, this solid colour replaces it, and can act as a background for the detail maps.
		 * @param[in] backgroundColor	The (r, g, b) background color to set.
		 * @note Does not replace 'diffuse <r g b>'.
		 */
		void setBackgroundColor(const Ogre::Vector4& backgroundColor);

		Ogre::Vector4 getBackgroundColor(void) const;

		/**
		 * @brief Specifies the RGB specular color. "kS" in most books about PBS
		 * @param[in] specularColor	The (r, g, b) specular color to set.
		 */
		void setSpecularColor(const Ogre::Vector3& specularColor);

		Ogre::Vector3 getSpecularColor(void) const;
		
		/**
		 * @brief Sets emissive color (e.g. a firefly).
		 * @param[in] emissiveColor	The (r, g, b) emissive color to set.
		 * @note Emissive color has no physical basis. Though in HDR, if you're working in lumens, this value should probably be in lumens too.
         * To disable emissive, setEmissive( Vector3::ZERO ) and unset any texture in PBSM_EMISSIVE slot.
		 */
		void setEmissiveColor(const Ogre::Vector3& emissiveColor);

		Ogre::Vector3 getEmissiveColor(void) const;

		/**
		 * @brief Sets the diffuse texture Name
		 * @param[in] diffuseTextureName	The texture name to set
		 */
		void setDiffuseTextureName(const Ogre::String& diffuseTextureName);

		const Ogre::String getDiffuseTextureName(void) const;

		/**
		 * @brief Sets the normal texture Name
		 * @param[in] normalTextureName	The normal texture name to set
		 */
		void setNormalTextureName(const Ogre::String& normalTextureName);

		const Ogre::String getNormalTextureName(void) const;

		/** 
		 * @brief Sets the normal mapping weight. The range doesn't necessarily have to be in [0; 1]
         * @remarks
         *    An exact value of 1 will generate a shader without the weighting math, while any
         *    other value will generate another shader that uses this weight (i.e. will
         *    cause a flushRenderables)
         * @param detailNormalMapIdx
         *    Value in the range [0; 4)
         * @param weight
         *    The weight for the normal map.
         *    A value of 0 means no effect (tangent space normal is 0, 0, 1); and would be
         *    the same as disabling the normal map texture.
         *    A value of 1 means full normal map effect.
         *    A value outside the [0; 1] range extrapolates.
         *    Default value is 1.
         */
		void setNormalMapWeight(Ogre::Real normalMapWeight);

		/** 
		 * @brief Returns the detail normal maps' weight.
		 * @return normalMapWeight	The normal map weight to get
		 */
		Ogre::Real getNormalMapWeight(void) const;

        /** 
		 * @brief Sets the strength of the of the clear coat layer and its roughness.
         * @param strength
         *  This should be treated as a binary value, set to either 0 or 1. Intermediate values are
         *  useful to control transitions between parts of the surface that have a clear coat layers and
         *  parts that don't.
         */
        void setClearCoat(Ogre::Real clearCoat);

		/** 
		 * @brief Returns the clear coat value.
		 * @return clearCoat	The clear coat value to get
		 */
		Ogre::Real getClearCoat(void) const;

		/** 
		 * @brief Sets the strength of the of the clear coat layer and its roughness.
         * @param strength
         *  This should be treated as a binary value, set to either 0 or 1. Intermediate values are
         *  useful to control transitions between parts of the surface that have a clear coat layers and
         *  parts that don't.
         */
        void setClearCoatRoughness(Ogre::Real clearCoatRoughness);
      
		/** 
		 * @brief Returns the clear coat roughness.
		 * @return clearCoatRoughness	The clear coat roughness to get
		 */
		Ogre::Real getClearCoatRoughness(void) const;

		/**
		 * @brief Sets the specular texture Name
		 * @param[in] specularTextureName	The specular texture name to set
		 */
		void setSpecularTextureName(const Ogre::String& specularTextureName);

		const Ogre::String getSpecularTextureName(void) const;

		/**
		 * @brief Sets the metallic texture Name
		 * @param[in] metallicTextureName	The metallic texture name to set
		 * @note The texture types PBSM_SPECULAR & PBSM_METALLIC map to the same value.
		 */
		void setMetallicTextureName(const Ogre::String& metallicTextureName);

		const Ogre::String getMetallicTextureName(void) const;

		/**
		 * @brief Sets the roughness texture Name
		 * @param[in] roughnessTextureName	The roughness texture name to set
		 * @note Only the Red channel will be used, and the texture will be converted to an efficient monochrome representation.
		 */
		void setRoughnessTextureName(const Ogre::String& roughnessTextureName);

		const Ogre::String getRoughnessTextureName(void) const;

		/**
		 * @brief Sets the detail weight texture Name
		 * @param[in] detailWeightTextureName	The detail weight texture name to set
		 * @note Texture that when present, will be used as mask/weight for the 4 detail maps. The R channel is used for detail map #0; the G for detail map #1, B for #2, and Alpha for #3.
         *        This affects both the diffuse and normal-mapped detail maps.
		 */
		void setDetailWeightTextureName(const Ogre::String& detailWeightTextureName);

		const Ogre::String getDetailWeightTextureName(void) const;

		/**
		 * @brief Sets the detail 0 texture Name
		 * @param[in] detail0TextureName	The detail 0 texture name to set
		 * @note Similar: detail_map1, detail_map2, detail_map3 Name of the detail map to be used on top of the diffuse color. There can be gaps (i.e. set detail maps 0 and 2 but not 1)
		 */
		void setDetail0TextureName(const Ogre::String& detail0TextureName);

		const Ogre::String getDetail0TextureName(void) const;

		/**
		 * @brief Sets the blend mode of a detail map.
		 * @param[in]	blendMode The blend mode to set as string
		 * 				The following values are possible:
		 * 					'PBSM_BLEND_NORMAL_NON_PREMUL'
		 * 					'PBSM_BLEND_NORMAL_PREMUL'
		 * 					'PBSM_BLEND_ADD'
		 * 					'PBSM_BLEND_SUBTRACT'
		 * 					'PBSM_BLEND_MULTIPLY'
		 * 					'PBSM_BLEND_MULTIPLY2X'
		 * 					'PBSM_BLEND_SCREEN'
		 * 					'PBSM_BLEND_OVERLAY'
		 * 					'PBSM_BLEND_LIGHTEN'
		 * 					'PBSM_BLEND_DARKEN'
		 * 					'PBSM_BLEND_GRAIN_EXTRACT'
		 * 					'PBSM_BLEND_GRAIN_MERGE'
		 * 					'PBSM_BLEND_DIFFERENCE'
		 */
		void setBlendMode0(const Ogre::String& blendMode0);

		Ogre::String getBlendMode0(void) const;

		/**
		 * @brief Sets the detail 1 texture Name
		 * @param[in] detail1TextureName	The detail 1 texture name to set
		 * @note Similar: detail_map1, detail_map2, detail_map3 Name of the detail map to be used on top of the diffuse color. There can be gaps (i.e. set detail maps 0 and 2 but not 1)
		 */
		void setDetail1TextureName(const Ogre::String& detail1TextureName);

		const Ogre::String getDetail1TextureName(void) const;

		void setBlendMode1(const Ogre::String& blendMode1);

		Ogre::String getBlendMode1(void) const;

		/**
		 * @brief Sets the detail 2 texture Name
		 * @param[in] detail2TextureName	The detail 2 texture name to set
		 * @note Similar: detail_map1, detail_map2, detail_map3 Name of the detail map to be used on top of the diffuse color. There can be gaps (i.e. set detail maps 0 and 2 but not 1)
		 */
		void setDetail2TextureName(const Ogre::String& detail2TextureName);

		const Ogre::String getDetail2TextureName(void) const;

		void setBlendMode2(const Ogre::String& blendMode2);

		Ogre::String getBlendMode2(void) const;

		void setDetail3TextureName(const Ogre::String& detail3TextureName);

		const Ogre::String getDetail3TextureName(void) const;

		void setBlendMode3(const Ogre::String& blendMode3);

		Ogre::String getBlendMode3(void) const;

		void setDetail0NMTextureName(const Ogre::String& detail0NMTextureName);

		const Ogre::String getDetail0NMTextureName(void) const;

		void setDetail1NMTextureName(const Ogre::String& detail1NMTextureName);

		const Ogre::String getDetail1NMTextureName(void) const;

		void setDetail2NMTextureName(const Ogre::String& detail2NMTextureName);

		const Ogre::String getDetail2NMTextureName(void) const;

		void setDetail3NMTextureName(const Ogre::String& detail3NMTextureName);

		const Ogre::String getDetail3NMTextureName(void) const;

		void setReflectionTextureName(const Ogre::String& reflectionTextureName);

		const Ogre::String getReflectionTextureName(void) const;

		void setEmissiveTextureName(const Ogre::String& emissiveTextureName);

		const Ogre::String getEmissiveTextureName(void) const;

		/**
		 * @brief Sets the transparency mode
		 * @param[in] transparencyMode	The transparency mode to set.
		 *								Possible values are:
		 * 								'Transparent'
		 * 								'Fade'
		 */
		void setTransparencyMode(const Ogre::String& transparencyMode);

		Ogre::String getTransparencyMode(void);

		/**
		 * @brief Makes the material transparent, and sets the amount of transparency
		 * @param[in] transparency Value in range [0; 1] where 0 = full transparency and 1 = fully opaque.
		 */
		void setTransparency(Ogre::Real transparency);

		Ogre::Real getTransparency(void) const;

		/**
		 * @brief Sets whether to use transparency from textures or not
		 * @param[in]	useAlphaFromTextures	When false, the alpha channel of the diffuse maps and detail maps will be ignored. It's a GPU performance optimization.
		 */
		void setUseAlphaFromTextures(bool useAlphaFromTextures);

		bool getUseAlphaFromTextures(void) const;

		 /** 
		  * @brief When set, it treats the emissive map as a lightmap; which means it will be multiplied against the diffuse component.
          * @remarks
            Note that HlmsPbsDatablock::setEmissive still applies,
            thus set it to 1 to avoid surprises.
          */
		void setUseEmissiveAsLightMap(bool useEmissiveAsLightMap);

		bool getUseEmissiveAsLightMap(void) const;

		void setUVScale(unsigned int uvScale);

		unsigned int getUVScale(void) const;

		void setShadowConstBias(Ogre::Real shadowConstBias);

		Ogre::Real getShadowConstBias(void) const;

		void setBringToFront(bool bringToFront);

		bool getIsInFront(void) const;

		void setCutOff(bool cutOff);

		bool getCutOff(void) const;

		Ogre::HlmsPbsDatablock* getDataBlock(void) const;

		void doNotCloneDataBlock(void);
	public:
		static const Ogre::String AttrSubEntityIndex(void) { return "Sub-Entity Index"; }
		static const Ogre::String AttrWorkflow(void) { return "Workflow"; }
		static const Ogre::String AttrMetalness(void) { return "Metalness"; }
		static const Ogre::String AttrRoughness(void) { return "Roughness"; }
		static const Ogre::String AttrFresnel(void) { return "Fresnel"; }
		static const Ogre::String AttrIndexOfRefraction(void) { return "Index Of Refraction"; }
		static const Ogre::String AttrRefractionStrength(void) { return "Refraction Strength"; }
		static const Ogre::String AttrBrdf(void) { return "Brdf"; }
		static const Ogre::String AttrTwoSidedLighting(void) { return "Two Sided Lighting"; }
		static const Ogre::String AttrOneSidedShadowCastCullingMode(void) { return "One Sided Shadow Cast Culling Mode"; }
		static const Ogre::String AttrAlphaTest(void) { return "Alpha Test"; }
		static const Ogre::String AttrAlphaTestThreshold(void) { return "Alpha Test Threshold"; }
		static const Ogre::String AttrReceiveShadows(void) { return "Receive Shadows"; }
		static const Ogre::String AttrDiffuseColor(void) { return "Diffuse Color"; }
		static const Ogre::String AttrBackgroundColor(void) { return "Background Color"; }
		static const Ogre::String AttrSpecularColor(void) { return "Specular Color"; }
		static const Ogre::String AttrEmissiveColor(void) { return "Emissive Color"; }
		static const Ogre::String AttrDiffuseTexture(void) { return "Diffuse Texture"; }
		static const Ogre::String AttrNormalTexture(void) { return "Normal Texture"; }
		static const Ogre::String AttrNormalMapWeight(void) { return "Normal Map Weight"; }
		static const Ogre::String AttrClearCoat(void) { return "Clear Coat"; }
		static const Ogre::String AttrClearCoatRoughness(void) { return "Clear Coat Roughness"; }
		static const Ogre::String AttrSpecularTexture(void) { return "Specular Texture"; }
		static const Ogre::String AttrMetallicTexture(void) { return "Metallic Texture"; }
		static const Ogre::String AttrRoughnessTexture(void) { return "Roughness Texture"; }
		static const Ogre::String AttrDetailWeightTexture(void) { return "Detail Weight Texture"; }
		static const Ogre::String AttrDetail0Texture(void) { return "Detail 0 Texture"; }
		static const Ogre::String AttrBlendMode0(void) { return "Blend Mode 0"; }
		static const Ogre::String AttrDetail1Texture(void) { return "Detail 1 Texture"; }
		static const Ogre::String AttrBlendMode1(void) { return "Blend Mode 1"; }
		static const Ogre::String AttrDetail2Texture(void) { return "Detail 2 Texture"; }
		static const Ogre::String AttrBlendMode2(void) { return "Blend Mode 2"; }
		static const Ogre::String AttrDetail3Texture(void) { return "Detail 3 Texture"; }
		static const Ogre::String AttrBlendMode3(void) { return "Blend Mode 3"; }
		static const Ogre::String AttrDetail0NMTexture(void) { return "Detail 0 NM Texture"; }
		static const Ogre::String AttrDetail1NMTexture(void) { return "Detail 1 NM Texture"; }
		static const Ogre::String AttrDetail2NMTexture(void) { return "Detail 2 NM Texture"; }
		static const Ogre::String AttrDetail3NMTexture(void) { return "Detail 3 NM Texture"; }
		static const Ogre::String AttrReflectionTexture(void) { return "Reflection Texture"; }
		static const Ogre::String AttrEmissiveTexture(void) { return "Emissive Texture"; }
		static const Ogre::String AttrTransparencyMode(void) { return "Transparency Mode"; }
		static const Ogre::String AttrTransparency(void) { return "Transparency"; }
		static const Ogre::String AttrUseAlphaFromTextures(void) { return "Use Alpha From Textures"; }
		static const Ogre::String AttrUVScale(void) { return "UV Scale"; }
		static const Ogre::String AttrUseEmissiveAsLightMap(void) { return "Use Emissive As LightMap"; }
		static const Ogre::String AttrShadowConstBias(void) { return "Shadow Const Bias"; }
		static const Ogre::String AttrBringToFront(void) { return "Bring To Front"; }
		static const Ogre::String AttrCutOff(void) { return "Cut Off"; }
	private:
		bool preReadDatablock(void);
		bool readDatablockEntity(Ogre::v1::Entity* entity);
		bool readDatablockItem(Ogre::Item* item);
		void postReadDatablock(void);
		Ogre::String getPbsTextureName(Ogre::HlmsPbsDatablock* datablock, Ogre::PbsTextureTypes type);
		void internalSetTextureName(Ogre::PbsTextureTypes pbsTextureType, Ogre::CommonTextureTypes::CommonTextureTypes textureType, Variant* attribute, const Ogre::String& textureName);

		Ogre::String mapWorkflowToString(Ogre::HlmsPbsDatablock::Workflows workflow);
		Ogre::HlmsPbsDatablock::Workflows mapStringToWorkflow(const Ogre::String& strWorkflow);

		Ogre::String mapTransparencyModeToString(Ogre::HlmsPbsDatablock::TransparencyModes mode);
		Ogre::HlmsPbsDatablock::TransparencyModes mapStringToTransparencyMode(const Ogre::String& strTransparencyMode);

		Ogre::String mapBrdfToString(Ogre::PbsBrdf::PbsBrdf brdf);
		Ogre::PbsBrdf::PbsBrdf mapStringToBrdf(const Ogre::String& strBrdf);

		Ogre::String mapBlendModeToString(Ogre::PbsBlendModes blendMode);
		Ogre::PbsBlendModes mapStringToBlendMode(const Ogre::String& strBlendMode);

		Ogre::String mapCullingModeToString(Ogre::CullingMode cullingMode);
		Ogre::CullingMode mapStringToCullingMode(const Ogre::String& strCullingMode);

		Ogre::String mapAlphaTestToString(Ogre::CompareFunction compareFunction);
		Ogre::CompareFunction mapStringToAlphaTest(const Ogre::String& strAlphaTest);

		void setReflectionTextureNames(void);
	protected:
		Variant* subEntityIndex;
		Variant* workflow; // List selection
		Variant* metalness; // Real
		Variant* roughness; // Real
		Variant* fresnel; // vector4 -> last is separate
		Variant* indexOfRefraction; // vector4 -> last is separate
		Variant* refractionStrength;
		Variant* brdf; // List selection
		Variant* twoSidedLighting; // bool
		Variant* oneSidedShadowCastCullingMode; // List selection
		Variant* alphaTest; // List selection
		Variant* alphaTestThreshold; // Real
		Variant* receiveShadows; // Bool
		Variant* diffuseColor;
		Variant* backgroundColor;
		Variant* specularColor;
		Variant* emissiveColor;
		Variant* diffuseTextureName;
		Variant* normalTextureName;
		Variant* normalMapWeight;
        Variant* clearCoat;
        Variant* clearCoatRoughness;
		Variant* specularTextureName;
		Variant* metallicTextureName;
		Variant* roughnessTextureName;
		Variant* detailWeightTextureName;
		Variant* detail0TextureName;
		Variant* blendMode0; // List selection
		Variant* detail1TextureName;
		Variant* blendMode1; // List selection
		Variant* detail2TextureName;
		Variant* blendMode2; // List selection
		Variant* detail3TextureName;
		Variant* blendMode3; // List selection
		Variant* detail0NMTextureName;
		Variant* detail1NMTextureName;
		Variant* detail2NMTextureName;
		Variant* detail3NMTextureName;
		Variant* reflectionTextureName;
		Variant* emissiveTextureName;
		Variant* transparencyMode;
		Variant* transparency;
		Variant* useAlphaFromTextures; // bool
		Variant* uvScale;
		Variant* useEmissiveAsLightMap;
		Variant* shadowConstBias;
		Variant* bringToFront;
		Variant* cutOff;

		Ogre::HlmsPbsDatablock* datablock;
		Ogre::HlmsPbsDatablock* originalDatablock;
		Ogre::HlmsMacroblock* originalMacroblock;
		Ogre::HlmsBlendblock* originalBlendblock;
		bool alreadyCloned;
		bool isCloned;
		bool newlyCreated;
		unsigned int oldSubIndex;
	};

}; //namespace end

#endif