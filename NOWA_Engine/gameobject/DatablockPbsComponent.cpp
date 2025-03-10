#include "NOWAPrecompiled.h"
#include "DatablockPbsComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "modules/DeployResourceModule.h"
#include "modules/WorkspaceModule.h"
#include "main/AppStateManager.h"
#include "WorkspaceComponents.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	DatablockPbsComponent::DatablockPbsComponent()
		: GameObjectComponent(),
		subEntityIndex(new Variant(DatablockPbsComponent::AttrSubEntityIndex(), static_cast<unsigned int>(0), this->attributes)),
		workflow(new Variant(DatablockPbsComponent::AttrWorkflow(), { "SpecularWorkflow", "SpecularAsFresnelWorkflow", "MetallicWorkflow" }, this->attributes)),
		metalness(new Variant(DatablockPbsComponent::AttrMetalness(), 0.818f, this->attributes)),
		roughness(new Variant(DatablockPbsComponent::AttrRoughness(), 1.0f, this->attributes)),
		fresnel(new Variant(DatablockPbsComponent::AttrFresnel(), Ogre::Vector4(0.01f, 0.01f, 0.01f, 0.0f), this->attributes)),
		indexOfRefraction(new Variant(DatablockPbsComponent::AttrIndexOfRefraction(), Ogre::Vector4(0.9f, 0.9f, 0.9f, 0.0f), this->attributes)),
		refractionStrength(new Variant(DatablockPbsComponent::AttrRefractionStrength(), 0.0f, this->attributes)),
		brdf(new Variant(DatablockPbsComponent::AttrBrdf(), { "Default", "CookTorrance", "BlinnPhong", "DefaultUncorrelated", "DefaultSeparateDiffuseFresnel", 
																	"CookTorranceSeparateDiffuseFresnel", "BlinnPhongSeparateDiffuseFresnel", "BlinnPhongLegacyMath", "BlinnPhongFullLegacy" }, this->attributes)),
		twoSidedLighting(new Variant(DatablockPbsComponent::AttrTwoSidedLighting(), false, this->attributes)),
		oneSidedShadowCastCullingMode(new Variant(DatablockPbsComponent::AttrOneSidedShadowCastCullingMode(), { "CULL_NONE", "CULL_CLOCKWISE", "CULL_ANTICLOCKWISE" }, this->attributes)),
		alphaTest(new Variant(DatablockPbsComponent::AttrAlphaTest(), { "CMPF_ALWAYS_FAIL", "CMPF_ALWAYS_PASS", "CMPF_LESS", "CMPF_LESS_EQUAL", "CMPF_EQUAL",
																		"CMPF_NOT_EQUAL", "CMPF_GREATER_EQUAL", "CMPF_GREATER", "NUM_COMPARE_FUNCTIONS", }, this->attributes)),
		alphaTestThreshold(new Variant(DatablockPbsComponent::AttrAlphaTestThreshold(), 1.0f, this->attributes)),
		receiveShadows(new Variant(DatablockPbsComponent::AttrReceiveShadows(), true, this->attributes)),
		diffuseColor(new Variant(DatablockPbsComponent::AttrDiffuseColor(), Ogre::Vector3(3.14f, 3.14f, 3.14f), this->attributes)),
		backgroundColor(new Variant(DatablockPbsComponent::AttrBackgroundColor(), Ogre::Vector4(1.0f, 1.0f, 1.0f, 1.0f), this->attributes)),
		specularColor(new Variant(DatablockPbsComponent::AttrSpecularColor(), Ogre::Vector3::UNIT_SCALE, this->attributes)),
		emissiveColor(new Variant(DatablockPbsComponent::AttrEmissiveColor(), Ogre::Vector3::ZERO, this->attributes)),
		diffuseTextureName(new Variant(DatablockPbsComponent::AttrDiffuseTexture(), Ogre::String(), this->attributes)),
		normalTextureName(new Variant(DatablockPbsComponent::AttrNormalTexture(), Ogre::String(), this->attributes)),
		normalMapWeight(new Variant(DatablockPbsComponent::AttrNormalMapWeight(), 1.0f, this->attributes)),
		clearCoat(new Variant(DatablockPbsComponent::AttrClearCoat(), 0.2f, this->attributes)),
		clearCoatRoughness(new Variant(DatablockPbsComponent::AttrClearCoatRoughness(), 0.2f, this->attributes)),
		specularTextureName(new Variant(DatablockPbsComponent::AttrSpecularTexture(), Ogre::String(), this->attributes)),
		metallicTextureName(new Variant(DatablockPbsComponent::AttrMetallicTexture(), Ogre::String(), this->attributes)),
		roughnessTextureName(new Variant(DatablockPbsComponent::AttrRoughnessTexture(), Ogre::String(), this->attributes)),
		detailWeightTextureName(new Variant(DatablockPbsComponent::AttrDetailWeightTexture(), Ogre::String(), this->attributes)),
		detail0TextureName(new Variant(DatablockPbsComponent::AttrDetail0Texture(), Ogre::String(), this->attributes)),
		blendMode0(new Variant(DatablockPbsComponent::AttrBlendMode0(), { "PBSM_BLEND_NORMAL_NON_PREMUL", "PBSM_BLEND_NORMAL_PREMUL", "PBSM_BLEND_ADD"
																												, "PBSM_BLEND_SUBTRACT", "PBSM_BLEND_MULTIPLY", "PBSM_BLEND_MULTIPLY2X"
																												, "PBSM_BLEND_SCREEN", "PBSM_BLEND_OVERLAY", "PBSM_BLEND_LIGHTEN", "PBSM_BLEND_DARKEN"
																												, "PBSM_BLEND_GRAIN_EXTRACT", "PBSM_BLEND_GRAIN_MERGE", "PBSM_BLEND_DIFFERENCE"}, this->attributes)),
		detail1TextureName(new Variant(DatablockPbsComponent::AttrDetail1Texture(), Ogre::String(), this->attributes)),
		blendMode1(new Variant(DatablockPbsComponent::AttrBlendMode1(), { "PBSM_BLEND_NORMAL_NON_PREMUL", "PBSM_BLEND_NORMAL_PREMUL", "PBSM_BLEND_ADD"
																												, "PBSM_BLEND_SUBTRACT", "PBSM_BLEND_MULTIPLY", "PBSM_BLEND_MULTIPLY2X"
																												, "PBSM_BLEND_SCREEN", "PBSM_BLEND_OVERLAY", "PBSM_BLEND_LIGHTEN", "PBSM_BLEND_DARKEN"
																												, "PBSM_BLEND_GRAIN_EXTRACT", "PBSM_BLEND_GRAIN_MERGE", "PBSM_BLEND_DIFFERENCE"}, this->attributes)),
		detail2TextureName(new Variant(DatablockPbsComponent::AttrDetail2Texture(), Ogre::String(), this->attributes)),
		blendMode2(new Variant(DatablockPbsComponent::AttrBlendMode2(), { "PBSM_BLEND_NORMAL_NON_PREMUL", "PBSM_BLEND_NORMAL_PREMUL", "PBSM_BLEND_ADD"
																												, "PBSM_BLEND_SUBTRACT", "PBSM_BLEND_MULTIPLY", "PBSM_BLEND_MULTIPLY2X"
																												, "PBSM_BLEND_SCREEN", "PBSM_BLEND_OVERLAY", "PBSM_BLEND_LIGHTEN", "PBSM_BLEND_DARKEN"
																												, "PBSM_BLEND_GRAIN_EXTRACT", "PBSM_BLEND_GRAIN_MERGE", "PBSM_BLEND_DIFFERENCE"}, this->attributes)),
		detail3TextureName(new Variant(DatablockPbsComponent::AttrDetail3Texture(), Ogre::String(), this->attributes)),
		blendMode3(new Variant(DatablockPbsComponent::AttrBlendMode3(), { "PBSM_BLEND_NORMAL_NON_PREMUL", "PBSM_BLEND_NORMAL_PREMUL", "PBSM_BLEND_ADD"
																												, "PBSM_BLEND_SUBTRACT", "PBSM_BLEND_MULTIPLY", "PBSM_BLEND_MULTIPLY2X"
																												, "PBSM_BLEND_SCREEN", "PBSM_BLEND_OVERLAY", "PBSM_BLEND_LIGHTEN", "PBSM_BLEND_DARKEN"
																												, "PBSM_BLEND_GRAIN_EXTRACT", "PBSM_BLEND_GRAIN_MERGE", "PBSM_BLEND_DIFFERENCE"}, this->attributes)),
		detail0NMTextureName(new Variant(DatablockPbsComponent::AttrDetail0NMTexture(), Ogre::String(), this->attributes)),
		detail1NMTextureName(new Variant(DatablockPbsComponent::AttrDetail1NMTexture(), Ogre::String(), this->attributes)),
		detail2NMTextureName(new Variant(DatablockPbsComponent::AttrDetail2NMTexture(), Ogre::String(), this->attributes)),
		detail3NMTextureName(new Variant(DatablockPbsComponent::AttrDetail3NMTexture(), Ogre::String(), this->attributes)),
		reflectionTextureName(new Variant(DatablockPbsComponent::AttrReflectionTexture(), std::vector<Ogre::String>(), this->attributes)),
		emissiveTextureName(new Variant(DatablockPbsComponent::AttrEmissiveTexture(), Ogre::String(), this->attributes)),
		transparencyMode(new Variant(DatablockPbsComponent::AttrTransparencyMode(), { "None", "Transparent", "Fade", "Refractive" }, this->attributes)),
		transparency(new Variant(DatablockPbsComponent::AttrTransparency(), 1.0f, this->attributes)),
		useAlphaFromTextures(new Variant(DatablockPbsComponent::AttrUseAlphaFromTextures(), true, this->attributes)),
		uvScale(new Variant(DatablockPbsComponent::AttrUVScale(), static_cast<unsigned int>(1), this->attributes)),
		useEmissiveAsLightMap(new Variant(DatablockPbsComponent::AttrUseEmissiveAsLightMap(), false, this->attributes)),
		shadowConstBias(new Variant(DatablockPbsComponent::AttrShadowConstBias(), 0.001f, this->attributes)),
		bringToFront(new Variant(DatablockPbsComponent::AttrBringToFront(), false, this->attributes)),
		cutOff(new Variant(DatablockPbsComponent::AttrCutOff(), false, this->attributes)),
		datablock(nullptr),
		originalDatablock(nullptr),
		alreadyCloned(false),
		isCloned(false),
		newlyCreated(true),
		oldSubIndex(0),
		originalMacroblock(nullptr),
		originalBlendblock(nullptr)
	{
		this->subEntityIndex->setDescription("Specifies the sub entity index, for which the datablock should be shown.");
		this->subEntityIndex->addUserData(GameObject::AttrActionNeedRefresh());
		this->workflow->addUserData(GameObject::AttrActionNeedRefresh());
		this->diffuseColor->addUserData(GameObject::AttrActionColorDialog());
		this->backgroundColor->addUserData(GameObject::AttrActionColorDialog());
		this->specularColor->addUserData(GameObject::AttrActionColorDialog());
		this->emissiveColor->addUserData(GameObject::AttrActionColorDialog());
		this->uvScale->setDescription("Scale texture and wrap, min: 1 max: 8");

		// Note: setUserData2 is already used for internal purposes and would cause a crash! Only setUserData must be used
		this->diffuseTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
		this->normalTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
		this->specularTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
		this->metallicTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
		this->roughnessTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
		this->detailWeightTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
		this->detail0TextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
		this->detail1TextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
		this->detail2TextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
		this->detail3TextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
		this->detail0NMTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
		this->detail1NMTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
		this->detail2NMTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
		this->detail3NMTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
		// this->reflectionTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
		this->emissiveTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");

		this->setReflectionTextureNames();

		this->refractionStrength->setConstraints(0.0f, 1.0f);
		this->normalMapWeight->setConstraints(0.0f, 2.0f);
		this->clearCoat->setConstraints(0.0f, 1.0f);
		this->clearCoatRoughness->setConstraints(0.0f, 1.0f);
		this->metalness->setConstraints(0.0f, 1.0f);
		this->roughness->setConstraints(0.0f, 8.0f);
		this->transparency->setConstraints(0.0f, 1.0f);

		this->brdf->setDescription("Note: clear coat can only be set for brdf 'Default'");
		this->brdf->addUserData(GameObject::AttrActionNeedRefresh());

		this->metalness->setDescription("If some areas are saturated with white spots, like a shine reflection. The following can be done for dimming: "
										"1) Directly through metalness: Lower this value to a minimum."
										"2) Indirectly through roughness: Increase this value to dim it");
		this->roughness->setDescription("If some areas are saturated with white spots, like a shine reflection. The following can be done for dimming: "
										"1) Directly through metalness: Lower this value to a minimum."
										"2) Indirectly through roughness: Increase this value to dim it");

		this->cutOff->setDescription("Cuts off this mesh from all other meshes which are touching this mesh.");
	}

	DatablockPbsComponent::~DatablockPbsComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DatablockPbsComponent] Destructor datablock pbs component for game object: " + this->gameObjectPtr->getName());

	}

	void DatablockPbsComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		// If a datablock has been cloned, it must be destroyed manually
		if (true == this->alreadyCloned)
		{
			Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
			if (nullptr != entity)
			{
				Ogre::String dataBlockName = *this->datablock->getNameStr();
				if (nullptr != this->originalDatablock)
				{
					Ogre::String originalDataBlockName = *this->originalDatablock->getNameStr();
					// Set back the default datablock
					entity->getSubEntity(this->oldSubIndex)->setDatablock(this->originalDatablock);
				}
			}
			else
			{
				Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
				if (nullptr != item)
				{
					// Set back the default datablock
					item->getSubItem(this->oldSubIndex)->setDatablock(this->originalDatablock);
				}
			}

			if (nullptr != this->datablock)
			{
				Ogre::String dataBlockName = *this->datablock->getNameStr();

				auto& linkedRenderabled = this->datablock->getLinkedRenderables();

				// Only destroy if the datablock is not used else where
				if (true == linkedRenderabled.empty())
				{
					this->datablock->getCreator()->destroyDatablock(this->datablock->getName());
					this->datablock = nullptr;
				}
			}
		}

		this->originalMacroblock = nullptr;
		this->originalBlendblock = nullptr;
	}

	bool DatablockPbsComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SubEntityIndex")
		{
			this->subEntityIndex->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Workflow")
		{
			this->setWorkflow(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Metalness")
		{
			this->setMetalness(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Roughness")
		{
			this->setRoughness(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Fresnel")
		{
			Ogre::Vector4 fresnelData = XMLConverter::getAttribVector4(propertyElement, "data");
			
			this->setFresnel(Ogre::Vector3(fresnelData.x, fresnelData.y, fresnelData.z), static_cast<bool>(fresnelData.w));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "IndexOfRefraction")
		{
			Ogre::Vector4 indexOfRefractionData = XMLConverter::getAttribVector4(propertyElement, "data");
			
			this->setIndexOfRefraction(Ogre::Vector3(indexOfRefractionData.x, indexOfRefractionData.y, indexOfRefractionData.z), static_cast<bool>(indexOfRefractionData.w));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RefractionStrength")
		{
			this->setRefractionStrength(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Brdf")
		{
			this->setBrdf(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TwoSidedLighting")
		{
			this->setTwoSidedLighting(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OneSidedShadowCastCullingMode")
		{
			this->setOneSidedShadowCastCullingMode(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AlphaTest")
		{
			this->setAlphaTest(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AlphaTestThreshold")
		{
			this->setAlphaTestThreshold(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ReceiveShadows")
		{
			this->setReceiveShadows(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DiffuseColor")
		{
			this->setDiffuseColor(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BackgroundColor")
		{
			this->setBackgroundColor(XMLConverter::getAttribVector4(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SpecularColor")
		{
			this->setSpecularColor(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EmissiveColor")
		{
			this->setEmissiveColor(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DiffuseTextureName")
		{
			this->setDiffuseTextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NormalTextureName")
		{
			this->setNormalTextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NormalMapWeight")
		{
			this->setNormalMapWeight(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ClearCoat")
		{
			this->setClearCoat(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ClearCoatRoughness")
		{
			this->setClearCoatRoughness(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SpecularTextureName")
		{
			this->setSpecularTextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MetallicTextureName")
		{
			this->setMetallicTextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RoughnessTextureName")
		{
			this->setRoughnessTextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DetailWeightTextureName")
		{
			this->setDetailWeightTextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Detail0TextureName")
		{
			this->setDetail0TextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BlendMode0")
		{
			this->setBlendMode0(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Detail1TextureName")
		{
			this->setDetail1TextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BlendMode1")
		{
			this->setBlendMode1(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Detail2TextureName")
		{
			this->setDetail2TextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BlendMode2")
		{
			this->setBlendMode2(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Detail3TextureName")
		{
			this->setDetail3TextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BlendMode3")
		{
			this->setBlendMode3(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Detail0NMTextureName")
		{
			this->setDetail0NMTextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Detail1NMTextureName")
		{
			this->setDetail1NMTextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Detail2NMTextureName")
		{
			this->setDetail2NMTextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Detail3NMTextureName")
		{
			this->setDetail3NMTextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ReflectionTextureName")
		{
			this->setReflectionTextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EmissiveTextureName")
		{
			this->setEmissiveTextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TransparencyMode")
		{
			this->setTransparencyMode(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Transparency")
		{
			this->setTransparency(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseAlphaFromTextures")
		{
			this->setUseAlphaFromTextures(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UVScale")
		{
			this->setUVScale(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseEmissiveAsLightMap")
		{
			this->setUseEmissiveAsLightMap(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShadowConstBias")
		{
			this->setShadowConstBias(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BringToFront")
		{
			this->setBringToFront(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CutOff")
		{
			this->setCutOff(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		this->newlyCreated = false;

		return true;
	}

	GameObjectCompPtr DatablockPbsComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		DatablockPbsCompPtr clonedCompPtr(boost::make_shared<DatablockPbsComponent>());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);
		
		clonedCompPtr->setSubEntityIndex(this->subEntityIndex->getUInt());
		clonedCompPtr->setWorkflow(this->workflow->getListSelectedValue());
		clonedCompPtr->setMetalness(this->metalness->getReal());
		clonedCompPtr->setRoughness(this->roughness->getReal());
		clonedCompPtr->setFresnel(Ogre::Vector3(this->fresnel->getVector4().x, this->fresnel->getVector4().y, this->fresnel->getVector4().z), static_cast<bool>(this->fresnel->getVector4().w));
		clonedCompPtr->setIndexOfRefraction(Ogre::Vector3(this->indexOfRefraction->getVector4().x, this->indexOfRefraction->getVector4().y, this->indexOfRefraction->getVector4().z), static_cast<bool>(this->indexOfRefraction->getVector4().w));
		clonedCompPtr->setRefractionStrength(this->refractionStrength->getReal());
		clonedCompPtr->setBrdf(this->brdf->getListSelectedValue());
		clonedCompPtr->setTwoSidedLighting(this->twoSidedLighting->getBool());
		clonedCompPtr->setOneSidedShadowCastCullingMode(this->oneSidedShadowCastCullingMode->getListSelectedValue());
		clonedCompPtr->setAlphaTest(this->alphaTest->getListSelectedValue());
		clonedCompPtr->setAlphaTestThreshold(this->alphaTestThreshold->getReal());
		clonedCompPtr->setReceiveShadows(this->receiveShadows->getBool());
		clonedCompPtr->setDiffuseColor(this->diffuseColor->getVector3());
		clonedCompPtr->setBackgroundColor(this->backgroundColor->getVector4());
		clonedCompPtr->setSpecularColor(this->specularColor->getVector3());
		clonedCompPtr->setEmissiveColor(this->emissiveColor->getVector3());
		
		clonedCompPtr->setDiffuseTextureName(this->diffuseTextureName->getString());
		clonedCompPtr->setNormalTextureName(this->normalTextureName->getString());
		clonedCompPtr->setNormalMapWeight(this->normalMapWeight->getReal());
		clonedCompPtr->setClearCoat(this->clearCoat->getReal());
		clonedCompPtr->setClearCoatRoughness(this->clearCoatRoughness->getReal());
		clonedCompPtr->setSpecularTextureName(this->specularTextureName->getString());
		clonedCompPtr->setMetallicTextureName(this->metallicTextureName->getString());
		clonedCompPtr->setRoughnessTextureName(this->roughnessTextureName->getString());
		clonedCompPtr->setDetailWeightTextureName(this->detailWeightTextureName->getString());
		clonedCompPtr->setDetail0TextureName(this->detail0TextureName->getString());
		clonedCompPtr->setBlendMode0(this->blendMode0->getListSelectedValue());
		clonedCompPtr->setDetail1TextureName(this->detail1TextureName->getString());
		clonedCompPtr->setBlendMode1(this->blendMode1->getListSelectedValue());
		clonedCompPtr->setDetail2TextureName(this->detail2TextureName->getString());
		clonedCompPtr->setBlendMode2(this->blendMode2->getListSelectedValue());
		clonedCompPtr->setDetail3TextureName(this->detail3TextureName->getString());
		clonedCompPtr->setBlendMode3(this->blendMode3->getListSelectedValue());
		clonedCompPtr->setDetail0NMTextureName(this->detail0NMTextureName->getString());
		clonedCompPtr->setDetail1NMTextureName(this->detail1NMTextureName->getString());
		clonedCompPtr->setDetail2NMTextureName(this->detail2NMTextureName->getString());
		clonedCompPtr->setDetail3NMTextureName(this->detail3NMTextureName->getString());
		clonedCompPtr->setReflectionTextureName(this->reflectionTextureName->getListSelectedValue());
		clonedCompPtr->setEmissiveTextureName(this->emissiveTextureName->getString());

		clonedCompPtr->setTransparencyMode(this->transparencyMode->getListSelectedValue());
		clonedCompPtr->setTransparency(this->transparency->getReal());
		clonedCompPtr->setUseAlphaFromTextures(this->useAlphaFromTextures->getBool());
		clonedCompPtr->setUVScale(this->uvScale->getUInt());
		clonedCompPtr->setUseEmissiveAsLightMap(this->useEmissiveAsLightMap->getBool());
		clonedCompPtr->setShadowConstBias(this->shadowConstBias->getReal());
		clonedCompPtr->setBringToFront(this->bringToFront->getBool());
		clonedCompPtr->setCutOff(this->cutOff->getBool());

		clonedCompPtr->isCloned = true;
		
		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool DatablockPbsComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DatablockPbsComponent] Init datablock pbs component for game object: " + this->gameObjectPtr->getName());

		return this->preReadDatablock();
	}

	Ogre::String DatablockPbsComponent::getPbsTextureName(Ogre::HlmsPbsDatablock* datablock, Ogre::PbsTextureTypes type)
	{
		Ogre::TextureGpu* texture = datablock->getTexture(type);
		if (nullptr != texture)
		{
			// Attention: Here get name, or getNameStr?
			return texture->getNameStr();
		}
		return "";
	}
	
	Ogre::String DatablockPbsComponent::mapWorkflowToString(Ogre::HlmsPbsDatablock::Workflows workflow)
	{
		Ogre::String strWorkflow = "SpecularWorkflow";
		if (Ogre::HlmsPbsDatablock::SpecularAsFresnelWorkflow == workflow)
			strWorkflow = "SpecularAsFresnelWorkflow";
		else if (Ogre::HlmsPbsDatablock::MetallicWorkflow == workflow)
			strWorkflow = "MetallicWorkflow";
		
		return strWorkflow;
	}
	
	Ogre::HlmsPbsDatablock::Workflows DatablockPbsComponent::mapStringToWorkflow(const Ogre::String& strWorkflow)
	{
		Ogre::HlmsPbsDatablock::Workflows workflow = Ogre::HlmsPbsDatablock::SpecularWorkflow;
		if ("SpecularAsFresnelWorkflow" == strWorkflow)
			workflow = Ogre::HlmsPbsDatablock::SpecularAsFresnelWorkflow;
		else if ("MetallicWorkflow" == strWorkflow)
			workflow = Ogre::HlmsPbsDatablock::MetallicWorkflow;

		return workflow;
	}
	
	Ogre::String DatablockPbsComponent::mapTransparencyModeToString(Ogre::HlmsPbsDatablock::TransparencyModes mode)
	{
		Ogre::String strMode = "None";
		if (Ogre::HlmsPbsDatablock::Transparent == mode)
			strMode = "Transparent";
		else if (Ogre::HlmsPbsDatablock::Fade == mode)
			strMode = "Fade";
		
		return strMode;
	}

	Ogre::HlmsPbsDatablock::TransparencyModes DatablockPbsComponent::mapStringToTransparencyMode(const Ogre::String& strTransparencyMode)
	{
		Ogre::HlmsPbsDatablock::TransparencyModes mode = Ogre::HlmsPbsDatablock::None;
		if ("Transparent" == strTransparencyMode)
			mode = Ogre::HlmsPbsDatablock::Transparent;
		else if ("Fade" == strTransparencyMode)
			mode = Ogre::HlmsPbsDatablock::Fade;

		return mode;
	}
	
	Ogre::String DatablockPbsComponent::mapBrdfToString(Ogre::PbsBrdf::PbsBrdf brdf)
	{
		Ogre::String strBrdf = "Default";
		if (Ogre::PbsBrdf::CookTorrance == brdf)
			strBrdf = "CookTorrance";
		else if (Ogre::PbsBrdf::BlinnPhong == brdf)
			strBrdf = "BlinnPhong";
		else if (Ogre::PbsBrdf::DefaultUncorrelated == brdf)
			strBrdf = "DefaultUncorrelated";
		else if (Ogre::PbsBrdf::DefaultSeparateDiffuseFresnel == brdf)
			strBrdf = "DefaultSeparateDiffuseFresnel";
		else if (Ogre::PbsBrdf::CookTorranceSeparateDiffuseFresnel == brdf)
			strBrdf = "CookTorranceSeparateDiffuseFresnel";
		else if (Ogre::PbsBrdf::BlinnPhongSeparateDiffuseFresnel == brdf)
			strBrdf = "BlinnPhongSeparateDiffuseFresnel";
		else if (Ogre::PbsBrdf::BlinnPhongLegacyMath == brdf)
			strBrdf = "BlinnPhongLegacyMath";
		else if (Ogre::PbsBrdf::BlinnPhongFullLegacy == brdf)
			strBrdf = "BlinnPhongFullLegacy";
		
		return strBrdf;
	}
	
	Ogre::PbsBrdf::PbsBrdf DatablockPbsComponent::mapStringToBrdf(const Ogre::String& strBrdf)
	{
		uint32_t brdf = Ogre::PbsBrdf::Default;
		if ("CookTorrance" == strBrdf)
			brdf = Ogre::PbsBrdf::CookTorrance;
		else if ("BlinnPhong" == strBrdf)
			brdf = Ogre::PbsBrdf::BlinnPhong;
		else if ("DefaultUncorrelated" == strBrdf)
			brdf = Ogre::PbsBrdf::DefaultUncorrelated;
		else if ("DefaultSeparateDiffuseFresnel" == strBrdf)
		{
			brdf = Ogre::PbsBrdf::DefaultSeparateDiffuseFresnel;
			brdf |= Ogre::PbsBrdf::FLAG_HAS_DIFFUSE_FRESNEL |
				Ogre::PbsBrdf::FLAG_SPERATE_DIFFUSE_FRESNEL;
		}
		else if ("CookTorranceSeparateDiffuseFresnel" == strBrdf)
		{
			brdf = Ogre::PbsBrdf::CookTorranceSeparateDiffuseFresnel;
			brdf |= Ogre::PbsBrdf::FLAG_HAS_DIFFUSE_FRESNEL |
				Ogre::PbsBrdf::FLAG_SPERATE_DIFFUSE_FRESNEL;
		}
		else if ("BlinnPhongSeparateDiffuseFresnel" == strBrdf)
		{
			brdf = Ogre::PbsBrdf::BlinnPhongSeparateDiffuseFresnel;
			brdf |= Ogre::PbsBrdf::FLAG_HAS_DIFFUSE_FRESNEL |
				Ogre::PbsBrdf::FLAG_SPERATE_DIFFUSE_FRESNEL;
		}
		else if ("BlinnPhongLegacyMath" == strBrdf)
			brdf = Ogre::PbsBrdf::BlinnPhongLegacyMath;
		else if ("BlinnPhongFullLegacy" == strBrdf)
			brdf = Ogre::PbsBrdf::BlinnPhongFullLegacy;

		return static_cast<Ogre::PbsBrdf::PbsBrdf>(brdf);
	}
	
	Ogre::String DatablockPbsComponent::mapBlendModeToString(Ogre::PbsBlendModes blendMode)
	{
		Ogre::String strBlendMode = "PBSM_BLEND_NORMAL_NON_PREMUL";
		if (Ogre::PbsBlendModes::PBSM_BLEND_NORMAL_PREMUL == blendMode)
			strBlendMode = "PBSM_BLEND_NORMAL_PREMUL";
		else if (Ogre::PbsBlendModes::PBSM_BLEND_ADD == blendMode)
			strBlendMode = "PBSM_BLEND_ADD";
		else if (Ogre::PbsBlendModes::PBSM_BLEND_SUBTRACT == blendMode)
			strBlendMode = "PBSM_BLEND_SUBTRACT";
		else if (Ogre::PbsBlendModes::PBSM_BLEND_MULTIPLY == blendMode)
			strBlendMode = "PBSM_BLEND_MULTIPLY";
		else if (Ogre::PbsBlendModes::PBSM_BLEND_MULTIPLY2X == blendMode)
			strBlendMode = "PBSM_BLEND_MULTIPLY2X";
		else if (Ogre::PbsBlendModes::PBSM_BLEND_SCREEN == blendMode)
			strBlendMode = "PBSM_BLEND_SCREEN";
		else if (Ogre::PbsBlendModes::PBSM_BLEND_OVERLAY == blendMode)
			strBlendMode = "PBSM_BLEND_OVERLAY";
		else if (Ogre::PbsBlendModes::PBSM_BLEND_LIGHTEN == blendMode)
			strBlendMode = "PBSM_BLEND_LIGHTEN";
		else if (Ogre::PbsBlendModes::PBSM_BLEND_DARKEN == blendMode)
			strBlendMode = "PBSM_BLEND_DARKEN";
		else if (Ogre::PbsBlendModes::PBSM_BLEND_GRAIN_EXTRACT == blendMode)
			strBlendMode = "PBSM_BLEND_GRAIN_EXTRACT";
		else if (Ogre::PbsBlendModes::PBSM_BLEND_GRAIN_MERGE == blendMode)
			strBlendMode = "PBSM_BLEND_GRAIN_MERGE";
		else if (Ogre::PbsBlendModes::PBSM_BLEND_DIFFERENCE == blendMode)
			strBlendMode = "PBSM_BLEND_DIFFERENCE";
		
		return strBlendMode;
	}
	
	Ogre::PbsBlendModes DatablockPbsComponent::mapStringToBlendMode(const Ogre::String& strBlendMode)
	{
		Ogre::PbsBlendModes blendMode = Ogre::PbsBlendModes::PBSM_BLEND_NORMAL_NON_PREMUL;
		if ("PBSM_BLEND_NORMAL_PREMUL" == strBlendMode)
			blendMode = Ogre::PbsBlendModes::PBSM_BLEND_NORMAL_PREMUL;
		else if ("PBSM_BLEND_ADD" == strBlendMode)
			blendMode = Ogre::PbsBlendModes::PBSM_BLEND_ADD;
		else if ("PBSM_BLEND_SUBTRACT" == strBlendMode)
			blendMode = Ogre::PbsBlendModes::PBSM_BLEND_SUBTRACT;
		else if ("PBSM_BLEND_MULTIPLY" == strBlendMode)
			blendMode = Ogre::PbsBlendModes::PBSM_BLEND_MULTIPLY;
		else if ("PBSM_BLEND_MULTIPLY2X" == strBlendMode)
			blendMode = Ogre::PbsBlendModes::PBSM_BLEND_MULTIPLY2X;
		else if ("PBSM_BLEND_SCREEN" == strBlendMode)
			blendMode = Ogre::PbsBlendModes::PBSM_BLEND_SCREEN;
		else if ("PBSM_BLEND_OVERLAY" == strBlendMode)
			blendMode = Ogre::PbsBlendModes::PBSM_BLEND_OVERLAY;
		else if ("PBSM_BLEND_LIGHTEN" == strBlendMode)
			blendMode = Ogre::PbsBlendModes::PBSM_BLEND_LIGHTEN;
		else if ("PBSM_BLEND_DARKEN" == strBlendMode)
			blendMode = Ogre::PbsBlendModes::PBSM_BLEND_DARKEN;
		else if ("PBSM_BLEND_GRAIN_EXTRACT" == strBlendMode)
			blendMode = Ogre::PbsBlendModes::PBSM_BLEND_GRAIN_EXTRACT;
		else if ("PBSM_BLEND_GRAIN_MERGE" == strBlendMode)
			blendMode = Ogre::PbsBlendModes::PBSM_BLEND_GRAIN_MERGE;
		else if ("PBSM_BLEND_DIFFERENCE" == strBlendMode)
			blendMode = Ogre::PbsBlendModes::PBSM_BLEND_DIFFERENCE;

		return blendMode;
	}
	
	Ogre::String DatablockPbsComponent::mapCullingModeToString(Ogre::CullingMode cullingMode)
	{
		Ogre::String strCullingMode = "CULL_NONE";
		if (Ogre::CULL_CLOCKWISE == cullingMode)
			strCullingMode = "CULL_CLOCKWISE";
		else if (Ogre::CULL_ANTICLOCKWISE == cullingMode)
			strCullingMode = "CULL_ANTICLOCKWISE";
		
		return strCullingMode;
	}
	
	Ogre::CullingMode DatablockPbsComponent::mapStringToCullingMode(const Ogre::String& strCullingMode)
	{
		Ogre::CullingMode cullingMode = Ogre::CULL_NONE;
		if ("CULL_CLOCKWISE" == strCullingMode)
			cullingMode = Ogre::CULL_CLOCKWISE;
		else if ("CULL_ANTICLOCKWISE" == strCullingMode)
			cullingMode = Ogre::CULL_ANTICLOCKWISE;

		return cullingMode;
	}

	Ogre::String DatablockPbsComponent::mapAlphaTestToString(Ogre::CompareFunction compareFunction)
	{
		Ogre::String strAlphaTest = "CMPF_ALWAYS_PASS";
		if (Ogre::CMPF_ALWAYS_FAIL == compareFunction)
			strAlphaTest = "CMPF_ALWAYS_FAIL";
		else if (Ogre::CMPF_LESS == compareFunction)
			strAlphaTest = "CMPF_LESS";
		else if (Ogre::CMPF_LESS_EQUAL == compareFunction)
			strAlphaTest = "CMPF_LESS_EQUAL";
		else if (Ogre::CMPF_EQUAL == compareFunction)
			strAlphaTest = "CMPF_EQUAL";
		else if (Ogre::CMPF_NOT_EQUAL == compareFunction)
			strAlphaTest = "CMPF_NOT_EQUAL";
		else if (Ogre::CMPF_GREATER_EQUAL == compareFunction)
			strAlphaTest = "CMPF_GREATER_EQUAL";
		else if (Ogre::CMPF_GREATER == compareFunction)
			strAlphaTest = "CMPF_GREATER";
		
		return strAlphaTest;
	}

	Ogre::CompareFunction DatablockPbsComponent::mapStringToAlphaTest(const Ogre::String& strAlphaTest)
	{
		Ogre::CompareFunction alphaTest = Ogre::CMPF_ALWAYS_PASS;
		if ("CMPF_ALWAYS_FAIL" == strAlphaTest)
			alphaTest = Ogre::CMPF_ALWAYS_FAIL;
		else if ("CMPF_LESS" == strAlphaTest)
			alphaTest = Ogre::CMPF_LESS;
		else if ("CMPF_LESS_EQUAL" == strAlphaTest)
			alphaTest = Ogre::CMPF_LESS_EQUAL;
		else if ("CMPF_EQUAL" == strAlphaTest)
			alphaTest = Ogre::CMPF_EQUAL;
		else if ("CMPF_NOT_EQUAL" == strAlphaTest)
			alphaTest = Ogre::CMPF_NOT_EQUAL;
		else if ("CMPF_GREATER_EQUAL" == strAlphaTest)
			alphaTest = Ogre::CMPF_GREATER_EQUAL;
		else if ("CMPF_GREATER" == strAlphaTest)
			alphaTest = Ogre::CMPF_GREATER;

		return alphaTest;
	}

	void DatablockPbsComponent::setReflectionTextureNames(void)
	{
		Ogre::StringVectorPtr skyNames = Ogre::ResourceGroupManager::getSingleton().findResourceNames("Skies", "*.dds");
		std::vector<Ogre::String> compatibleSkyNames(skyNames.getPointer()->size() + 1);
		unsigned int i = 1;
		compatibleSkyNames[0] = "";
		for (auto& it = skyNames.getPointer()->cbegin(); it != skyNames.getPointer()->cend(); it++)
		{
			compatibleSkyNames[i++] = *it;
		}
		this->reflectionTextureName->getList().clear();
		this->reflectionTextureName->setValue(compatibleSkyNames);
		this->reflectionTextureName->setListSelectedValue("");
	}
	
	bool DatablockPbsComponent::preReadDatablock(void)
	{
		bool success = true;

		if (nullptr == this->gameObjectPtr)
		{
			return false;
		}

		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity)
		{
			success = this->readDatablockEntity(entity);
		}
		else
		{
			Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
			if (nullptr != item)
			{
				success = this->readDatablockItem(item);
			}
		}

		if (false == success)
		{
			this->datablock = nullptr;
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DatablockPbsComponent] Datablock reading failed for game object: " + this->gameObjectPtr->getName());
		}

		// Refresh reflection is used
		if (true == this->gameObjectPtr->getUseReflection())
		{
			this->gameObjectPtr->setUseReflection(false);
			this->gameObjectPtr->setUseReflection(true);
		}

		return success;
	}

	bool DatablockPbsComponent::readDatablockEntity(Ogre::v1::Entity* entity)
	{
		// Two data block components with the same entity index can not exist
		for (unsigned int i = 0; i < static_cast<unsigned int>(this->gameObjectPtr->getComponents()->size()); i++)
		{
			auto& priorPbsComponent = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<DatablockPbsComponent>(DatablockPbsComponent::getStaticClassName(), i));
			if (nullptr != priorPbsComponent && priorPbsComponent.get() != this)
			{
				if (this->subEntityIndex->getUInt() == priorPbsComponent->getSubEntityIndex())
				{
					this->subEntityIndex->setValue(priorPbsComponent->getSubEntityIndex() + 1);
				}
			}
		}
		
		if (this->subEntityIndex->getUInt() >= static_cast<unsigned int>(entity->getNumSubEntities()))
		{
			this->datablock = nullptr;
			Ogre::String message = "[DatablockPbsComponent] Datablock reading failed, because there is no such sub entity index: "
				+ Ogre::StringConverter::toString(this->subEntityIndex->getUInt()) + " for game object: "
				+ this->gameObjectPtr->getName();
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, message);

			boost::shared_ptr<EventDataFeedback> eventDataFeedback(new EventDataFeedback(false, message));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataFeedback);
			return false;
		}

		// If a prior component set this custom data string, with this content, do not clone the datablock (see. PlaneComponent)
		if ("doNotCloneDatablock" == this->gameObjectPtr->getCustomDataString())
		{
			this->alreadyCloned = true;
			this->gameObjectPtr->setCustomDataString("");
		}

		// Get the cloned data block, so that it can be manipulated individually
		this->originalDatablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(entity->getSubEntity(this->subEntityIndex->getUInt())->getDatablock());
		// Datablock could not be received, pbs entity got unlit data block component?
		if (nullptr == this->originalDatablock)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DatablockUnlitComponent] Warning: Could not use datablock pbs component, because this game object has a unlit data block for game object: "
															+ this->gameObjectPtr->getName());
			return false;
		}

		// Store also the original name
		Ogre::String originalDataBlockName = *this->originalDatablock->getNameStr();

		if ("Missing" == originalDataBlockName)
			return false;
		
		// If its already a cloned data block (with __ + id extension), this one will be then adapted by properties from this component
		size_t found = originalDataBlockName.find("__");
		if (found != Ogre::String::npos)
		{
			originalDataBlockName = originalDataBlockName.substr(0, found);
		}

		if (false == this->alreadyCloned && false == this->isCloned && nullptr != this->originalDatablock)
		{
			bool alreadyExists = false;
			unsigned char index = 0;
			do
			{
				try
				{
					alreadyExists = false;
					this->datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(this->originalDatablock->clone(originalDataBlockName + "__"
																			+ Ogre::StringConverter::toString(this->gameObjectPtr->getId())));
				}
				catch (const Ogre::Exception& exception)
				{
					alreadyExists = true;
					// Already exists, try again:
					this->datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(this->originalDatablock->clone(originalDataBlockName + "__" + Ogre::StringConverter::toString(index++)
																			+ Ogre::StringConverter::toString(this->gameObjectPtr->getId())));
				}
			} while (false == alreadyExists);

			this->alreadyCloned = true;
		}
		else
		{
			this->datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(entity->getSubEntity(this->subEntityIndex->getUInt())->getDatablock());
		}

		if (nullptr == this->datablock)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DatablockPbsComponent] Datablock reading failed, because there is no data block for game object: "
				+ this->gameObjectPtr->getName());
			return false;
		}

		entity->getSubEntity(this->subEntityIndex->getUInt())->setDatablock(this->datablock);
		this->oldSubIndex = this->subEntityIndex->getUInt();

		this->postReadDatablock();

		return true;
	}

	bool DatablockPbsComponent::readDatablockItem(Ogre::Item* item)
	{
		// Two data block components with the same entity index can not exist
		for (unsigned int i = 0; i < static_cast<unsigned int>(this->gameObjectPtr->getComponents()->size()); i++)
		{
			auto& priorPbsComponent = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<DatablockPbsComponent>(DatablockPbsComponent::getStaticClassName(), i));
			if (nullptr != priorPbsComponent && priorPbsComponent.get() != this)
			{
				if (this->subEntityIndex->getUInt() == priorPbsComponent->getSubEntityIndex())
				{
					this->subEntityIndex->setValue(priorPbsComponent->getSubEntityIndex() + 1);
				}
			}
		}

		// If a prior component set this custom data string, with this content, do not clone the datablock (see. PlaneComponent)
		if ("doNotCloneDatablock" == this->gameObjectPtr->getCustomDataString())
		{
			this->alreadyCloned = true;
			this->gameObjectPtr->setCustomDataString("");
		}

		// Get the cloned data block, so that it can be manipulated individually
		this->originalDatablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(item->getSubItem(this->subEntityIndex->getUInt())->getDatablock());

		// Store also the original name
		Ogre::String originalDataBlockName = *this->originalDatablock->getNameStr();

		/*if ("Missing" == originalDataBlockName)
			return false;*/

		// If its already a cloned data block (with __ + id extension), this one will be then adapted by properties from this component
		size_t found = originalDataBlockName.find("__");
		if (found != Ogre::String::npos)
		{
			originalDataBlockName = originalDataBlockName.substr(0, found);
		}

		if (false == this->alreadyCloned && false == this->isCloned && nullptr != this->originalDatablock)
		{
			bool alreadyExists = false;
			unsigned char index = 0;
			do
			{
				try
				{
					alreadyExists = false;
					this->datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(this->originalDatablock->clone(originalDataBlockName + "__"
																			+ Ogre::StringConverter::toString(this->gameObjectPtr->getId())));
				}
				catch (const Ogre::Exception& exception)
				{
					alreadyExists = true;
					// Already exists, try again:
					this->datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(this->originalDatablock->clone(originalDataBlockName + "__" + Ogre::StringConverter::toString(index++)
																			+ Ogre::StringConverter::toString(this->gameObjectPtr->getId())));
				}
			} while (false == alreadyExists);

			this->alreadyCloned = true;
		}
		else
		{
			this->datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(item->getSubItem(this->subEntityIndex->getUInt())->getDatablock());
		}

		if (nullptr == this->datablock)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DatablockPbsComponent] Datablock reading failed, because there is no data block for game object: "
															+ this->gameObjectPtr->getName());
			return false;
		}

		item->getSubItem(this->subEntityIndex->getUInt())->setDatablock(this->datablock);
		this->oldSubIndex = this->subEntityIndex->getUInt();

		this->postReadDatablock();

		return true;
	}

	void DatablockPbsComponent::postReadDatablock(void)
	{
		/*
		 enum PbsTextureTypes
			{
				PBSM_DIFFUSE,
				PBSM_NORMAL,
				PBSM_SPECULAR,
				PBSM_METALLIC = PBSM_SPECULAR,
				PBSM_ROUGHNESS,
				PBSM_DETAIL_WEIGHT,
				PBSM_DETAIL0,
				PBSM_DETAIL1,
				PBSM_DETAIL2,
				PBSM_DETAIL3,
				PBSM_DETAIL0_NM,
				PBSM_DETAIL1_NM,
				PBSM_DETAIL2_NM,
				PBSM_DETAIL3_NM,
				PBSM_EMISSIVE,
				PBSM_REFLECTION,
				NUM_PBSM_SOURCES = PBSM_REFLECTION,
				NUM_PBSM_TEXTURE_TYPES
			};
		*/
		
		this->setWorkflow(this->workflow->getListSelectedValue());

		this->setMetalness(this->metalness->getReal());

		this->setRoughness(this->roughness->getReal());

		this->setFresnel(Ogre::Vector3(this->fresnel->getVector4().x, this->fresnel->getVector4().y, this->fresnel->getVector4().z), static_cast<bool>(this->fresnel->getVector4().w));

		this->setIndexOfRefraction(Ogre::Vector3(this->indexOfRefraction->getVector4().x, this->indexOfRefraction->getVector4().y, this->indexOfRefraction->getVector4().z), static_cast<bool>(this->indexOfRefraction->getVector4().w));

		this->setRefractionStrength(this->refractionStrength->getReal());

		this->setBrdf(this->brdf->getListSelectedValue());

		this->setTwoSidedLighting(this->twoSidedLighting->getBool());

		this->setOneSidedShadowCastCullingMode(this->oneSidedShadowCastCullingMode->getListSelectedValue());

		this->setAlphaTest(this->alphaTest->getListSelectedValue());

		this->setAlphaTestThreshold(this->alphaTestThreshold->getReal());

		this->setReceiveShadows(this->receiveShadows->getBool());

		this->setDiffuseColor(this->diffuseColor->getVector3());

		this->backgroundColor->setValue(Ogre::Vector4(this->datablock->getBackgroundDiffuse().r, this->datablock->getBackgroundDiffuse().g,
			this->datablock->getBackgroundDiffuse().b, this->datablock->getBackgroundDiffuse().a));

		this->setSpecularColor(this->specularColor->getVector3());
		
		this->setEmissiveColor(this->emissiveColor->getVector3());

		this->setDiffuseTextureName(this->diffuseTextureName->getString());

		this->setNormalTextureName(this->normalTextureName->getString());

		this->setNormalMapWeight(this->normalMapWeight->getReal());

		this->setClearCoat(this->clearCoat->getReal());

		this->setClearCoatRoughness(this->clearCoatRoughness->getReal());

		this->setSpecularTextureName(this->specularTextureName->getString());

		this->setMetallicTextureName(this->metallicTextureName->getString());

		this->setRoughnessTextureName(this->roughnessTextureName->getString());

		this->setDetailWeightTextureName(this->detailWeightTextureName->getString());

		this->setDetail0TextureName(this->detail0TextureName->getString());

		this->setBlendMode0(this->blendMode0->getListSelectedValue());

		this->setDetail1TextureName(this->detail1TextureName->getString());

		this->setBlendMode1(this->blendMode1->getListSelectedValue());

		this->setDetail2TextureName(this->detail2TextureName->getString());

		this->setBlendMode2(this->blendMode2->getListSelectedValue());

		this->setDetail3TextureName(this->detail3TextureName->getString());

		this->setBlendMode3(this->blendMode3->getListSelectedValue());

		this->setDetail0NMTextureName(this->detail0NMTextureName->getString());

		this->setDetail1NMTextureName(this->detail1NMTextureName->getString());

		this->setDetail2NMTextureName(this->detail2NMTextureName->getString());

		this->setDetail3NMTextureName(this->detail3NMTextureName->getString());

		this->setReflectionTextureName(this->reflectionTextureName->getListSelectedValue());

		this->setEmissiveTextureName(this->emissiveTextureName->getString());
		// Attention: Metallic?? Is that new?
						// this->emissiveTextureName->setValue(this->getPbsTextureName(datablock, Ogre::PbsTextureTypes::PBSM_METALLIC));

		this->setTransparencyMode(this->transparencyMode->getListSelectedValue());

		this->setTransparency(this->transparency->getReal());

		this->setUseAlphaFromTextures(this->useAlphaFromTextures->getBool());

		this->setUVScale(this->uvScale->getUInt());

		this->setUseEmissiveAsLightMap(this->useEmissiveAsLightMap->getBool());

		this->setShadowConstBias(this->shadowConstBias->getReal());

		this->setBringToFront(this->bringToFront->getBool());

		this->setCutOff(this->cutOff->getBool());
	}

	Ogre::String DatablockPbsComponent::getClassName(void) const
	{
		return "DatablockPbsComponent";
	}

	Ogre::String DatablockPbsComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void DatablockPbsComponent::update(Ogre::Real dt, bool notSimulating)
	{
		
	}

	void DatablockPbsComponent::actualizeValue(Variant* attribute)
	{
		// Some attribute changed, component is no longer new! So textures can change
		this->newlyCreated = false;

		GameObjectComponent::actualizeValue(attribute);

		if (DatablockPbsComponent::AttrSubEntityIndex() == attribute->getName())
		{
			this->setSubEntityIndex(attribute->getUInt());
		}
		else if (DatablockPbsComponent::AttrWorkflow() == attribute->getName())
		{
			this->setWorkflow(attribute->getListSelectedValue());
		}
		else if (DatablockPbsComponent::AttrMetalness() == attribute->getName())
		{
			this->setMetalness(attribute->getReal());
		}
		else if (DatablockPbsComponent::AttrRoughness() == attribute->getName())
		{
			this->setRoughness(attribute->getReal());
		}
		else if (DatablockPbsComponent::AttrFresnel() == attribute->getName())
		{
			this->setFresnel(Ogre::Vector3(attribute->getVector4().x, attribute->getVector4().y, attribute->getVector4().z), static_cast<bool>(attribute->getVector4().w));
		}
		else if (DatablockPbsComponent::AttrIndexOfRefraction() == attribute->getName())
		{
			this->setIndexOfRefraction(Ogre::Vector3(attribute->getVector4().x, attribute->getVector4().y, attribute->getVector4().z), static_cast<bool>(attribute->getVector4().w));
		}
		else if (DatablockPbsComponent::AttrRefractionStrength() == attribute->getName())
		{
			this->setRefractionStrength(attribute->getReal());
		}
		else if (DatablockPbsComponent::AttrBrdf() == attribute->getName())
		{
			this->setBrdf(attribute->getListSelectedValue());
		}
		else if (DatablockPbsComponent::AttrTwoSidedLighting() == attribute->getName())
		{
			this->setTwoSidedLighting(attribute->getBool());
		}
		else if (DatablockPbsComponent::AttrOneSidedShadowCastCullingMode() == attribute->getName())
		{
			this->setOneSidedShadowCastCullingMode(attribute->getListSelectedValue());
		}
		else if (DatablockPbsComponent::AttrAlphaTest() == attribute->getName())
		{
			this->setAlphaTest(attribute->getListSelectedValue());
		}
		else if (DatablockPbsComponent::AttrAlphaTestThreshold() == attribute->getName())
		{
			this->setAlphaTestThreshold(attribute->getReal());
		}
		else if (DatablockPbsComponent::AttrReceiveShadows() == attribute->getName())
		{
			this->setReceiveShadows(attribute->getBool());
		}
		else if (DatablockPbsComponent::AttrDiffuseColor() == attribute->getName())
		{
			this->setDiffuseColor(attribute->getVector3());
		}
		else if (DatablockPbsComponent::AttrBackgroundColor() == attribute->getName())
		{
			this->setBackgroundColor(attribute->getVector4());
		}
		else if (DatablockPbsComponent::AttrSpecularColor() == attribute->getName())
		{
			this->setSpecularColor(attribute->getVector3());
		}
		else if (DatablockPbsComponent::AttrEmissiveColor() == attribute->getName())
		{
			this->setEmissiveColor(attribute->getVector3());
		}
		else if (DatablockPbsComponent::AttrDiffuseTexture() == attribute->getName())
		{
			this->setDiffuseTextureName(attribute->getString());
		}
		else if (DatablockPbsComponent::AttrNormalTexture() == attribute->getName())
		{
			this->setNormalTextureName(attribute->getString());
		}
		else if (DatablockPbsComponent::AttrNormalMapWeight() == attribute->getName())
		{
			this->setNormalMapWeight(attribute->getReal());
		}
		else if (DatablockPbsComponent::AttrClearCoat() == attribute->getName())
		{
			this->setClearCoat(attribute->getReal());
		}
		else if (DatablockPbsComponent::AttrClearCoatRoughness() == attribute->getName())
		{
			this->setClearCoatRoughness(attribute->getReal());
		}
		else if (DatablockPbsComponent::AttrSpecularTexture() == attribute->getName())
		{
			this->setSpecularTextureName(attribute->getString());
		}
		else if (DatablockPbsComponent::AttrMetallicTexture() == attribute->getName())
		{
			this->setMetallicTextureName(attribute->getString());
		}
		else if (DatablockPbsComponent::AttrRoughnessTexture() == attribute->getName())
		{
			this->setRoughnessTextureName(attribute->getString());
		}
		else if (DatablockPbsComponent::AttrDetailWeightTexture() == attribute->getName())
		{
			this->setDetailWeightTextureName(attribute->getString());
		}
		else if (DatablockPbsComponent::AttrDetail0Texture() == attribute->getName())
		{
			this->setDetail0TextureName(attribute->getString());
		}
		else if (DatablockPbsComponent::AttrBlendMode0() == attribute->getName())
		{
			this->setBlendMode0(attribute->getListSelectedValue());
		}
		else if (DatablockPbsComponent::AttrDetail1Texture() == attribute->getName())
		{
			this->setDetail1TextureName(attribute->getString());
		}
		else if (DatablockPbsComponent::AttrBlendMode1() == attribute->getName())
		{
			this->setBlendMode1(attribute->getListSelectedValue());
		}
		else if (DatablockPbsComponent::AttrDetail2Texture() == attribute->getName())
		{
			this->setDetail2TextureName(attribute->getString());
		}
		else if (DatablockPbsComponent::AttrBlendMode2() == attribute->getName())
		{
			this->setBlendMode2(attribute->getListSelectedValue());
		}
		else if (DatablockPbsComponent::AttrDetail3Texture() == attribute->getName())
		{
			this->setDetail3TextureName(attribute->getString());
		}
		else if (DatablockPbsComponent::AttrBlendMode3() == attribute->getName())
		{
			this->setBlendMode3(attribute->getListSelectedValue());
		}
		else if (DatablockPbsComponent::AttrDetail0NMTexture() == attribute->getName())
		{
			this->setDetail0NMTextureName(attribute->getString());
		}
		else if (DatablockPbsComponent::AttrDetail1NMTexture() == attribute->getName())
		{
			this->setDetail1NMTextureName(attribute->getString());
		}
		else if (DatablockPbsComponent::AttrDetail2NMTexture() == attribute->getName())
		{
			this->setDetail2NMTextureName(attribute->getString());
		}
		else if (DatablockPbsComponent::AttrDetail3NMTexture() == attribute->getName())
		{
			this->setDetail3NMTextureName(attribute->getString());
		}
		else if (DatablockPbsComponent::AttrReflectionTexture() == attribute->getName())
		{
			this->setReflectionTextureName(attribute->getListSelectedValue());
		}
		else if (DatablockPbsComponent::AttrEmissiveTexture() == attribute->getName())
		{
			this->setEmissiveTextureName(attribute->getString());
		}
		else if (DatablockPbsComponent::AttrTransparencyMode() == attribute->getName())
		{
			this->setTransparencyMode(attribute->getListSelectedValue());
		}
		else if (DatablockPbsComponent::AttrTransparency() == attribute->getName())
		{
			this->setTransparency(attribute->getReal());
		}
		else if (DatablockPbsComponent::AttrUseAlphaFromTextures() == attribute->getName())
		{
			this->setUseAlphaFromTextures(attribute->getBool());
		}
		else if (DatablockPbsComponent::AttrUVScale() == attribute->getName())
		{
			this->setUVScale(attribute->getUInt());
		}
		else if (DatablockPbsComponent::AttrUseEmissiveAsLightMap() == attribute->getName())
		{
			this->setUseEmissiveAsLightMap(attribute->getBool());
		}
		else if (DatablockPbsComponent::AttrShadowConstBias() == attribute->getName())
		{
			this->setShadowConstBias(attribute->getReal());
		}
		else if (DatablockPbsComponent::AttrBringToFront() == attribute->getName())
		{
			this->setBringToFront(attribute->getBool());
		}
		else if (DatablockPbsComponent::AttrCutOff() == attribute->getName())
		{
			this->setCutOff(attribute->getBool());
		}
	}

	void DatablockPbsComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SubEntityIndex"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->subEntityIndex->getUInt())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Workflow"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->workflow->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Metalness"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->metalness->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Roughness"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roughness->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Fresnel"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fresnel->getVector4())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "IndexOfRefraction"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->indexOfRefraction->getVector4())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RefractionStrength"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->refractionStrength->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Brdf"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->brdf->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TwoSidedLighting"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->twoSidedLighting->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OneSidedShadowCastCullingMode"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->oneSidedShadowCastCullingMode->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AlphaTest"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->alphaTest->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AlphaTestThreshold"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->alphaTestThreshold->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ReceiveShadows"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->receiveShadows->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DiffuseColor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->diffuseColor->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BackgroundColor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->backgroundColor->getVector4())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SpecularColor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->specularColor->getVector3())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "EmissiveColor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->emissiveColor->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DiffuseTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->diffuseTextureName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "NormalTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->normalTextureName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "NormalMapWeight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->normalMapWeight->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ClearCoat"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->clearCoat->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ClearCoatRoughness"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->clearCoatRoughness->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SpecularTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->specularTextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MetallicTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->metallicTextureName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RoughnessTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roughnessTextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DetailWeightTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->detailWeightTextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail0TextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->detail0TextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BlendMode0"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->blendMode0->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail1TextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->detail1TextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BlendMode1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->blendMode1->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail2TextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->detail2TextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BlendMode2"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->blendMode2->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail3TextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->detail3TextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BlendMode3"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->blendMode3->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail0NMTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->detail0NMTextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail1NMTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->detail1NMTextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail2NMTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->detail2NMTextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail3NMTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->detail3NMTextureName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ReflectionTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->reflectionTextureName->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "EmissiveTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->emissiveTextureName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TransparencyMode"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->transparencyMode->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Transparency"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->transparency->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseAlphaFromTextures"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useAlphaFromTextures->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UVScale"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->uvScale->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseEmissiveAsLightMap"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useEmissiveAsLightMap->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShadowConstBias"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->shadowConstBias->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BringToFront"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->bringToFront->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CutOff"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->cutOff->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	void DatablockPbsComponent::internalSetTextureName(Ogre::PbsTextureTypes pbsTextureType, Ogre::CommonTextureTypes::CommonTextureTypes textureType, Variant* attribute, const Ogre::String& textureName)
	{
		Ogre::TextureGpuManager* hlmsTextureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();

		Ogre::String previousTextureName = attribute->getString();

		// If the data block component has just been created, get texture name from existing data block
		Ogre::String tempTextureName = textureName;
		
		if (Ogre::PBSM_REFLECTION != pbsTextureType)
		{
			// If the data block component has just been created, get texture name from existing data block
			if (true == newlyCreated && nullptr != this->datablock)
			{
				tempTextureName = this->getPbsTextureName(this->datablock, pbsTextureType);
			}
			attribute->setValue(tempTextureName);
			this->addAttributeFilePathData(attribute);
		}
		else
		{
			// If dynamic cubemap is used, skip texture creation as a reflection scenario with dynamic cubemap and camera is used!
			if ("cubemap" == textureName)
			{
				attribute->getList().clear();
				attribute->getList().emplace_back("cubemap");
				attribute->setListSelectedValue("cubemap");
				attribute->setReadOnly(true);
				return;
			}
			else
			{
				this->setReflectionTextureNames();
				attribute->setListSelectedValue(textureName);
				attribute->setReadOnly(false);

				/*Ogre::TextureGpu* reflectionTexture = datablock->getTexture(pbsTextureType);
				if (nullptr != reflectionTexture)
				{
					hlmsTextureManager->destroyTexture(reflectionTexture);
				}*/
			}
		}
		
		// Store the old texture name as user data
		if (false == tempTextureName.empty())
		{
			attribute->addUserData("OldTexture", tempTextureName);
		}
		else if (nullptr != this->datablock)
		{
			attribute->addUserData("OldTexture", this->getPbsTextureName(this->datablock, pbsTextureType));
		}

		if (nullptr != this->datablock /*&& false == newlyCreated*/)
		{
			// If no texture has been loaded, but still the data block has an internal one, get this one and remove it manually!
			if (true == attribute->getUserDataValue("OldTexture").empty())
			{
				tempTextureName = this->getPbsTextureName(this->datablock, pbsTextureType);
				attribute->addUserData(tempTextureName);
			}

			// createOrRetrieveTexture crashes, when texture alias name is empty
			Ogre::String oldTextureName = attribute->getUserDataValue("OldTexture");
			if (false == oldTextureName.empty())
			{
				if (false == Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(oldTextureName))
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DatablockPbsComponent] Cannot set texture: '" + oldTextureName + 
						"', because it does not exist in any resource group! for game object: " + this->gameObjectPtr->getName());
					attribute->setValue(previousTextureName);
					this->addAttributeFilePathData(attribute);
					return;
				}

				Ogre::uint32 textureFlags = Ogre::TextureFlags::AutomaticBatching;

				if (this->datablock->suggestUsingSRGB(pbsTextureType))
				{
					textureFlags |= Ogre::TextureFlags::PrefersLoadingFromFileAsSRGB;
				}

				Ogre::TextureTypes::TextureTypes internalTextureType = Ogre::TextureTypes::Type2D;
				if (Ogre::PBSM_REFLECTION == pbsTextureType)
				{
					textureFlags |= ~Ogre::TextureFlags::AutomaticBatching;
					textureFlags |= Ogre::TextureFlags::RenderToTexture;
					textureFlags |= Ogre::TextureFlags::AllowAutomipmaps;
					internalTextureType = Ogre::TextureTypes::TypeCube;
				}

				Ogre::uint32 filters = Ogre::TextureFilter::FilterTypes::TypeGenerateDefaultMipmaps;
				filters |= this->datablock->suggestFiltersForType(pbsTextureType);

				// Really important: createOrRetrieveTexture when its created, its width/height is 0 etc. so the texture is just prepared
				// it will be filled with correct data when setDataBlock is called
				Ogre::TextureGpu* texture = hlmsTextureManager->createOrRetrieveTexture(oldTextureName,
					Ogre::GpuPageOutStrategy::SaveToSystemRam, textureFlags, internalTextureType,
					Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, filters, 0u);
				
				// Check if its a valid texture
				if (nullptr != texture)
				{
					if (Ogre::PBSM_REFLECTION == pbsTextureType)
					{
						texture->setResolution(1024u, 1024u);
						texture->setNumMipmaps(Ogre::PixelFormatGpuUtils::getMaxMipmapCount(1024u, 1024u));
						texture->setPixelFormat(Ogre::PFG_RGBA8_UNORM_SRGB);
					}

					if (nullptr != texture)
					{
						try
						{
							texture->scheduleTransitionTo(Ogre::GpuResidency::Resident);
							/*if (false == this->newlyCreated)
							{
								hlmsTextureManager->waitForStreamingCompletion();
							}*/
						}
						catch (const Ogre::Exception& exception)
						{
							Ogre::LogManager::getSingleton().logMessage(exception.getFullDescription(), Ogre::LML_CRITICAL);
							Ogre::LogManager::getSingleton().logMessage("[DatablockPbsComponent] Error: Could not set texture: '" + oldTextureName + "' For game object: " + this->gameObjectPtr->getName(), Ogre::LML_CRITICAL);
							attribute->setValue(previousTextureName);
							this->addAttributeFilePathData(attribute);
							return;
						}
					}

					// Note: width may be 0, when create or retrieve is called, if its a heavy resolution texture. So the width/height becomes available after waitForData, if its still 0, texture is invalid!
					if (0 == texture->getWidth())
					{
						hlmsTextureManager->waitForStreamingCompletion();
					}
					// Invalid texture skip
					if (0 == texture->getWidth())
					{
						 attribute->setValue(previousTextureName);
						 this->addAttributeFilePathData(attribute);
						 return;
					}
					// If texture has been removed, set null texture, so that it will be removed from data block
					if (true == tempTextureName.empty())
					{
						hlmsTextureManager->destroyTexture(texture);
						texture = nullptr;
					}

					const Ogre::HlmsSamplerblock* tempSamplerBlock = this->datablock->getSamplerblock(pbsTextureType);
					if (nullptr != tempSamplerBlock)
					{
						Ogre::HlmsSamplerblock samplerblock(*tempSamplerBlock);
						samplerblock.mU = Ogre::TAM_WRAP;
						samplerblock.mV = Ogre::TAM_WRAP;
						samplerblock.mW = Ogre::TAM_WRAP;

						samplerblock.mMinFilter = Ogre::FO_ANISOTROPIC;
						samplerblock.mMagFilter = Ogre::FO_ANISOTROPIC;
						samplerblock.mMipFilter = Ogre::FO_ANISOTROPIC;
						samplerblock.mMaxAnisotropy = 1; // test also with -1;

						this->datablock->setTexture(pbsTextureType, texture, &samplerblock);
					}
					else
					{
						Ogre::HlmsSamplerblock samplerblock;
						samplerblock.mU = Ogre::TAM_WRAP;
						samplerblock.mV = Ogre::TAM_WRAP;
						samplerblock.mW = Ogre::TAM_WRAP;

						samplerblock.mMinFilter = Ogre::FO_ANISOTROPIC;
						samplerblock.mMagFilter = Ogre::FO_ANISOTROPIC;
						samplerblock.mMipFilter = Ogre::FO_ANISOTROPIC;
						samplerblock.mMaxAnisotropy = 1; // test also with -1;

						this->datablock->setSamplerblock(pbsTextureType, samplerblock);
						this->datablock->setTexture(pbsTextureType, texture, &samplerblock);

						// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DatablockPbsComponent] Can not apply texture for this datablock, because it has no sampler block for game object: " + this->gameObjectPtr->getName());
					}
				}
				else
				{
					attribute->setValue("");
					attribute->removeUserData("OldTexture");
				}
			}
		}
	}

	void DatablockPbsComponent::setSubEntityIndex(unsigned int subEntityIndex)
	{
		// if (GameObject::AttrCustomDataSkipCreation() == this->gameObjectPtr->getCustomDataString())
		// {
		// 	return;
		// }

		this->subEntityIndex->setValue(subEntityIndex);

		if (nullptr != this->gameObjectPtr)
		{
			// Two data block components with the same entity index can not exist
			for (unsigned int i = 0; i < static_cast<unsigned int>(this->gameObjectPtr->getComponents()->size()); i++)
			{
				auto& priorPbsComponent = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<DatablockPbsComponent>(DatablockPbsComponent::getStaticClassName(), i));
				if (nullptr != priorPbsComponent && priorPbsComponent.get() != this)
				{
					if (subEntityIndex == priorPbsComponent->getSubEntityIndex())
					{
						subEntityIndex = priorPbsComponent->getSubEntityIndex() + 1;
					}
				}
			}

			Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
			Ogre::Item* item = nullptr;
			if (nullptr != entity)
			{
				if (subEntityIndex >= static_cast<unsigned int>(entity->getNumSubEntities()))
				{
					subEntityIndex = static_cast<unsigned int>(entity->getNumSubEntities()) - 1;
				}
			}
			else
			{
				item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
				if (nullptr != item)
				{
					if (subEntityIndex >= static_cast<unsigned int>(item->getNumSubItems()))
					{
						subEntityIndex = static_cast<unsigned int>(item->getNumSubItems()) - 1;
					}
				}
			}

			if (this->oldSubIndex != subEntityIndex)
			{
				// Read everything from the beginning, if a new sub index has been set
				this->newlyCreated = true;

				// Old data block must be destroyed
				if (true == this->alreadyCloned)
				{
					// Set back the default datablock
					if (nullptr != entity)
					{
						entity->getSubEntity(this->oldSubIndex)->setDatablock(this->originalDatablock);
					}
					else if (nullptr != item)
					{
						item->getSubItem(this->oldSubIndex)->setDatablock(this->originalDatablock);
					}
					
					if (nullptr != this->datablock)
					{
						Ogre::Hlms* hlmsPbs = Ogre::Root::getSingletonPtr()->getHlmsManager()->getHlms(Ogre::HLMS_PBS);
						hlmsPbs->destroyDatablock(this->datablock->getName());
						this->datablock = nullptr;
					}
				}

				this->alreadyCloned = false;
			}
			this->preReadDatablock();

			this->oldSubIndex = subEntityIndex;
		}
	}

	unsigned int DatablockPbsComponent::getSubEntityIndex(void) const
	{
		return this->subEntityIndex->getUInt();
	}
	
	void DatablockPbsComponent::setWorkflow(const Ogre::String& workflow)
	{
		this->workflow->setListSelectedValue(workflow);

		if ("MetallicWorkflow" == workflow)
		{
			this->fresnel->setVisible(false);
			this->indexOfRefraction->setVisible(false);
			this->metalness->setVisible(true);
		}
		else if ("SpecularAsFresnelWorkflow" == workflow)
		{
			this->metalness->setVisible(false);
			this->fresnel->setVisible(true);
			this->indexOfRefraction->setVisible(true);
		}
		else if ("SpecularWorkflow" == workflow)
		{
			this->metalness->setVisible(false);
			this->fresnel->setVisible(true);
			this->indexOfRefraction->setVisible(true);
		}

		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
				this->datablock->setWorkflow(this->mapStringToWorkflow(workflow));
			else
				this->workflow->setListSelectedValue(this->mapWorkflowToString(this->datablock->getWorkflow()));
		}
	}
	
	Ogre::String DatablockPbsComponent::getWorkflow(void) const
	{
		return this->workflow->getListSelectedValue();
	}
	
	void DatablockPbsComponent::setMetalness(Ogre::Real metalness)
	{
		this->metalness->setValue(metalness);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
			{
				if (Ogre::HlmsPbsDatablock::MetallicWorkflow == this->datablock->getWorkflow())
				{
					this->datablock->setMetalness(metalness);
				}
			}
			else
				this->metalness->setValue(this->datablock->getMetalness());
		}
	}
	
	Ogre::Real DatablockPbsComponent::getMetalness(void) const
	{
		return this->metalness->getReal();
	}

	void DatablockPbsComponent::setRoughness(Ogre::Real roughness)
	{
		if (roughness < 0.001f)
		{
			roughness = 0.001f;
		}

		this->roughness->setValue(roughness);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
				this->datablock->setRoughness(roughness);
			else
				this->roughness->setValue(this->datablock->getRoughness());
		}
	}

	Ogre::Real DatablockPbsComponent::getRoughness(void) const
	{
		return this->roughness->getReal();
	}
	
	void DatablockPbsComponent::setFresnel(const Ogre::Vector3& fresnel, bool separateFresnel)
	{
		this->fresnel->setValue(Ogre::Vector4(fresnel.x, fresnel.y, fresnel.z, static_cast<Ogre::Real>(separateFresnel)));
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
			{
				if (Ogre::HlmsPbsDatablock::MetallicWorkflow != this->datablock->getWorkflow())
				{
					this->datablock->setFresnel(Ogre::Vector3(fresnel.x, fresnel.y, fresnel.z), separateFresnel);
				}
			}
			// Do not read from datablock because it will look ugly, set default values
			else
			{
				this->fresnel->setValue(Ogre::Vector4(this->datablock->getFresnel().x, this->datablock->getFresnel().y, this->datablock->getFresnel().z,
					static_cast<Ogre::Real>(this->datablock->hasSeparateFresnel())));
			}
		}
	}
	
	Ogre::Vector4 DatablockPbsComponent::getFresnel(void) const
	{
		return this->fresnel->getVector4();
	}

	void DatablockPbsComponent::setIndexOfRefraction(const Ogre::Vector3& refractionIdx, bool separateFresnel)
	{
		this->indexOfRefraction->setValue(Ogre::Vector4(refractionIdx.x, refractionIdx.y, refractionIdx.z, static_cast<Ogre::Real>(separateFresnel)));
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
			{
				if (Ogre::HlmsPbsDatablock::MetallicWorkflow != this->datablock->getWorkflow())
				{
					this->datablock->setIndexOfRefraction(Ogre::Vector3(refractionIdx.x, refractionIdx.y, refractionIdx.z), separateFresnel);
				}
			}
		}
	}

	Ogre::Vector4 DatablockPbsComponent::getIndexOfRefraction(void) const
	{
		return this->indexOfRefraction->getVector4();
	}

	void DatablockPbsComponent::setRefractionStrength(Ogre::Real refractionStrength)
	{
		this->refractionStrength->setValue(refractionStrength);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
				this->datablock->setRefractionStrength(refractionStrength);
			else
				this->refractionStrength->setValue(this->datablock->getRefractionStrength());
		}
	}

	Ogre::Real DatablockPbsComponent::getRefractionStrength(void) const
	{
		return this->refractionStrength->getReal();
	}
	
	void DatablockPbsComponent::setBrdf(const Ogre::String& brdf)
	{
		this->brdf->setListSelectedValue(brdf);
		if (nullptr != this->datablock)
		{
			Ogre::PbsBrdf::PbsBrdf brdfFlags = this->mapStringToBrdf(brdf);
			bool isDefaultBrdf = (brdfFlags & Ogre::PbsBrdf::BRDF_MASK) == Ogre::PbsBrdf::Default;

			this->clearCoat->setVisible(isDefaultBrdf);
			this->clearCoatRoughness->setVisible(isDefaultBrdf);

			if (false == newlyCreated)
				this->datablock->setBrdf(brdfFlags);
			else
				this->brdf->setListSelectedValue(this->mapBrdfToString(static_cast<Ogre::PbsBrdf::PbsBrdf>(this->datablock->getBrdf())));
		}
	}
	
	Ogre::String DatablockPbsComponent::getBrdf(void) const
	{
		return this->brdf->getListSelectedValue();
	}
	
	void DatablockPbsComponent::setTwoSidedLighting(bool twoSided)
	{
		this->twoSidedLighting->setValue(twoSided);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
				this->datablock->setTwoSidedLighting(twoSided, true, this->mapStringToCullingMode(this->oneSidedShadowCastCullingMode->getListSelectedValue()));
			else
				this->twoSidedLighting->setValue(this->datablock->getTwoSidedLighting());
		}
	}
		
	bool DatablockPbsComponent::getTwoSidedLighting(void) const
	{
		return this->twoSidedLighting->getBool();
	}
	
	void DatablockPbsComponent::setOneSidedShadowCastCullingMode(const Ogre::String& cullingMode)
	{
		this->oneSidedShadowCastCullingMode->setListSelectedValue(cullingMode);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
				// true = default = change macro block
				this->datablock->setTwoSidedLighting(this->twoSidedLighting->getBool(), true, this->mapStringToCullingMode(this->oneSidedShadowCastCullingMode->getListSelectedValue()));
			else
				this->oneSidedShadowCastCullingMode->setListSelectedValue("CULL_ANTICLOCKWISE");
		}
	}
		
	Ogre::String DatablockPbsComponent::getOneSidedShadowCastCullingMode(void) const
	{
		return this->oneSidedShadowCastCullingMode->getListSelectedValue();
	}

	void DatablockPbsComponent::setAlphaTest(const Ogre::String& alphaTest)
	{
		this->alphaTest->setListSelectedValue(alphaTest);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
				this->datablock->setAlphaTest(this->mapStringToAlphaTest(this->alphaTest->getListSelectedValue()));
			else
				this->alphaTest->setListSelectedValue(this->mapAlphaTestToString(this->datablock->getAlphaTest()));
		}
	}

	Ogre::String DatablockPbsComponent::getAlphaTest(void) const
	{
		return this->alphaTest->getListSelectedValue();
	}
	
	void DatablockPbsComponent::setAlphaTestThreshold(Ogre::Real threshold)
	{
		this->alphaTestThreshold->setValue(threshold);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
				this->datablock->setAlphaTestThreshold(threshold);
			else
				this->alphaTestThreshold->setValue(this->datablock->getAlphaTestThreshold());
		}
	}
		
	Ogre::Real DatablockPbsComponent::getAlphaTestThreshold(void) const
	{
		return this->alphaTestThreshold->getReal();
	}
	
	void DatablockPbsComponent::setReceiveShadows(bool receiveShadows)
	{
		this->receiveShadows->setValue(receiveShadows);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
				this->datablock->setReceiveShadows(receiveShadows);
			else
				this->receiveShadows->setValue(this->datablock->getReceiveShadows());
		}
	}
		
	bool DatablockPbsComponent::getReceiveShadows(void) const
	{
		return this->receiveShadows->getBool();
	}
	
	void DatablockPbsComponent::setDiffuseColor(const Ogre::Vector3& diffuseColor)
	{
		this->diffuseColor->setValue(diffuseColor);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
				this->datablock->setDiffuse(diffuseColor);
			else
				this->diffuseColor->setValue(this->datablock->getDiffuse());
		}
	}

	Ogre::Vector3 DatablockPbsComponent::getDiffuseColor(void) const
	{
		return this->diffuseColor->getVector3();
	}

	void DatablockPbsComponent::setBackgroundColor(const Ogre::Vector4& backgroundColor)
	{
		this->backgroundColor->setValue(backgroundColor);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
				this->datablock->setBackgroundDiffuse(Ogre::ColourValue(backgroundColor.x, backgroundColor.y, backgroundColor.z, backgroundColor.w));
			else
			{
				this->backgroundColor->setValue(Ogre::Vector4(this->datablock->getBackgroundDiffuse().r, this->datablock->getBackgroundDiffuse().g,
					this->datablock->getBackgroundDiffuse().b, this->datablock->getBackgroundDiffuse().a));
			}
		}
	}

	Ogre::Vector4 DatablockPbsComponent::getBackgroundColor(void) const
	{
		return this->backgroundColor->getVector4();
	}

	void DatablockPbsComponent::setSpecularColor(const Ogre::Vector3& specularColor)
	{
		this->specularColor->setValue(specularColor);
		
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
				this->datablock->setSpecular(specularColor);
			else
				this->specularColor->setValue(this->datablock->getSpecular());
		}
	}

	Ogre::Vector3 DatablockPbsComponent::getSpecularColor(void) const
	{
		return this->specularColor->getVector3();
	}
	
	void DatablockPbsComponent::setEmissiveColor(const Ogre::Vector3& emissiveColor)
	{
		this->emissiveColor->setValue(emissiveColor);
		
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
				this->datablock->setEmissive(emissiveColor);
			else
				this->emissiveColor->setValue(this->datablock->getEmissive());
		}
	}

	Ogre::Vector3 DatablockPbsComponent::getEmissiveColor(void) const
	{
		return this->emissiveColor->getVector3();
	}

	void DatablockPbsComponent::setDiffuseTextureName(const Ogre::String& diffuseTextureName)
	{
		this->internalSetTextureName(Ogre::PBSM_DIFFUSE, Ogre::CommonTextureTypes::Diffuse, this->diffuseTextureName, diffuseTextureName);
	}

	const Ogre::String DatablockPbsComponent::getDiffuseTextureName(void) const
	{
		return this->diffuseTextureName->getString();
	}

	void DatablockPbsComponent::setNormalTextureName(const Ogre::String& normalTextureName)
	{
		this->internalSetTextureName(Ogre::PBSM_NORMAL, Ogre::CommonTextureTypes::NormalMap, this->normalTextureName, normalTextureName);
	}

	const Ogre::String DatablockPbsComponent::getNormalTextureName(void) const
	{
		return this->normalTextureName->getString();
	}

	void DatablockPbsComponent::setNormalMapWeight(Ogre::Real normalMapWeight)
	{
		this->normalMapWeight->setValue(normalMapWeight);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
				this->datablock->setNormalMapWeight(normalMapWeight);
			else
				this->normalMapWeight->setValue(this->datablock->getNormalMapWeight());
		}
	}

	Ogre::Real DatablockPbsComponent::getNormalMapWeight(void) const
	{
		return this->normalMapWeight->getReal();
	}

	void DatablockPbsComponent::setClearCoat(Ogre::Real clearCoat)
	{
		if (false == this->clearCoat->isVisible())
		{
			return;
		}
		this->clearCoat->setValue(clearCoat);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated && (this->datablock->getBrdf() & Ogre::PbsBrdf::BRDF_MASK) == Ogre::PbsBrdf::Default)
			{
				this->datablock->setClearCoat(clearCoat);
			}
			else
				this->clearCoat->setValue(this->datablock->getClearCoat());
		}
	}

	Ogre::Real DatablockPbsComponent::getClearCoat(void) const
	{
		return this->clearCoat->getReal();
	}

	void DatablockPbsComponent::setClearCoatRoughness(Ogre::Real clearCoatRoughness)
	{
		if (false == this->clearCoatRoughness->isVisible())
		{
			return;
		}
		this->clearCoatRoughness->setValue(clearCoatRoughness);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
				this->datablock->setClearCoatRoughness(clearCoatRoughness);
			else
				this->clearCoat->setValue(this->datablock->getClearCoatRoughness());
		}
	}

	Ogre::Real DatablockPbsComponent::getClearCoatRoughness(void) const
	{
		return this->clearCoatRoughness->getReal();
	}

	void DatablockPbsComponent::setSpecularTextureName(const Ogre::String& specularTextureName)
	{
		this->internalSetTextureName(Ogre::PBSM_SPECULAR, Ogre::CommonTextureTypes::Diffuse, this->specularTextureName, specularTextureName);
	}

	const Ogre::String DatablockPbsComponent::getSpecularTextureName(void) const
	{
		return this->specularTextureName->getString();
	}
	
	void DatablockPbsComponent::setMetallicTextureName(const Ogre::String& metallicTextureName)
	{
		this->internalSetTextureName(Ogre::PBSM_METALLIC, Ogre::CommonTextureTypes::Diffuse, this->metallicTextureName, metallicTextureName);
	}

	const Ogre::String DatablockPbsComponent::getMetallicTextureName(void) const
	{
		return this->metallicTextureName->getString();
	}

	void DatablockPbsComponent::setRoughnessTextureName(const Ogre::String& roughnessTextureName)
	{
		this->internalSetTextureName(Ogre::PBSM_ROUGHNESS, Ogre::CommonTextureTypes::Monochrome, this->roughnessTextureName, roughnessTextureName);
	}

	const Ogre::String DatablockPbsComponent::getRoughnessTextureName(void) const
	{
		return this->roughnessTextureName->getString();
	}

	void DatablockPbsComponent::setDetailWeightTextureName(const Ogre::String& detailWeightTextureName)
	{
		this->internalSetTextureName(Ogre::PBSM_DETAIL_WEIGHT, Ogre::CommonTextureTypes::NonColourData, this->detailWeightTextureName, detailWeightTextureName);
	}
	
	const Ogre::String DatablockPbsComponent::getDetailWeightTextureName(void) const
	{
		return this->detailWeightTextureName->getString();
	}
	
	void DatablockPbsComponent::setDetail0TextureName(const Ogre::String& detail0TextureName)
	{
		// Ogre::HlmsTextureManager::TEXTURE_TYPE_DETAIL
// Attention: There is no detail type anymore, what type is correct?
		this->internalSetTextureName(Ogre::PBSM_DETAIL0, Ogre::CommonTextureTypes::Diffuse, this->detail0TextureName, detail0TextureName);
	}
	
	const Ogre::String DatablockPbsComponent::getDetail0TextureName(void) const
	{
		return this->detail0TextureName->getString();
	}
	
	void DatablockPbsComponent::setBlendMode0(const Ogre::String& blendMode0)
	{
		this->blendMode0->setListSelectedValue(blendMode0);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
				this->datablock->setDetailMapBlendMode(0, this->mapStringToBlendMode(blendMode0));
			else
				this->blendMode0->setListSelectedValue(this->mapBlendModeToString(this->datablock->getDetailMapBlendMode(0)));
		}
	}
	
    Ogre::String DatablockPbsComponent::getBlendMode0(void) const
	{
		return this->blendMode0->getListSelectedValue();
	}
	
	void DatablockPbsComponent::setDetail1TextureName(const Ogre::String& detail1TextureName)
	{
		this->internalSetTextureName(Ogre::PBSM_DETAIL1, Ogre::CommonTextureTypes::Diffuse, this->detail1TextureName, detail1TextureName);
	}
	
	const Ogre::String DatablockPbsComponent::getDetail1TextureName(void) const
	{
		return this->detail1TextureName->getString();
	}
	
	void DatablockPbsComponent::setBlendMode1(const Ogre::String& blendMode1)
	{
		this->blendMode1->setListSelectedValue(blendMode1);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
				this->datablock->setDetailMapBlendMode(1, this->mapStringToBlendMode(blendMode1));
			else
				this->blendMode1->setListSelectedValue(this->mapBlendModeToString(this->datablock->getDetailMapBlendMode(1)));
		}
	}
	
    Ogre::String DatablockPbsComponent::getBlendMode1(void) const
	{
		return this->blendMode1->getListSelectedValue();
	}
	
	void DatablockPbsComponent::setDetail2TextureName(const Ogre::String& detail2TextureName)
	{
		this->internalSetTextureName(Ogre::PBSM_DETAIL2, Ogre::CommonTextureTypes::Diffuse, this->detail2TextureName, detail2TextureName);
	}
	
	const Ogre::String DatablockPbsComponent::getDetail2TextureName(void) const
	{
		return this->detail2TextureName->getString();
	}
	
	void DatablockPbsComponent::setBlendMode2(const Ogre::String& blendMode2)
	{
		this->blendMode2->setListSelectedValue(blendMode2);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
				this->datablock->setDetailMapBlendMode(2, this->mapStringToBlendMode(blendMode2));
			else
				this->blendMode2->setListSelectedValue(this->mapBlendModeToString(this->datablock->getDetailMapBlendMode(2)));
		}
	}
	
    Ogre::String DatablockPbsComponent::getBlendMode2(void) const
	{
		return this->blendMode2->getListSelectedValue();
	}
	
	void DatablockPbsComponent::setDetail3TextureName(const Ogre::String& detail3TextureName)
	{
		this->internalSetTextureName(Ogre::PBSM_DETAIL3, Ogre::CommonTextureTypes::Diffuse, this->detail3TextureName, detail3TextureName);
	}
	
	const Ogre::String DatablockPbsComponent::getDetail3TextureName(void) const
	{
		return this->detail3TextureName->getString();
	}
	
	void DatablockPbsComponent::setBlendMode3(const Ogre::String& blendMode3)
	{
		this->blendMode3->setListSelectedValue(blendMode3);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
				this->datablock->setDetailMapBlendMode(3, this->mapStringToBlendMode(blendMode3));
			else
				this->blendMode3->setListSelectedValue(this->mapBlendModeToString(this->datablock->getDetailMapBlendMode(3)));
		}
	}

    Ogre::String DatablockPbsComponent::getBlendMode3(void) const
	{
		return this->blendMode3->getListSelectedValue();
	}
	
	void DatablockPbsComponent::setDetail0NMTextureName(const Ogre::String& detail0NMTextureName)
	{
		// Attention: There is no Ogre::HlmsTextureManager::TEXTURE_TYPE_DETAIL_NORMAL_MAP type anymore, what is correct?
		this->internalSetTextureName(Ogre::PBSM_DETAIL0_NM, Ogre::CommonTextureTypes::NormalMap, this->detail0NMTextureName, detail0NMTextureName);
	}
	
	const Ogre::String DatablockPbsComponent::getDetail0NMTextureName(void) const
	{
		return this->detail0NMTextureName->getString();
	}
	
	void DatablockPbsComponent::setDetail1NMTextureName(const Ogre::String& detail1NMTextureName)
	{
		this->internalSetTextureName(Ogre::PBSM_DETAIL1_NM, Ogre::CommonTextureTypes::NormalMap, this->detail1NMTextureName, detail1NMTextureName);
	}
	
	const Ogre::String DatablockPbsComponent::getDetail1NMTextureName(void) const
	{
		return this->detail1NMTextureName->getString();
	}
	
	void DatablockPbsComponent::setDetail2NMTextureName(const Ogre::String& detail2NMTextureName)
	{
		this->internalSetTextureName(Ogre::PBSM_DETAIL2_NM, Ogre::CommonTextureTypes::NormalMap, this->detail2NMTextureName, detail2NMTextureName);
	}
	
	const Ogre::String DatablockPbsComponent::getDetail2NMTextureName(void) const
	{
		return this->detail2NMTextureName->getString();
	}
	
	void DatablockPbsComponent::setDetail3NMTextureName(const Ogre::String& detail3NMTextureName)
	{
		this->internalSetTextureName(Ogre::PBSM_DETAIL3_NM, Ogre::CommonTextureTypes::NormalMap, this->detail3NMTextureName, detail3NMTextureName);
	}
	
	const Ogre::String DatablockPbsComponent::getDetail3NMTextureName(void) const
	{
		return this->detail3NMTextureName->getString();
	}
	
	void DatablockPbsComponent::setReflectionTextureName(const Ogre::String& reflectionTextureName)
	{
		this->internalSetTextureName(Ogre::PBSM_REFLECTION, Ogre::CommonTextureTypes::EnvMap, this->reflectionTextureName, reflectionTextureName);
	}

	const Ogre::String DatablockPbsComponent::getReflectionTextureName(void) const
	{
		return this->reflectionTextureName->getListSelectedValue();
	}

	void DatablockPbsComponent::setEmissiveTextureName(const Ogre::String& emissiveTextureName)
	{
		this->internalSetTextureName(Ogre::PBSM_EMISSIVE, Ogre::CommonTextureTypes::Diffuse, this->emissiveTextureName, emissiveTextureName);
	}

	const Ogre::String DatablockPbsComponent::getEmissiveTextureName(void) const
	{
		return this->emissiveTextureName->getString();
	}

	void DatablockPbsComponent::setTransparencyMode(const Ogre::String& transparencyMode)
	{
		this->transparencyMode->setListSelectedValue(transparencyMode);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
				this->datablock->setTransparency(this->transparency->getReal(), this->mapStringToTransparencyMode(transparencyMode), this->useAlphaFromTextures->getBool());
			else
				this->transparencyMode->setListSelectedValue(this->mapTransparencyModeToString(this->datablock->getTransparencyMode()));
		}
	}

	Ogre::String DatablockPbsComponent::getTransparencyMode(void)
	{
		return this->transparencyMode->getListSelectedValue();
	}

	void DatablockPbsComponent::setTransparency(Ogre::Real transparency)
	{
		this->transparency->setValue(transparency);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
			{
				this->datablock->setTransparency(transparency, this->mapStringToTransparencyMode(this->transparencyMode->getListSelectedValue()), this->useAlphaFromTextures->getBool());
				// Change render queue index, so that other game objects can be rendered correctly after this transparent game object
				if (this->transparency->getReal() < 1.0f)
				{
					this->gameObjectPtr->setRenderQueueIndex(10);
				}
				else
				{
					this->gameObjectPtr->setRenderQueueIndex(1);
				}
			}
			else
			{
				this->transparency->setValue(this->datablock->getTransparency());
			}
		}
	}

	Ogre::Real DatablockPbsComponent::getTransparency(void) const
	{
		return this->transparency->getReal();
	}
	
	void DatablockPbsComponent::setUseAlphaFromTextures(bool useAlphaFromTextures)
	{
		this->useAlphaFromTextures->setValue(useAlphaFromTextures);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
				this->datablock->setTransparency(this->transparency->getReal(), this->mapStringToTransparencyMode(this->transparencyMode->getListSelectedValue()), useAlphaFromTextures);
			else
				this->useAlphaFromTextures->setValue(this->datablock->getUseAlphaFromTextures());
		}
	}
	
	bool DatablockPbsComponent::getUseAlphaFromTextures(void) const
	{
		return this->useAlphaFromTextures->getBool();
	}

	void DatablockPbsComponent::setUVScale(unsigned int uvScale)
	{
		this->uvScale->setValue(uvScale);
		// Causes endless exceptions!
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
			{
				// Ogre::Matrix4 mat(Ogre::Matrix4::IDENTITY);
				// mat.setScale(Ogre::Vector3(-1, 1, 1));
				// this->datablock->setEnableAnimationMatrix(0, true);
				// this->datablock->setAnimationMatrix(0, mat);
				/*
				The "1" in "setTexture( 1" and "setTextureUvSource( 1" indicate a virtual texture unit.
				As the doxygen doc says, it starts from 0; therefore the first texture should use the value 0, rather than 1.
				setTextureUvSource Says which uv will be used with which tex unit;
				so:
				CODE: SELECT ALL

				datablock->setTextureUvSource( 0, 0 ); //Tex Unit 0 will be sampled using uv0
				datablock->setTextureUvSource( 0, 1 ); //Tex Unit 0 will be sampled using uv1
				datablock->setTextureUvSource( 1, 0 ); //Tex Unit 1 will be sampled using uv0
				datablock->setTextureUvSource( 1, 1 ); //Tex Unit 1 will be sampled using uv1
				As for layering stuff together, I just noticed it couldn't be changed programatically, so I added the interface just now. Pull latest changes.
				*/
			//	for (unsigned short i = 0; i < Ogre::PbsTextureTypes::NUM_PBSM_TEXTURE_TYPES - 1; i++)
			//	{
			//		// https://forums.ogre3d.org/viewtopic.php?t=82797
			//		this->datablock->setTextureUvSource(static_cast<Ogre::PbsTextureTypes>(i), 0);
			//		this->datablock->setTextureUvSource(static_cast<Ogre::PbsTextureTypes>(i), uvScale);
			//		//static_cast<Ogre::HlmsUnlitDatablock*>(this->ribbonTrail->getDatablock())->setTextureUvSource(0, 0); //Tex Unit 0 will be sampled using uv0
			////static_cast<Ogre::HlmsUnlitDatablock*>(this->ribbonTrail->getDatablock())->setTextureUvSource(0, 1); //Tex Unit 0 will be sampled using uv1
			//	}
				/*this->datablock->setTextureUvSource(static_cast<Ogre::PbsTextureTypes>(0), 0);
				this->datablock->setTextureUvSource(static_cast<Ogre::PbsTextureTypes>(0), uvScale);
				this->datablock->setTextureUvSource(static_cast<Ogre::PbsTextureTypes>(1), 0);
				this->datablock->setTextureUvSource(static_cast<Ogre::PbsTextureTypes>(1), uvScale);*/
				/*
				22:09:33: Couldn't apply change to datablock '' for this renderable. Using default one. Check previous log messages to see if there's more information.
				22:09:33: OGRE EXCEPTION(1:InvalidStateException): Renderable needs at least 2 coordinates in UV set #1. Either change the mesh, or change the UV source settings in HlmsPbs::calculateHashForPreCreate at D:\Ogre\GameEngineDevelopment\external\Ogre2.1SDK\Components\Hlms\Pbs\src\OgreHlmsPbs.cpp (line 649)
				22:09:33: OGRE EXCEPTION(1:InvalidStateException): Renderable needs at least 2 coordinates in UV set #1. Either change the mesh, or change the UV source settings in HlmsPbs::calculateHashForPreCreate at D:\Ogre\GameEngineDevelopment\external\Ogre2.1SDK\Components\Hlms\Pbs\src\OgreHlmsPbs.cpp (line 649)
				22:09:33: Couldn't apply change to datablock '' for this renderable. Using default one. Check previous log messages to see if there's more information.
				*/
			}
				
			else
			{
				this->uvScale->setValue(this->datablock->getTextureUvSource(Ogre::PbsTextureTypes::PBSM_DIFFUSE));
			}
		}
	}

	unsigned int DatablockPbsComponent::getUVScale(void) const
	{
		return this->uvScale->getUInt();
	}

	void DatablockPbsComponent::setShadowConstBias(Ogre::Real shadowConstBias)
	{
		this->shadowConstBias->setValue(shadowConstBias);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
				this->datablock->mShadowConstantBias = shadowConstBias;
			else
				this->shadowConstBias->setValue(this->datablock->mShadowConstantBias);
		}
	}

	Ogre::Real DatablockPbsComponent::getShadowConstBias(void) const
	{
		return this->shadowConstBias->getReal();
	}

	void DatablockPbsComponent::setBringToFront(bool bringToFront)
	{
		this->bringToFront->setValue(bringToFront);
		
		if (true == bringToFront)
		{
			this->originalMacroblock = const_cast<Ogre::HlmsMacroblock*>(this->datablock->getMacroblock());
			Ogre::HlmsMacroblock macroblock;
			macroblock.mDepthWrite = true;
			macroblock.mDepthFunc = Ogre::CompareFunction::CMPF_ALWAYS_PASS;
			macroblock.mScissorTestEnabled = true;
			this->datablock->setMacroblock(macroblock);
		}
		else
		{
			if (nullptr != this->originalMacroblock)
			{
				this->datablock->setMacroblock(*this->originalMacroblock);
			}
		}
	}

	bool DatablockPbsComponent::getIsInFront(void) const
	{
		return this->bringToFront->getBool();
	}

	void DatablockPbsComponent::setCutOff(bool cutOff)
	{
		this->cutOff->setValue(cutOff);

		if (true == cutOff)
		{
			this->originalMacroblock = const_cast<Ogre::HlmsMacroblock*>(this->datablock->getMacroblock());
			this->originalBlendblock = const_cast<Ogre::HlmsBlendblock*>(this->datablock->getBlendblock());
			Ogre::HlmsMacroblock macroblock;
			macroblock.mDepthWrite = true;
			macroblock.mDepthFunc = Ogre::CompareFunction::CMPF_ALWAYS_PASS;
			// macroblock.mScissorTestEnabled = true;
			this->datablock->setMacroblock(macroblock);

			Ogre::HlmsBlendblock blendblock;
			blendblock.mBlendChannelMask = Ogre::HlmsBlendblock::BlendChannelForceDisabled;

			this->datablock->setBlendblock(blendblock);
		}
		else
		{
			if (nullptr != this->originalMacroblock)
			{
				this->datablock->setMacroblock(*this->originalMacroblock);
			}
			if (nullptr != this->originalBlendblock)
			{
				this->datablock->setBlendblock(*this->originalBlendblock);
			}
		}
	}

	bool DatablockPbsComponent::getCutOff(void) const
	{
		return this->cutOff->getBool();
	}

	void DatablockPbsComponent::setUseEmissiveAsLightMap(bool useEmissiveAsLightMap)
	{
		this->useEmissiveAsLightMap->setValue(useEmissiveAsLightMap);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
				this->datablock->setUseEmissiveAsLightmap(useEmissiveAsLightMap);
			else
				this->useEmissiveAsLightMap->setValue(this->datablock->getUseEmissiveAsLightmap());
		}
	}

	bool DatablockPbsComponent::getUseEmissiveAsLightMap(void) const
	{
		return this->useEmissiveAsLightMap->getBool();
	}
	
	Ogre::HlmsPbsDatablock* DatablockPbsComponent::getDataBlock(void) const
	{
		return this->datablock;
	}

	void DatablockPbsComponent::doNotCloneDataBlock(void)
	{
		this->alreadyCloned = true;
	}
	
}; // namespace end