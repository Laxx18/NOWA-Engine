#include "NOWAPrecompiled.h"
#include "DatablockPbsComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "modules/DeployResourceModule.h"

namespace NOWA
{
	using namespace rapidxml;

	DatablockPbsComponent::DatablockPbsComponent()
		: GameObjectComponent(),
		subEntityIndex(new Variant(DatablockPbsComponent::AttrSubEntityIndex(), static_cast<unsigned int>(0), this->attributes)),
		workflow(new Variant(DatablockPbsComponent::AttrWorkflow(), { "SpecularWorkflow", "SpecularAsFresnelWorkflow", "MetallicWorkflow" }, this->attributes)),
		metalness(new Variant(DatablockPbsComponent::AttrMetalness(), 0.818f, this->attributes)),
		roughness(new Variant(DatablockPbsComponent::AttrRoughness(), 1.0f, this->attributes)),
		fresnel(new Variant(DatablockPbsComponent::AttrFresnel(), Ogre::Vector4(0.01f, 0.01f, 0.01f, 0.0f), this->attributes)),
		brdf(new Variant(DatablockPbsComponent::AttrBrdf(), { "Default", "CookTorrance", "BlinnPhong", "DefaultUncorrelated", "DefaultSeparateDiffuseFresnel", 
																	"CookTorranceSeparateDiffuseFresnel", "BlinnPhongSeparateDiffuseFresnel", "BlinnPhongLegacyMath", "BlinnPhongFullLegacy" }, this->attributes)),
		twoSidedLighting(new Variant(DatablockPbsComponent::AttrTwoSidedLighting(), false, this->attributes)),
		oneSidedShadowCastCullingMode(new Variant(DatablockPbsComponent::AttrOneSidedShadowCastCullingMode(), { "CULL_NONE", "CULL_CLOCKWISE", "CULL_ANTICLOCKWISE" }, this->attributes)),
		alphaTestThreshold(new Variant(DatablockPbsComponent::AttrAlphaTestThreshold(), 1.0f, this->attributes)),
		receiveShadows(new Variant(DatablockPbsComponent::AttrReceiveShadows(), true, this->attributes)),
		diffuseColor(new Variant(DatablockPbsComponent::AttrDiffuseColor(), Ogre::Vector3(3.14f, 3.14f, 3.14f), this->attributes)),
		backgroundColor(new Variant(DatablockPbsComponent::AttrBackgroundColor(), Ogre::Vector4(1.0f, 1.0f, 1.0f, 1.0f), this->attributes)),
		specularColor(new Variant(DatablockPbsComponent::AttrSpecularColor(), Ogre::Vector3::UNIT_SCALE, this->attributes)),
		emissiveColor(new Variant(DatablockPbsComponent::AttrEmissiveColor(), Ogre::Vector3::ZERO, this->attributes)),
		diffuseTextureName(new Variant(DatablockPbsComponent::AttrDiffuseTexture(), Ogre::String(), this->attributes)),
		normalTextureName(new Variant(DatablockPbsComponent::AttrNormalTexture(), Ogre::String(), this->attributes)),
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
		reflectionTextureName(new Variant(DatablockPbsComponent::AttrReflectionTexture(), Ogre::String(), this->attributes)),
		emissiveTextureName(new Variant(DatablockPbsComponent::AttrEmissiveTexture(), Ogre::String(), this->attributes)),
		transparencyMode(new Variant(DatablockPbsComponent::AttrTransparencyMode(), { "None", "Transparent", "Fade" }, this->attributes)),
		transparency(new Variant(DatablockPbsComponent::AttrTransparency(), 1.0f, this->attributes)),
		useAlphaFromTextures(new Variant(DatablockPbsComponent::AttrUseAlphaFromTextures(), true, this->attributes)),
		uvScale(new Variant(DatablockPbsComponent::AttrUVScale(), static_cast<unsigned int>(1), this->attributes)),
		datablock(nullptr),
		originalDatablock(nullptr),
		alreadyCloned(false),
		newlyCreated(true),
		oldSubIndex(0)
	{
		this->subEntityIndex->setDescription("Specifies the sub entity index, for which the datablock should be shown.");
		this->subEntityIndex->setUserData(GameObject::AttrActionNeedRefresh());
		this->workflow->setUserData(GameObject::AttrActionNeedRefresh());
		this->diffuseColor->setUserData(GameObject::AttrActionColorDialog());
		this->backgroundColor->setUserData(GameObject::AttrActionColorDialog());
		this->specularColor->setUserData(GameObject::AttrActionColorDialog());
		this->emissiveColor->setUserData(GameObject::AttrActionColorDialog());
		this->uvScale->setDescription("Scale texture and wrap, min: 1 max: 8");
	}

	DatablockPbsComponent::~DatablockPbsComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DatablockPbsComponent] Destructor datablock pbs component for game object: " + this->gameObjectPtr->getName());

		// If a datablock has been cloned, it must be destroyed manually
		if (true == this->alreadyCloned)
		{
			Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject <Ogre::v1::Entity>();
			if (nullptr != entity)
			{
				// Set back the default datablock
				entity->getSubEntity(this->oldSubIndex)->setDatablock(this->originalDatablock);
			}
			if (nullptr != this->datablock)
			{
				Ogre::Hlms* hlmsPbs = Ogre::Root::getSingletonPtr()->getHlmsManager()->getHlms(Ogre::HLMS_PBS);
				hlmsPbs->destroyDatablock(this->datablock->getName());
				this->datablock = nullptr;
			}
		}

	}

	bool DatablockPbsComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		propertyElement = propertyElement->next_sibling("property");
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
		

		this->newlyCreated = false;

		return true;
	}

	GameObjectCompPtr DatablockPbsComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		DatablockPbsCompPtr clonedCompPtr(boost::make_shared<DatablockPbsComponent>());

		clonedCompPtr->setSubEntityIndex(this->subEntityIndex->getUInt());
		clonedCompPtr->setWorkflow(this->workflow->getListSelectedValue());
		clonedCompPtr->setMetalness(this->metalness->getReal());
		clonedCompPtr->setRoughness(this->roughness->getReal());
		clonedCompPtr->setFresnel(Ogre::Vector3(this->fresnel->getVector4().x, this->fresnel->getVector4().y, this->fresnel->getVector4().z), static_cast<bool>(this->fresnel->getVector4().w));
		clonedCompPtr->setBrdf(this->brdf->getListSelectedValue());
		clonedCompPtr->setTwoSidedLighting(this->twoSidedLighting->getBool());
		clonedCompPtr->setOneSidedShadowCastCullingMode(this->oneSidedShadowCastCullingMode->getListSelectedValue());
		clonedCompPtr->setAlphaTestThreshold(this->alphaTestThreshold->getReal());
		clonedCompPtr->setReceiveShadows(this->receiveShadows->getBool());
		clonedCompPtr->setDiffuseColor(this->diffuseColor->getVector3());
		clonedCompPtr->setBackgroundColor(this->backgroundColor->getVector4());
		clonedCompPtr->setSpecularColor(this->specularColor->getVector3());
		clonedCompPtr->setEmissiveColor(this->emissiveColor->getVector3());
		
		clonedCompPtr->setDiffuseTextureName(this->diffuseTextureName->getString());
		clonedCompPtr->setNormalTextureName(this->normalTextureName->getString());
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
		clonedCompPtr->setReflectionTextureName(this->reflectionTextureName->getString());
		clonedCompPtr->setEmissiveTextureName(this->emissiveTextureName->getString());

		clonedCompPtr->setTransparencyMode(this->transparencyMode->getListSelectedValue());
		clonedCompPtr->setTransparency(this->transparency->getReal());
		clonedCompPtr->setUseAlphaFromTextures(this->useAlphaFromTextures->getBool());
		clonedCompPtr->setUVScale(this->uvScale->getUInt());
		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);
		
		return clonedCompPtr;
	}

	bool DatablockPbsComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DatablockPbsComponent] Init datablock pbs component for game object: " + this->gameObjectPtr->getName());
		
		this->preReadDatablock();

		return true;
	}

	void DatablockPbsComponent::onRemoveComponent(void)
	{
		// Does crash
		//// If a datablock has been cloned, it must be destroyed manually
		//if (true == this->alreadyCloned)
		//{
		//	Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject <Ogre::v1::Entity>();
		//	if (nullptr != entity)
		//	{
		//		entity->getSubEntity(this->subEntityIndex->getUInt())->setDatablock(this->originalDataBlockName);
		//	}
		//	if (nullptr != this->datablock)
		//	{
		//		Ogre::Hlms* hlmsPbs = Ogre::Root::getSingletonPtr()->getHlmsManager()->getHlms(Ogre::HLMS_PBS);
		//		hlmsPbs->destroyDatablock(this->datablock->getName());
		//		this->datablock = nullptr;
		//	}
		//}
	}

#if 0
	Ogre::String DatablockPbsComponent::getPbsTextureName(Ogre::HlmsPbsDatablock* datablock, Ogre::PbsTextureTypes type)
	{
		Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();
		Ogre::HlmsTextureManager* hlmsTextureManager = hlmsManager->getTextureManager();
		Ogre::HlmsTextureManager::TextureLocation texLocation;

		texLocation.texture = datablock->getTexture(type);
		if (false == texLocation.texture.isNull())
		{
			texLocation.xIdx = datablock->_getTextureIdx(type);
			texLocation.yIdx = 0;
			texLocation.divisor = 1;

			const Ogre::String* textureName = hlmsManager->getTextureManager()->findAliasName(texLocation);
			if (nullptr != textureName)
			{
				return *textureName;
			}
		}
		return "";
	}
#endif

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
		Ogre::PbsBrdf::PbsBrdf brdf = Ogre::PbsBrdf::Default;
		if ("CookTorrance" == strBrdf)
			brdf = Ogre::PbsBrdf::CookTorrance;
		else if ("BlinnPhong" == strBrdf)
			brdf = Ogre::PbsBrdf::BlinnPhong;
		else if ("DefaultUncorrelated" == strBrdf)
			brdf = Ogre::PbsBrdf::DefaultUncorrelated;
		else if ("DefaultSeparateDiffuseFresnel" == strBrdf)
			brdf = Ogre::PbsBrdf::DefaultSeparateDiffuseFresnel;
		else if ("CookTorranceSeparateDiffuseFresnel" == strBrdf)
			brdf = Ogre::PbsBrdf::CookTorranceSeparateDiffuseFresnel;
		else if ("BlinnPhongSeparateDiffuseFresnel" == strBrdf)
			brdf = Ogre::PbsBrdf::BlinnPhongSeparateDiffuseFresnel;
		else if ("BlinnPhongLegacyMath" == strBrdf)
			brdf = Ogre::PbsBrdf::BlinnPhongLegacyMath;
		else if ("BlinnPhongFullLegacy" == strBrdf)
			brdf = Ogre::PbsBrdf::BlinnPhongFullLegacy;

		return brdf;
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
	
	void DatablockPbsComponent::preReadDatablock(void)
	{
		bool success = true;
		
		if (nullptr == this->gameObjectPtr)
			return;

		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity)
		{
			success = this->readDatablockEntity(entity);
		}

		if (false == success)
		{
			this->datablock = nullptr;
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DatablockPbsComponent] Datablock reading failed for game object: " + this->gameObjectPtr->getName());
			return;
		}
	}

	bool DatablockPbsComponent::readDatablockEntity(Ogre::v1::Entity* entity)
	{
		if (this->subEntityIndex->getUInt() >= entity->getNumSubEntities())
		{
			this->datablock = nullptr;
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DatablockPbsComponent] Datablock reading failed, because there is no such sub entity index: "
				+ Ogre::StringConverter::toString(this->subEntityIndex->getUInt()) + " for game object: "
				+ this->gameObjectPtr->getName());
			this->subEntityIndex->setValue(static_cast<unsigned int>(0));
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
		// Store also the original name
		this->originalDataBlockName = *this->originalDatablock->getNameStr();

		if ("Missing" == originalDataBlockName)
			return false;

		if (false == this->alreadyCloned && nullptr != this->originalDatablock)
		{
			this->datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(this->originalDatablock->clone(this->originalDataBlockName
				+ Ogre::StringConverter::toString(this->gameObjectPtr->getId())));
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

		this->setBrdf(this->brdf->getListSelectedValue());

		this->setTwoSidedLighting(this->twoSidedLighting->getBool());

		this->setOneSidedShadowCastCullingMode(this->oneSidedShadowCastCullingMode->getListSelectedValue());

		this->setAlphaTestThreshold(this->alphaTestThreshold->getReal());

		this->setReceiveShadows(this->receiveShadows->getBool());

		this->diffuseColor->setValue(this->datablock->getDiffuse());

		this->backgroundColor->setValue(Ogre::Vector4(this->datablock->getBackgroundDiffuse().r, this->datablock->getBackgroundDiffuse().g,
			this->datablock->getBackgroundDiffuse().b, this->datablock->getBackgroundDiffuse().a));

		this->specularColor->setValue(this->datablock->getSpecular());
		
		this->emissiveColor->setValue(this->datablock->getEmissive());

		this->setDiffuseTextureName(this->diffuseTextureName->getString());

		this->setNormalTextureName(this->normalTextureName->getString());

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

		this->setReflectionTextureName(this->reflectionTextureName->getString());

		this->setEmissiveTextureName(this->emissiveTextureName->getString());
		// Attention: Metallic?? Is that new?
						// this->emissiveTextureName->setValue(this->getPbsTextureName(datablock, Ogre::PbsTextureTypes::PBSM_METALLIC));

		this->setTransparencyMode(this->transparencyMode->getListSelectedValue());

		this->setTransparency(this->transparency->getReal());

		this->setUseAlphaFromTextures(this->useAlphaFromTextures->getBool());

		this->setUVScale(this->uvScale->getUInt());

		// Does not help
		// this->datablock->mShadowConstantBias = 0.001f;
		//Ogre::HlmsSamplerblock samplerblock(*datablock->getSamplerblock(Ogre::PBSM_DIFFUSE));
		//samplerblock.mU = Ogre::TAM_WRAP;
		//samplerblock.mV = Ogre::TAM_WRAP;
		//samplerblock.mW = Ogre::TAM_WRAP;
		////Set the new samplerblock. The Hlms system will
		////automatically create the API block if necessary
		//datablock->setSamplerblock(Ogre::PBSM_DIFFUSE, samplerblock);
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
			this->setFresnel(Ogre::Vector3(attribute->getVector4().x, attribute->getVector4().y,attribute->getVector4().z), static_cast<bool>(attribute->getVector4().w));
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
			this->setReflectionTextureName(attribute->getString());
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
	}

	void DatablockPbsComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ComponentDatablockPbs"));
		propertyXML->append_attribute(doc.allocate_attribute("data", "DatablockPbsComponent"));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SubEntityIndex"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->subEntityIndex->getUInt())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Workflow"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->workflow->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Metalness"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->metalness->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Roughness"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->roughness->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Fresnel"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->fresnel->getVector4())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Brdf"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->brdf->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TwoSidedLighting"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->twoSidedLighting->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OneSidedShadowCastCullingMode"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->oneSidedShadowCastCullingMode->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AlphaTestThreshold"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->alphaTestThreshold->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ReceiveShadows"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->receiveShadows->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DiffuseColor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->diffuseColor->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BackgroundColor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->backgroundColor->getVector4())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SpecularColor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->specularColor->getVector3())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "EmissiveColor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->emissiveColor->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DiffuseTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->diffuseTextureName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "NormalTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->normalTextureName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SpecularTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->specularTextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MetallicTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->metallicTextureName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RoughnessTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->roughnessTextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DetailWeightTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->detailWeightTextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail0TextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->detail0TextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BlendMode0"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->blendMode0->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail1TextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->detail1TextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BlendMode1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->blendMode1->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail2TextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->detail2TextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BlendMode2"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->blendMode2->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail3TextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->detail3TextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BlendMode3"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->blendMode3->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail0NMTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->detail0NMTextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail1NMTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->detail1NMTextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail2NMTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->detail2NMTextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail3NMTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->detail3NMTextureName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ReflectionTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->reflectionTextureName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "EmissiveTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->emissiveTextureName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TransparencyMode"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->transparencyMode->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Transparency"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->transparency->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseAlphaFromTextures"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->useAlphaFromTextures->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UVScale"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->uvScale->getUInt())));
		propertiesXML->append_node(propertyXML);
	}

#if 0
	void DatablockPbsComponent::internalSetTextureName(Ogre::PbsTextureTypes pbsTextureType, Ogre::HlmsTextureManager::TextureMapType textureType, Variant* attribute, const Ogre::String& textureName)
	{
		// If the data block component has just been created, get texture name from existing data block
		Ogre::String tempTextureName = textureName;
		if (true == newlyCreated && nullptr != this->datablock)
		{
			tempTextureName = this->getPbsTextureName(this->datablock, pbsTextureType);
		}

		attribute->setValue(tempTextureName);
		// Store the old texture name as user data
		if (false == tempTextureName.empty())
			attribute->setUserData(tempTextureName);
		
		if (nullptr != this->datablock && false == newlyCreated)
		{
			// If no texture has been loaded, but still the data block has an internal one, get this one and remove it manually!
			if (true == attribute->getUserData().first.empty())
			{
				tempTextureName = this->getPbsTextureName(this->datablock, pbsTextureType);
				attribute->setUserData(tempTextureName);
				// Retrieve the texture and remove it from data block
				tempTextureName = "";
			}
			
			// createOrRetrieveTexture crashes, when texture alias name is empty
			if (false == attribute->getUserData().first.empty())
			{
				Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();
				Ogre::HlmsTextureManager* hlmsTextureManager = hlmsManager->getTextureManager();
				Ogre::HlmsTextureManager::TextureLocation texLocation = hlmsTextureManager->createOrRetrieveTexture(attribute->getUserData().first, textureType);
				if (nullptr != texLocation.texture)
				{
					// If texture has been removed, set null texture, so that it will be removed from data block
					if (true == tempTextureName.empty())
					{
						texLocation.texture.setNull();
					}
					datablock->setTexture(pbsTextureType, texLocation.xIdx, texLocation.texture);
				}
			}
		}
	}
#endif

	void DatablockPbsComponent::internalSetTextureName(Ogre::PbsTextureTypes pbsTextureType, Ogre::CommonTextureTypes::CommonTextureTypes textureType, Variant* attribute, const Ogre::String& textureName)
	{
		// If the data block component has just been created, get texture name from existing data block
		Ogre::String tempTextureName = textureName;
		if (true == newlyCreated && nullptr != this->datablock)
		{
			tempTextureName = this->getPbsTextureName(this->datablock, pbsTextureType);
		}

		attribute->setValue(tempTextureName);
		// Store the old texture name as user data
		if (false == tempTextureName.empty())
			attribute->setUserData(tempTextureName);

		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
			{
				// If no texture has been loaded, but still the data block has an internal one, get this one and remove it manually!
				if (true == attribute->getUserData().first.empty())
				{
					tempTextureName = this->getPbsTextureName(this->datablock, pbsTextureType);
					attribute->setUserData(tempTextureName);
					// Retrieve the texture and remove it from data block
					tempTextureName = "";
				}
			}
			else
			{
				// createOrRetrieveTexture crashes, when texture alias name is empty
				if (false == attribute->getUserData().first.empty())
				{
					Ogre::TextureGpuManager* hlmsTextureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();

					Ogre::uint32 textureFlags = Ogre::TextureFlags::AutomaticBatching;

					if (datablock->suggestUsingSRGB(pbsTextureType))
						textureFlags |= Ogre::TextureFlags::PrefersLoadingFromFileAsSRGB;

					Ogre::TextureTypes::TextureTypes internalTextureType = Ogre::TextureTypes::Type2D;
					if (textureType == Ogre::PBSM_REFLECTION)
					{
						internalTextureType = Ogre::TextureTypes::TypeCube;
						textureFlags &= ~Ogre::TextureFlags::AutomaticBatching;
					}

					Ogre::uint32 filters = Ogre::TextureFilter::FilterTypes::TypeGenerateDefaultMipmaps;
					filters |= datablock->suggestFiltersForType(pbsTextureType);

					Ogre::TextureGpu* texture = hlmsTextureManager->createOrRetrieveTexture(attribute->getUserData().first,
						attribute->getUserData().first, Ogre::GpuPageOutStrategy::Discard, textureFlags, internalTextureType,
						Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, filters);

					//Ogre::TextureGpu* texture = hlmsTextureManager->createOrRetrieveTexture(attribute->getUserData().first, Ogre::GpuPageOutStrategy::Discard,
					//	textureType, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, 0); // Attention: Poolid = 0, is that correct?

					if (nullptr != texture)
					{
						// Attention: wait here, or before the loop?
						texture->scheduleTransitionTo(Ogre::GpuResidency::Resident);

						// If texture has been removed, set null texture, so that it will be removed from data block
						if (true == tempTextureName.empty())
						{
							// Attention: Is that correct?
							hlmsTextureManager->destroyTexture(texture);
							texture = nullptr;
						}
						datablock->setTexture(pbsTextureType, texture);
					}
				}
			}
		}
	}

	void DatablockPbsComponent::setSubEntityIndex(unsigned int subEntityIndex)
	{
		this->subEntityIndex->setValue(subEntityIndex);

		if (this->oldSubIndex != subEntityIndex)
		{
			// Read everything from the beginning, if a new sub index has been set
			this->newlyCreated = true;

			// Old data block must be destroyed
			if (true == this->alreadyCloned)
			{
				Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject <Ogre::v1::Entity>();
				if (nullptr != entity)
				{
					// Set back the default datablock
					entity->getSubEntity(this->oldSubIndex)->setDatablock(this->originalDatablock);
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
			this->metalness->setVisible(true);
		}
		else if ("SpecularAsFresnelWorkflow" == workflow)
		{
			this->metalness->setVisible(false);
			this->fresnel->setVisible(true);
		}
		else if ("SpecularWorkflow" == workflow)
		{
			this->metalness->setVisible(false);
			this->fresnel->setVisible(true);
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
			 	this->fresnel->setValue(Ogre::Vector4(this->datablock->getFresnel().x, this->datablock->getFresnel().y, this->datablock->getFresnel().z, 
			 		static_cast<Ogre::Real>(this->datablock->hasSeparateFresnel())));
		}
	}
	
	Ogre::Vector4 DatablockPbsComponent::getFresnel(void) const
	{
		return this->fresnel->getVector4();
	}
	
	void DatablockPbsComponent::setBrdf(const Ogre::String& brdf)
	{
		this->brdf->setListSelectedValue(brdf);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
				this->datablock->setBrdf(this->mapStringToBrdf(brdf));
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
		return this->reflectionTextureName->getString();
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
				this->datablock->setTransparency(transparency, this->mapStringToTransparencyMode(this->transparencyMode->getListSelectedValue()), this->useAlphaFromTextures->getBool());
			else
				this->transparency->setValue(this->datablock->getTransparency());
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
	
	Ogre::HlmsPbsDatablock* DatablockPbsComponent::getDataBlock(void) const
	{
		return this->datablock;
	}

	void DatablockPbsComponent::doNotCloneDataBlock(void)
	{
		this->alreadyCloned = true;
	}
	
}; // namespace end