#include "NOWAPrecompiled.h"
#include "DatablockTerraComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "modules/DeployResourceModule.h"
#include "modules/WorkspaceModule.h"
#include "main/ProcessManager.h"
#include "TerraComponent.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	DatablockTerraComponent::DatablockTerraComponent()
		: GameObjectComponent(),
		datablock(nullptr),
		originalDatablock(nullptr),
		alreadyCloned(false),
		newlyCreated(true),
		brdf(new Variant(DatablockTerraComponent::AttrBrdf(), { "Default", "CookTorrance", "BlinnPhong", "DefaultUncorrelated", "DefaultSeparateDiffuseFresnel",
																	"CookTorranceSeparateDiffuseFresnel", "BlinnPhongSeparateDiffuseFresnel", "BlinnPhongLegacyMath", "BlinnPhongFullLegacy" }, this->attributes)),
		diffuseColor(new Variant(DatablockTerraComponent::AttrDiffuseColor(), Ogre::Vector3(3.14f, 3.14f, 3.14f), this->attributes)),
		diffuseTextureName(new Variant(DatablockTerraComponent::AttrDiffuse(), Ogre::String(), this->attributes)),
		detail0TextureName(new Variant(DatablockTerraComponent::AttrDetail0(), Ogre::String(), this->attributes)),
		offsetScale0(new Variant(DatablockTerraComponent::AttrOffsetScale0(), Ogre::Vector4(0.0f, 0.0f, 128.0f, 128.0f), this->attributes)),
		detail1TextureName(new Variant(DatablockTerraComponent::AttrDetail1(), Ogre::String(), this->attributes)),
		offsetScale1(new Variant(DatablockTerraComponent::AttrOffsetScale1(), Ogre::Vector4(0.0f, 0.0f, 128.0f, 128.0f), this->attributes)),
		detail2TextureName(new Variant(DatablockTerraComponent::AttrDetail2(), Ogre::String(), this->attributes)),
		offsetScale2(new Variant(DatablockTerraComponent::AttrOffsetScale2(), Ogre::Vector4(0.0f, 0.0f, 128.0f, 128.0f), this->attributes)),
		detail3TextureName(new Variant(DatablockTerraComponent::AttrDetail3(), Ogre::String(), this->attributes)),
		offsetScale3(new Variant(DatablockTerraComponent::AttrOffsetScale3(), Ogre::Vector4(0.0f, 0.0f, 128.0f, 128.0f), this->attributes)),
		detail0NMTextureName(new Variant(DatablockTerraComponent::AttrDetail0NM(), Ogre::String(), this->attributes)),
		detail1NMTextureName(new Variant(DatablockTerraComponent::AttrDetail1NM(), Ogre::String(), this->attributes)),
		detail2NMTextureName(new Variant(DatablockTerraComponent::AttrDetail2NM(), Ogre::String(), this->attributes)),
		detail3NMTextureName(new Variant(DatablockTerraComponent::AttrDetail3NM(), Ogre::String(), this->attributes)),
		detail0RTextureName(new Variant(DatablockTerraComponent::AttrDetail0R(), Ogre::String(), this->attributes)),
		roughness0(new Variant(DatablockTerraComponent::AttrRoughness0(), 1.0f, this->attributes)),
		detail1RTextureName(new Variant(DatablockTerraComponent::AttrDetail1R(), Ogre::String(), this->attributes)),
		roughness1(new Variant(DatablockTerraComponent::AttrRoughness1(), 1.0f, this->attributes)),
		detail2RTextureName(new Variant(DatablockTerraComponent::AttrDetail2R(), Ogre::String(), this->attributes)),
		roughness2(new Variant(DatablockTerraComponent::AttrRoughness2(), 1.0f, this->attributes)),
		detail3RTextureName(new Variant(DatablockTerraComponent::AttrDetail3R(), Ogre::String(), this->attributes)),
		roughness3(new Variant(DatablockTerraComponent::AttrRoughness3(), 1.0f, this->attributes)),
		detail0MTextureName(new Variant(DatablockTerraComponent::AttrDetail0M(), Ogre::String(), this->attributes)),
		metalness0(new Variant(DatablockTerraComponent::AttrMetalness0(), 1.0f, this->attributes)),
		detail1MTextureName(new Variant(DatablockTerraComponent::AttrDetail1M(), Ogre::String(), this->attributes)),
		metalness1(new Variant(DatablockTerraComponent::AttrMetalness1(), 1.0f, this->attributes)),
		detail2MTextureName(new Variant(DatablockTerraComponent::AttrDetail2M(), Ogre::String(), this->attributes)),
		metalness2(new Variant(DatablockTerraComponent::AttrMetalness2(), 1.0f, this->attributes)),
		detail3MTextureName(new Variant(DatablockTerraComponent::AttrDetail3M(), Ogre::String(), this->attributes)),
		metalness3(new Variant(DatablockTerraComponent::AttrMetalness3(), 1.0f, this->attributes)),
		reflectionTextureName(new Variant(DatablockTerraComponent::AttrReflection(), Ogre::String(), this->attributes))
	{
		this->diffuseColor->addUserData(GameObject::AttrActionColorDialog());

		// Note: setUserData2 is already used for internal purposes and would cause a crash! Only setUserData must be used
		this->diffuseTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "TerrainTextures");
		this->detail0TextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "TerrainTextures");
		this->detail1TextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "TerrainTextures");
		this->detail2TextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "TerrainTextures");
		this->detail3TextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "TerrainTextures");
		this->detail0NMTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "TerrainTextures");
		this->detail1NMTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "TerrainTextures");
		this->detail2NMTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "TerrainTextures");
		this->detail3NMTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "TerrainTextures");
		this->detail0RTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "TerrainTextures");
		this->detail1RTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "TerrainTextures");
		this->detail2RTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "TerrainTextures");
		this->detail3RTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "TerrainTextures");
		this->detail0MTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "TerrainTextures");
		this->detail1MTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "TerrainTextures");
		this->detail2MTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "TerrainTextures");
		this->detail3MTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "TerrainTextures");
		this->reflectionTextureName->addUserData(GameObject::AttrActionFileOpenDialog(), "TerrainTextures");

		Ogre::String dimmingInfo = "If some areas are saturated with white spots, like a shine reflection. The following can be done for dimming: "
									"1) Directly through metalness: Lower this value to a minimum."
									"2) Indirectly through roughness: Increase this value to dim it";

		this->roughness0->setDescription(dimmingInfo);
		this->roughness1->setDescription(dimmingInfo);
		this->roughness2->setDescription(dimmingInfo);
		this->roughness3->setDescription(dimmingInfo);
		this->metalness0->setDescription(dimmingInfo);
		this->metalness1->setDescription(dimmingInfo);
		this->metalness2->setDescription(dimmingInfo);
		this->metalness3->setDescription(dimmingInfo);
	}

	DatablockTerraComponent::~DatablockTerraComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DatablockTerraComponent] Destructor datablock terra component for game object: " + this->gameObjectPtr->getName());
	}

	bool DatablockTerraComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		// Attention: Really important for terra! The following maps must be set! for all details. E.g. detail0: diffuse, roughness, metallic else painting is not possible

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Brdf")
		{
			this->setBrdf(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DiffuseColor")
		{
			this->setDiffuseColor(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DiffuseTextureName")
		{
			this->setDiffuseTextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Detail0TextureName")
		{
			this->setDetail0TextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetScale0")
		{
			this->setOffsetScale0(XMLConverter::getAttribVector4(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Detail1TextureName")
		{
			this->setDetail1TextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetScale1")
		{
			this->setOffsetScale1(XMLConverter::getAttribVector4(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Detail2TextureName")
		{
			this->setDetail2TextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetScale2")
		{
			this->setOffsetScale2(XMLConverter::getAttribVector4(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Detail3TextureName")
		{
			this->setDetail3TextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetScale3")
		{
			this->setOffsetScale3(XMLConverter::getAttribVector4(propertyElement, "data"));
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

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Detail0RTextureName")
		{
			this->setDetail0RTextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Roughness0")
		{
			this->setRoughness0(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Detail1RTextureName")
		{
			this->setDetail1RTextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Roughness1")
		{
			this->setRoughness1(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Detail2RTextureName")
		{
			this->setDetail2RTextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Roughness2")
		{
			this->setRoughness2(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Detail3RTextureName")
		{
			this->setDetail3RTextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Roughness3")
		{
			this->setRoughness3(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Detail0MTextureName")
		{
			this->setDetail0MTextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Metalness0")
		{
			this->setMetalness0(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Detail1MTextureName")
		{
			this->setDetail1MTextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Metalness1")
		{
			this->setMetalness1(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Detail2MTextureName")
		{
			this->setDetail2MTextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Metalness2")
		{
			this->setMetalness2(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Detail3MTextureName")
		{
			this->setDetail3MTextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Metalness3")
		{
			this->setMetalness3(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ReflectionTextureName")
		{
			this->setReflectionTextureName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		this->newlyCreated = false;

		return true;
	}

	void DatablockTerraComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		if (this->alreadyCloned)
		{
			Ogre::Terra* terraCopy = this->gameObjectPtr->getMovableObject<Ogre::Terra>();
			auto datablockCopy = this->datablock;
			auto originalDatablockCopy = this->originalDatablock;

			// Nullify members to avoid use-after-free
			this->datablock = nullptr;
			this->originalDatablock = nullptr;

			ENQUEUE_DESTROY_COMMAND("DatablockTerraComponent::onRemoveComponent", _3(terraCopy, datablockCopy, originalDatablockCopy),
			{
				if (terraCopy && originalDatablockCopy)
				{
					terraCopy->setDatablock(originalDatablockCopy);
				}

				if (datablockCopy)
				{
					const auto& linkedRenderables = datablockCopy->getLinkedRenderables();
					if (linkedRenderables.empty())
					{
						datablockCopy->getCreator()->destroyDatablock(datablockCopy->getName());
					}
				}
			});
		}
	}

	GameObjectCompPtr DatablockTerraComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		/*DatablockTerraCompPtr clonedCompPtr(boost::make_shared<DatablockTerraComponent>());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);
		
		clonedCompPtr->setDiffuseColor(this->diffuseColor->getVector3());
		
		clonedCompPtr->setBrdf(this->brdf->getListSelectedValue());
		clonedCompPtr->setDiffuseTextureName(this->diffuseTextureName->getString());
		clonedCompPtr->setDetailWeightTextureName(this->detailWeightTextureName->getString());
		clonedCompPtr->setDetail0TextureName(this->detail0TextureName->getString());
		clonedCompPtr->setDetail1TextureName(this->detail1TextureName->getString());
		clonedCompPtr->setDetail2TextureName(this->detail2TextureName->getString());
		clonedCompPtr->setDetail3TextureName(this->detail3TextureName->getString());
		clonedCompPtr->setDetail0NMTextureName(this->detail0NMTextureName->getString());
		clonedCompPtr->setDetail1NMTextureName(this->detail1NMTextureName->getString());
		clonedCompPtr->setDetail2NMTextureName(this->detail2NMTextureName->getString());
		clonedCompPtr->setDetail3NMTextureName(this->detail3NMTextureName->getString());
		clonedCompPtr->setDetail0RTextureName(this->detail0RTextureName->getString());
		clonedCompPtr->setRoughness0(this->roughness0->getReal());
		clonedCompPtr->setDetail1RTextureName(this->detail1RTextureName->getString());
		clonedCompPtr->setRoughness1(this->roughness1->getReal());
		clonedCompPtr->setDetail2RTextureName(this->detail2RTextureName->getString());
		clonedCompPtr->setRoughness2(this->roughness2->getReal());
		clonedCompPtr->setDetail3RTextureName(this->detail3RTextureName->getString());
		clonedCompPtr->setRoughness3(this->roughness3->getReal());
		clonedCompPtr->setDetail0MTextureName(this->detail0MTextureName->getString());
		clonedCompPtr->setMetalness0(this->metalness0->getReal());
		clonedCompPtr->setDetail1MTextureName(this->detail1MTextureName->getString());
		clonedCompPtr->setMetalness1(this->metalness1->getReal());
		clonedCompPtr->setDetail2MTextureName(this->detail2MTextureName->getString());
		clonedCompPtr->setMetalness2(this->metalness2->getReal());
		clonedCompPtr->setDetail3MTextureName(this->detail3MTextureName->getString());
		clonedCompPtr->setMetalness3(this->metalness3->getReal());
	
		clonedCompPtr->setReflectionTextureName(this->reflectionTextureName->getString());
		
		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;*/
		
		return nullptr;
	}

	bool DatablockTerraComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DatablockTerraComponent] Init datablock terra component for game object: " + this->gameObjectPtr->getName());
		
		this->preReadDatablock();

		return true;
	}

	Ogre::String DatablockTerraComponent::getTerraTextureName(Ogre::HlmsTerraDatablock* datablock, Ogre::TerraTextureTypes type)
	{
		Ogre::TextureGpu* texture = datablock->getTexture(type);
		if (nullptr != texture)
		{
			return texture->getNameStr();
		}
		return "";
	}

	void DatablockTerraComponent::preReadDatablock(void)
	{
		bool success = true;
		
		if (nullptr == this->gameObjectPtr)
			return;

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("DatablockTerraComponent::preReadDatablock", _1(&success),
		{
			auto & terraCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<TerraComponent>());
			if (nullptr != terraCompPtr)
			{
				if (nullptr != terraCompPtr->getTerra())
				{
					success = this->readDatablockTerra(terraCompPtr->getTerra());
				}
			}
			else
			{
				success = this->readDefaultDatablockTerra();
			}
		});

		if (false == success)
		{
			this->datablock = nullptr;
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DatablockTerraComponent] this->datablock reading failed for game object: " + this->gameObjectPtr->getName());
			return;
		}
	}

	bool DatablockTerraComponent::readDatablockTerra(Ogre::Terra* terra)
	{
		// Later create data block with name attribute and save/read as json??

			// If a prior component set this custom data string, with this content, do not clone the this->datablock (see. PlaneComponent)
			if ("doNotCloneDatablock" == this->gameObjectPtr->getCustomDataString())
			{
				this->alreadyCloned = true;
				this->gameObjectPtr->setCustomDataString("");
			}

			// Get the cloned data block, so that it can be manipulated individually
			// All terra cells are sharing one this->datablock!?
			auto it = terra->getTerrainCells().begin();

			this->originalDatablock = dynamic_cast<Ogre::HlmsTerraDatablock*>(it->getDatablock());

			if (false == this->alreadyCloned && nullptr != this->originalDatablock)
			{
				this->datablock = dynamic_cast<Ogre::HlmsTerraDatablock*>(this->originalDatablock->clone(*this->originalDatablock->getNameStr()
					+ Ogre::StringConverter::toString(this->gameObjectPtr->getId())));
				this->alreadyCloned = true;
			}
			else
			{
				this->datablock = dynamic_cast<Ogre::HlmsTerraDatablock*>(it->getDatablock());
			}

			// Note: Terra Pbs is: HLMS_USER3

			/*if (nullptr == this->datablock)
			{
				this->datablock = static_cast<Ogre::HlmsTerraDatablock*>(
						WorkspaceModule::getInstance()->getHlmsManager()->getHlms(Ogre::HLMS_USER3)->createDatablock(this->gameObjectPtr->getName(), this->gameObjectPtr->getName(),
							Ogre::HlmsMacroblock(), Ogre::HlmsBlendblock(), Ogre::HlmsParamVec()));
				it->setDatablock(this->datablock);
			}*/

			if (nullptr == this->datablock)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DatablockTerraComponent] this->datablock reading failed, because there is no data block for game object: "
					+ this->gameObjectPtr->getName());
				return false;
			}

			const Ogre::String* finalDatablockName = this->datablock->getNameStr();

			if (nullptr != finalDatablockName)
			{
				this->gameObjectPtr->actualizeDatablockName(*finalDatablockName, 0);
			}

			// Do not exchange blend weight map, as it has been generated in terra
			terra->setDatablock(this->datablock);

		this->postReadDatablock();

		return true;
	}

	bool DatablockTerraComponent::readDefaultDatablockTerra(void)
	{
		if (nullptr == this->datablock)
		{
			this->datablock = static_cast<Ogre::HlmsTerraDatablock*>(WorkspaceModule::getInstance()->getHlmsManager()->getDatablock("TerraExampleMaterial"));
			this->alreadyCloned = true;

			this->postReadDatablock();
		}
		return true;
	}

	void DatablockTerraComponent::postReadDatablock(void)
	{
		/*
		 enum TerraTextureTypes
		{
			TERRA_DIFFUSE,
			TERRA_DETAIL_WEIGHT,
			TERRA_DETAIL0,
			TERRA_DETAIL1,
			TERRA_DETAIL2,
			TERRA_DETAIL3,
			TERRA_DETAIL0_NM,
			TERRA_DETAIL1_NM,
			TERRA_DETAIL2_NM,
			TERRA_DETAIL3_NM,
			TERRA_DETAIL_ROUGHNESS0,
			TERRA_DETAIL_ROUGHNESS1,
			TERRA_DETAIL_ROUGHNESS2,
			TERRA_DETAIL_ROUGHNESS3,
			TERRA_DETAIL_METALNESS0,
			TERRA_DETAIL_METALNESS1,
			TERRA_DETAIL_METALNESS2,
			TERRA_DETAIL_METALNESS3,
			TERRA_REFLECTION,
			NUM_TERRA_TEXTURE_TYPES
		};
		*/

		this->setBrdf(this->brdf->getListSelectedValue());
		
		this->setDiffuseColor(this->diffuseColor->getVector3());

		this->setDiffuseTextureName(this->diffuseTextureName->getString());

		this->setDetail0TextureName(this->detail0TextureName->getString());

		this->setOffsetScale0(this->offsetScale0->getVector4());

		this->setDetail1TextureName(this->detail1TextureName->getString());

		this->setOffsetScale1(this->offsetScale1->getVector4());

		this->setDetail2TextureName(this->detail2TextureName->getString());

		this->setOffsetScale2(this->offsetScale2->getVector4());

		this->setDetail3TextureName(this->detail3TextureName->getString());

		this->setOffsetScale3(this->offsetScale3->getVector4());

		this->setDetail0NMTextureName(this->detail0NMTextureName->getString());

		this->setDetail1NMTextureName(this->detail1NMTextureName->getString());

		this->setDetail2NMTextureName(this->detail2NMTextureName->getString());

		this->setDetail3NMTextureName(this->detail3NMTextureName->getString());

		this->setDetail0RTextureName(this->detail0RTextureName->getString());
		this->setRoughness0(this->roughness0->getReal());

		this->setDetail1RTextureName(this->detail1RTextureName->getString());
		this->setRoughness1(this->roughness1->getReal());

		this->setDetail2RTextureName(this->detail2RTextureName->getString());
		this->setRoughness2(this->roughness2->getReal());

		this->setDetail3RTextureName(this->detail3RTextureName->getString());
		this->setRoughness3(this->roughness3->getReal());

		this->setDetail0MTextureName(this->detail0MTextureName->getString());
		this->setMetalness0(this->metalness0->getReal());

		this->setDetail1MTextureName(this->detail1MTextureName->getString());
		this->setMetalness1(this->metalness1->getReal());

		this->setDetail2MTextureName(this->detail2MTextureName->getString());
		this->setMetalness2(this->metalness2->getReal());

		this->setDetail3MTextureName(this->detail3MTextureName->getString());
		this->setMetalness3(this->metalness3->getReal());

		this->setReflectionTextureName(this->reflectionTextureName->getString());
	}

	Ogre::String DatablockTerraComponent::mapBrdfToString(Ogre::TerraBrdf::TerraBrdf brdf)
	{
		Ogre::String strBrdf = "Default";
		if (Ogre::TerraBrdf::CookTorrance == brdf)
			strBrdf = "CookTorrance";
		else if (Ogre::TerraBrdf::BlinnPhong == brdf)
			strBrdf = "BlinnPhong";
		else if (Ogre::TerraBrdf::DefaultUncorrelated == brdf)
			strBrdf = "DefaultUncorrelated";
		else if (Ogre::TerraBrdf::DefaultSeparateDiffuseFresnel == brdf)
			strBrdf = "DefaultSeparateDiffuseFresnel";
		else if (Ogre::TerraBrdf::CookTorranceSeparateDiffuseFresnel == brdf)
			strBrdf = "CookTorranceSeparateDiffuseFresnel";
		else if (Ogre::TerraBrdf::BlinnPhongSeparateDiffuseFresnel == brdf)
			strBrdf = "BlinnPhongSeparateDiffuseFresnel";
		else if (Ogre::TerraBrdf::BlinnPhongLegacyMath == brdf)
			strBrdf = "BlinnPhongLegacyMath";
		else if (Ogre::TerraBrdf::BlinnPhongFullLegacy == brdf)
			strBrdf = "BlinnPhongFullLegacy";

		return strBrdf;
	}

	Ogre::TerraBrdf::TerraBrdf DatablockTerraComponent::mapStringToBrdf(const Ogre::String& strBrdf)
	{
		Ogre::TerraBrdf::TerraBrdf brdf = Ogre::TerraBrdf::Default;
		if ("CookTorrance" == strBrdf)
			brdf = Ogre::TerraBrdf::CookTorrance;
		else if ("BlinnPhong" == strBrdf)
			brdf = Ogre::TerraBrdf::BlinnPhong;
		else if ("DefaultUncorrelated" == strBrdf)
			brdf = Ogre::TerraBrdf::DefaultUncorrelated;
		else if ("DefaultSeparateDiffuseFresnel" == strBrdf)
			brdf = Ogre::TerraBrdf::DefaultSeparateDiffuseFresnel;
		else if ("CookTorranceSeparateDiffuseFresnel" == strBrdf)
			brdf = Ogre::TerraBrdf::CookTorranceSeparateDiffuseFresnel;
		else if ("BlinnPhongSeparateDiffuseFresnel" == strBrdf)
			brdf = Ogre::TerraBrdf::BlinnPhongSeparateDiffuseFresnel;
		else if ("BlinnPhongLegacyMath" == strBrdf)
			brdf = Ogre::TerraBrdf::BlinnPhongLegacyMath;
		else if ("BlinnPhongFullLegacy" == strBrdf)
			brdf = Ogre::TerraBrdf::BlinnPhongFullLegacy;

		return brdf;
	}

	Ogre::String DatablockTerraComponent::getClassName(void) const
	{
		return "DatablockTerraComponent";
	}

	Ogre::String DatablockTerraComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void DatablockTerraComponent::update(Ogre::Real dt, bool notSimulating)
	{
		
	}

	void DatablockTerraComponent::actualizeValue(Variant* attribute)
	{
		// Some attribute changed, component is no longer new! So textures can change
		this->newlyCreated = false;

		GameObjectComponent::actualizeValue(attribute);

		if (DatablockTerraComponent::AttrBrdf() == attribute->getName())
		{
			this->setBrdf(attribute->getListSelectedValue());
		}
		if (DatablockTerraComponent::AttrDiffuseColor() == attribute->getName())
		{
			this->setDiffuseColor(attribute->getVector3());
		}
		else if (DatablockTerraComponent::AttrDiffuse() == attribute->getName())
		{
			this->setDiffuseTextureName(attribute->getString());
		}
		else if (DatablockTerraComponent::AttrDetail0() == attribute->getName())
		{
			this->setDetail0TextureName(attribute->getString());
		}
		else if (DatablockTerraComponent::AttrOffsetScale0() == attribute->getName())
		{
			this->setOffsetScale0(attribute->getVector4());
		}
		else if (DatablockTerraComponent::AttrDetail1() == attribute->getName())
		{
			this->setDetail1TextureName(attribute->getString());
		}
		else if (DatablockTerraComponent::AttrOffsetScale1() == attribute->getName())
		{
			this->setOffsetScale1(attribute->getVector4());
		}
		else if (DatablockTerraComponent::AttrDetail2() == attribute->getName())
		{
			this->setDetail2TextureName(attribute->getString());
		}
		else if (DatablockTerraComponent::AttrOffsetScale2() == attribute->getName())
		{
			this->setOffsetScale2(attribute->getVector4());
		}
		else if (DatablockTerraComponent::AttrDetail3() == attribute->getName())
		{
			this->setDetail3TextureName(attribute->getString());
		}
		else if (DatablockTerraComponent::AttrOffsetScale3() == attribute->getName())
		{
			this->setOffsetScale3(attribute->getVector4());
		}
		else if (DatablockTerraComponent::AttrDetail0NM() == attribute->getName())
		{
			this->setDetail0NMTextureName(attribute->getString());
		}
		else if (DatablockTerraComponent::AttrDetail1NM() == attribute->getName())
		{
			this->setDetail1NMTextureName(attribute->getString());
		}
		else if (DatablockTerraComponent::AttrDetail2NM() == attribute->getName())
		{
			this->setDetail2NMTextureName(attribute->getString());
		}
		else if (DatablockTerraComponent::AttrDetail3NM() == attribute->getName())
		{
			this->setDetail3NMTextureName(attribute->getString());
		}
		else if (DatablockTerraComponent::AttrDetail0R() == attribute->getName())
		{
			this->setDetail0RTextureName(attribute->getString());
		}
		else if (DatablockTerraComponent::AttrRoughness0() == attribute->getName())
		{
			this->setRoughness0(attribute->getReal());
		}
		else if (DatablockTerraComponent::AttrDetail1R() == attribute->getName())
		{
			this->setDetail1RTextureName(attribute->getString());
		}
		else if (DatablockTerraComponent::AttrRoughness1() == attribute->getName())
		{
			this->setRoughness1(attribute->getReal());
		}
		else if (DatablockTerraComponent::AttrDetail2R() == attribute->getName())
		{
			this->setDetail2RTextureName(attribute->getString());
		}
		else if (DatablockTerraComponent::AttrRoughness2() == attribute->getName())
		{
			this->setRoughness2(attribute->getReal());
		}
		else if (DatablockTerraComponent::AttrDetail3R() == attribute->getName())
		{
			this->setDetail3RTextureName(attribute->getString());
		}
		else if (DatablockTerraComponent::AttrRoughness3() == attribute->getName())
		{
			this->setRoughness3(attribute->getReal());
		}
		else if (DatablockTerraComponent::AttrDetail0M() == attribute->getName())
		{
			this->setDetail0MTextureName(attribute->getString());
		}
		else if (DatablockTerraComponent::AttrMetalness0() == attribute->getName())
		{
			this->setMetalness0(attribute->getReal());
		}
		else if (DatablockTerraComponent::AttrDetail1M() == attribute->getName())
		{
			this->setDetail1MTextureName(attribute->getString());
		}
		else if (DatablockTerraComponent::AttrMetalness1() == attribute->getName())
		{
			this->setMetalness1(attribute->getReal());
		}
		else if (DatablockTerraComponent::AttrDetail2M() == attribute->getName())
		{
			this->setDetail2MTextureName(attribute->getString());
		}
		else if (DatablockTerraComponent::AttrMetalness2() == attribute->getName())
		{
			this->setMetalness2(attribute->getReal());
		}
		else if (DatablockTerraComponent::AttrDetail3M() == attribute->getName())
		{
			this->setDetail3MTextureName(attribute->getString());
		}
		else if (DatablockTerraComponent::AttrMetalness3() == attribute->getName())
		{
			this->setMetalness3(attribute->getReal());
		}
		else if (DatablockTerraComponent::AttrReflection() == attribute->getName())
		{
			this->setReflectionTextureName(attribute->getString());
		}
	}

	void DatablockTerraComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Brdf"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->brdf->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DiffuseColor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->diffuseColor->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DiffuseTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->diffuseTextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail0TextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->detail0TextureName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetScale0"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->offsetScale0->getVector4())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail1TextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->detail1TextureName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetScale1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->offsetScale1->getVector4())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail2TextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->detail2TextureName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetScale2"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->offsetScale2->getVector4())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail3TextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->detail3TextureName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetScale3"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->offsetScale3->getVector4())));
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail0RTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->detail0RTextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Roughness0"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roughness0->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail1RTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->detail1RTextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Roughness1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roughness1->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail2RTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->detail2RTextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Roughness2"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roughness2->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail3RTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->detail3RTextureName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Roughness3"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roughness3->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail0MTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->detail0MTextureName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Metalness0"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->metalness0->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail1MTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->detail1MTextureName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Metalness1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->metalness1->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail2MTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->detail2MTextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Metalness2"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->metalness2->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Detail3MTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->detail3MTextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Metalness3"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->metalness3->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ReflectionTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->reflectionTextureName->getString())));
		propertiesXML->append_node(propertyXML);
	}

	void DatablockTerraComponent::internalSetTextureName(Ogre::TerraTextureTypes terraTextureType, Ogre::CommonTextureTypes::CommonTextureTypes textureType, Variant* attribute, const Ogre::String& textureName)
	{
		ENQUEUE_RENDER_COMMAND_MULTI("DatablockTerraComponent::internalSetTextureName", _4(terraTextureType, textureType, attribute, textureName),
		{
			Ogre::TextureGpuManager * hlmsTextureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();

			Ogre::String previousTextureName = attribute->getString();
			Ogre::String tempTextureName = textureName;

			// If the data block component has just been created, get texture name from existing data block
			if (true == newlyCreated && nullptr != this->datablock)
			{
				tempTextureName = this->getTerraTextureName(this->datablock, terraTextureType);
			}

			attribute->setValue(tempTextureName);
			this->addAttributeFilePathData(attribute);

			// Store the old texture name as user data
			if (false == tempTextureName.empty())
			{
				// attribute->addUserData(tempTextureName);
				attribute->addUserData("OldTexture", tempTextureName);
			}
			else if (nullptr != this->datablock)
			{
				attribute->addUserData("OldTexture", this->getTerraTextureName(this->datablock, terraTextureType));
			}

			if (nullptr != this->datablock/* && false == newlyCreated*/)
			{
				// If no texture has been loaded, but still the data block has an internal one, get this one and remove it manually!
				if (true == attribute->getUserDataValue("OldTexture").empty())
				{
					tempTextureName = this->getTerraTextureName(this->datablock, terraTextureType);
					attribute->addUserData(tempTextureName);
					this->addAttributeFilePathData(attribute);
				}

				// createOrRetrieveTexture crashes, when texture alias name is empty
				Ogre::String oldTextureName = attribute->getUserDataValue("OldTexture");
				if (false == oldTextureName.empty())
				{
					if (false == Ogre::ResourceGroupManager::getSingletonPtr()->resourceExistsInAnyGroup(oldTextureName))
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DatablockTerraComponent] Cannot set texture: '" + oldTextureName +
							"', because it does not exist in any resource group! For game object: " + this->gameObjectPtr->getName());
						attribute->setValue(previousTextureName);
						this->addAttributeFilePathData(attribute);
						return;
					}

					Ogre::uint32 textureFlags = 0;

					if (datablock->suggestUsingSRGB(terraTextureType))
					{
						textureFlags |= Ogre::TextureFlags::PrefersLoadingFromFileAsSRGB;
						// textureFlags |= Ogre::TextureFlags::AutomaticBatching;
						// textureFlags |= Ogre::TextureFlags::ManualTexture;
					}

					if (terraTextureType >= Ogre::TERRA_DETAIL0_NM && terraTextureType <= Ogre::TERRA_DETAIL_METALNESS3)
					{
						textureFlags |= Ogre::TextureFlags::AutomaticBatching;
					}

					Ogre::TextureTypes::TextureTypes internalTextureType = Ogre::TextureTypes::Type2D;
					if (terraTextureType == Ogre::TERRA_REFLECTION)
					{
						internalTextureType = Ogre::TextureTypes::TypeCube;
						textureFlags &= ~Ogre::TextureFlags::AutomaticBatching;
					}

					Ogre::uint32 filters = Ogre::TextureFilter::FilterTypes::TypeGenerateDefaultMipmaps;
					filters |= datablock->suggestFiltersForType(terraTextureType);

					// Can never be found^^
					/*const Ogre::String* foundTextureName = hlmsTextureManager->findResourceGroupStr(oldTextureName);
					if (nullptr == foundTextureName)
					{
						Ogre::LogManager::getSingleton().logMessage("[DatablockTerraComponent] Error: Could not find texture: " + oldTextureName, Ogre::LML_CRITICAL);
						attribute->setValue(previousTextureName);
						return;
					}*/

					// Can never be found^^
					/*Ogre::TextureGpu* foundTexture = hlmsTextureManager->findTextureNoThrow(oldTextureName);
					if (nullptr == foundTexture)
					{
						Ogre::LogManager::getSingleton().logMessage("[DatablockTerraComponent] Error: Could not find texture: " + oldTextureName, Ogre::LML_CRITICAL);
						attribute->setValue(previousTextureName);
						return;
					}*/

					// Really important: createOrRetrieveTexture when its created, its width/height is 0 etc. so the texture is just prepared
					// it will be filled with correct data when terra->setDataBlock is called, see internal function: itor->setDatablock( datablock );
					Ogre::TextureGpu* texture = hlmsTextureManager->createOrRetrieveTexture(oldTextureName,
						oldTextureName, Ogre::GpuPageOutStrategy::SaveToSystemRam, textureFlags, internalTextureType,
						Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, filters);

					// Ogre::Image2 image;
					// image.load(oldTextureName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

					// Ogre::LogManager::getSingleton().logMessage("[DatablockTerraComponent] Texture: " + oldTextureName + " size: " + Ogre::StringConverter::toString(image.getSizeBytes()), Ogre::LML_CRITICAL);

					if (nullptr != texture)
					{
						try
						{
							texture->scheduleTransitionTo(Ogre::GpuResidency::Resident);
						}
						catch (const Ogre::Exception& exception)
						{
							Ogre::LogManager::getSingleton().logMessage(exception.getFullDescription(), Ogre::LML_CRITICAL);
							Ogre::LogManager::getSingleton().logMessage("[DatablockPbsComponent] Error: Could not set texture: '" + oldTextureName + "' For game object: " + this->gameObjectPtr->getName(), Ogre::LML_CRITICAL);
							attribute->setValue(previousTextureName);
							this->addAttributeFilePathData(attribute);
							return;
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

						Ogre::HlmsSamplerblock samplerblock(*datablock->getSamplerblock(terraTextureType));
						if (terraTextureType >= Ogre::TERRA_DETAIL0 && terraTextureType <= Ogre::TERRA_DETAIL_METALNESS3)
						{
							//Detail maps default to wrap mode.
							samplerblock.mU = Ogre::TAM_WRAP;
							samplerblock.mV = Ogre::TAM_WRAP;
							samplerblock.mW = Ogre::TAM_WRAP;

							samplerblock.mMinFilter = Ogre::FO_ANISOTROPIC;
							samplerblock.mMagFilter = Ogre::FO_ANISOTROPIC;
							samplerblock.mMipFilter = Ogre::FO_ANISOTROPIC;
							samplerblock.mMaxAnisotropy = 1; // test also with -1;
						}

						datablock->setTexture(terraTextureType, texture, &samplerblock);
					}
					else
					{
						attribute->setValue("");
						attribute->removeUserData("OldTexture");
					}
				}
			}
		});
	}

	void DatablockTerraComponent::setBrdf(const Ogre::String& brdf)
	{
		this->brdf->setListSelectedValue(brdf);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
			{
				ENQUEUE_RENDER_COMMAND_MULTI("DatablockTerraComponent::setWorkflow", _1(brdf),
				{
					this->datablock->setBrdf(this->mapStringToBrdf(brdf));
				});
			}
			else
			{
				this->brdf->setListSelectedValue(this->mapBrdfToString(static_cast<Ogre::TerraBrdf::TerraBrdf>(this->datablock->getBrdf())));
			}
		}
	}

	Ogre::String DatablockTerraComponent::getBrdf(void) const
	{
		return this->brdf->getListSelectedValue();
	}

	void DatablockTerraComponent::setDiffuseColor(const Ogre::Vector3& diffuseColor)
	{
		this->diffuseColor->setValue(diffuseColor);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
			{
				ENQUEUE_RENDER_COMMAND_MULTI("DatablockTerraComponent::setDiffuseColor", _1(diffuseColor),
				{
					this->datablock->setDiffuse(diffuseColor);
				});
			}
			else
			{
				this->diffuseColor->setValue(this->datablock->getDiffuse());
			}
		}
	}

	Ogre::Vector3 DatablockTerraComponent::getDiffuseColor(void) const
	{
		return this->diffuseColor->getVector3();
	}

	void DatablockTerraComponent::setDiffuseTextureName(const Ogre::String& diffuseTextureName)
	{
		this->internalSetTextureName(Ogre::TerraTextureTypes::TERRA_DIFFUSE, Ogre::CommonTextureTypes::Diffuse, this->diffuseTextureName, diffuseTextureName);
	}

	const Ogre::String DatablockTerraComponent::getDiffuseTextureName(void) const
	{
		return this->diffuseTextureName->getString();
	}
	
	void DatablockTerraComponent::setDetail0TextureName(const Ogre::String& detail0TextureName)
	{
		this->internalSetTextureName(Ogre::TerraTextureTypes::TERRA_DETAIL0, Ogre::CommonTextureTypes::Diffuse, this->detail0TextureName, detail0TextureName);
	}
	
	const Ogre::String DatablockTerraComponent::getDetail0TextureName(void) const
	{
		return this->detail0TextureName->getString();
	}

	void DatablockTerraComponent::setOffsetScale0(const Ogre::Vector4& offsetScale0)
	{
		this->offsetScale0->setValue(offsetScale0);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
			{
				ENQUEUE_RENDER_COMMAND_MULTI("DatablockTerraComponent::setOffsetScale0", _1(offsetScale0),
				{
					this->datablock->setDetailMapOffsetScale(0, offsetScale0);
				});
			}
			else
			{
				this->offsetScale0->setValue(this->datablock->getDetailMapOffsetScale(0));
			}
		}
	}

	Ogre::Vector4 DatablockTerraComponent::getOffsetScale0(void) const
	{
		return this->offsetScale0->getVector4();
	}
	
	void DatablockTerraComponent::setDetail1TextureName(const Ogre::String& detail1TextureName)
	{
		this->internalSetTextureName(Ogre::TerraTextureTypes::TERRA_DETAIL1, Ogre::CommonTextureTypes::Diffuse, this->detail1TextureName, detail1TextureName);
	}
	
	const Ogre::String DatablockTerraComponent::getDetail1TextureName(void) const
	{
		return this->detail1TextureName->getString();
	}

	void DatablockTerraComponent::setOffsetScale1(const Ogre::Vector4& offsetScale1)
	{
		this->offsetScale1->setValue(offsetScale1);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
			{
				ENQUEUE_RENDER_COMMAND_MULTI("DatablockTerraComponent::setOffsetScale1", _1(offsetScale1),
				{
					this->datablock->setDetailMapOffsetScale(1, offsetScale1);
				});
			}
			else
			{
				this->offsetScale1->setValue(this->datablock->getDetailMapOffsetScale(1));
			}
		}
	}

	Ogre::Vector4 DatablockTerraComponent::getOffsetScale1(void) const
	{
		return this->offsetScale1->getVector4();
	}
	
	void DatablockTerraComponent::setDetail2TextureName(const Ogre::String& detail2TextureName)
	{
		this->internalSetTextureName(Ogre::TerraTextureTypes::TERRA_DETAIL2, Ogre::CommonTextureTypes::Diffuse, this->detail2TextureName, detail2TextureName);
	}
	
	const Ogre::String DatablockTerraComponent::getDetail2TextureName(void) const
	{
		return this->detail2TextureName->getString();
	}

	void DatablockTerraComponent::setOffsetScale2(const Ogre::Vector4& offsetScale2)
	{
		this->offsetScale2->setValue(offsetScale2);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
			{
				ENQUEUE_RENDER_COMMAND_MULTI("DatablockTerraComponent::setOffsetScale2", _1(offsetScale2),
				{
					this->datablock->setDetailMapOffsetScale(2, offsetScale2);
				});
			}
			else
			{
				this->offsetScale2->setValue(this->datablock->getDetailMapOffsetScale(2));
			}
		}
	}

	Ogre::Vector4 DatablockTerraComponent::getOffsetScale2(void) const
	{
		return this->offsetScale2->getVector4();
	}
	
	void DatablockTerraComponent::setDetail3TextureName(const Ogre::String& detail3TextureName)
	{
		this->internalSetTextureName(Ogre::TerraTextureTypes::TERRA_DETAIL3, Ogre::CommonTextureTypes::Diffuse, this->detail3TextureName, detail3TextureName);
	}
	
	const Ogre::String DatablockTerraComponent::getDetail3TextureName(void) const
	{
		return this->detail3TextureName->getString();
	}

	void DatablockTerraComponent::setOffsetScale3(const Ogre::Vector4& offsetScale3)
	{
		this->offsetScale3->setValue(offsetScale3);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
			{
				ENQUEUE_RENDER_COMMAND_MULTI("DatablockTerraComponent::setOffsetScale3", _1(offsetScale3),
					{
						this->datablock->setDetailMapOffsetScale(3, offsetScale3);
					});
			}
			else
			{
				this->offsetScale3->setValue(this->datablock->getDetailMapOffsetScale(3));
			}
		}
	}

	Ogre::Vector4 DatablockTerraComponent::getOffsetScale3(void) const
	{
		return this->offsetScale3->getVector4();
	}
	
	void DatablockTerraComponent::setDetail0NMTextureName(const Ogre::String& detail0NMTextureName)
	{
		this->internalSetTextureName(Ogre::TerraTextureTypes::TERRA_DETAIL0_NM, Ogre::CommonTextureTypes::NormalMap, this->detail0NMTextureName, detail0NMTextureName);
	}
	
	const Ogre::String DatablockTerraComponent::getDetail0NMTextureName(void) const
	{
		return this->detail0NMTextureName->getString();
	}
	
	void DatablockTerraComponent::setDetail1NMTextureName(const Ogre::String& detail1NMTextureName)
	{
		this->internalSetTextureName(Ogre::TerraTextureTypes::TERRA_DETAIL1_NM, Ogre::CommonTextureTypes::NormalMap, this->detail1NMTextureName, detail1NMTextureName);
	}
	
	const Ogre::String DatablockTerraComponent::getDetail1NMTextureName(void) const
	{
		return this->detail1NMTextureName->getString();
	}
	
	void DatablockTerraComponent::setDetail2NMTextureName(const Ogre::String& detail2NMTextureName)
	{
		this->internalSetTextureName(Ogre::TerraTextureTypes::TERRA_DETAIL2_NM, Ogre::CommonTextureTypes::NormalMap, this->detail2NMTextureName, detail2NMTextureName);
	}
	
	const Ogre::String DatablockTerraComponent::getDetail2NMTextureName(void) const
	{
		return this->detail2NMTextureName->getString();
	}

	void DatablockTerraComponent::setDetail3NMTextureName(const Ogre::String& detail3NMTextureName)
	{
		this->internalSetTextureName(Ogre::TerraTextureTypes::TERRA_DETAIL3_NM, Ogre::CommonTextureTypes::NormalMap, this->detail3NMTextureName, detail3NMTextureName);
	}
	
	const Ogre::String DatablockTerraComponent::getDetail3NMTextureName(void) const
	{
		return this->detail3NMTextureName->getString();
	}
	
	void DatablockTerraComponent::setDetail0RTextureName(const Ogre::String& detail0RTextureName)
	{
		// Texture must not be empty, else painting on terra is not possible
		Ogre::String textureName = detail0RTextureName;
		if (false == this->newlyCreated && true == textureName.empty())
		{
			textureName = this->detail0TextureName->getString();
		}

		this->internalSetTextureName(Ogre::TerraTextureTypes::TERRA_DETAIL_ROUGHNESS0, Ogre::CommonTextureTypes::Monochrome, this->detail0RTextureName, textureName);
	}
	
	const Ogre::String DatablockTerraComponent::getDetail0RTextureName(void) const
	{
		return this->detail0RTextureName->getString();
	}
	
	void DatablockTerraComponent::setRoughness0(Ogre::Real roughness0)
	{
		this->roughness0->setValue(roughness0);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
			{
				ENQUEUE_RENDER_COMMAND_MULTI("DatablockPbsComponent::setRoughness0", _1(roughness0),
				{
					this->datablock->setRoughness(0, roughness0);
				});
			}
			else
			{
				this->roughness0->setValue(this->datablock->getRoughness(0));
			}
		}
	}

	Ogre::Real DatablockTerraComponent::getRoughness0(void) const
	{
		return this->roughness0->getReal();
	}
	
	void DatablockTerraComponent::setDetail1RTextureName(const Ogre::String& detail1RTextureName)
	{
		// Texture must not be empty, else painting on terra is not possible
		Ogre::String textureName = detail1RTextureName;
		if (false == this->newlyCreated && true == textureName.empty())
		{
			textureName = this->detail1TextureName->getString();
		}

		this->internalSetTextureName(Ogre::TerraTextureTypes::TERRA_DETAIL_ROUGHNESS1, Ogre::CommonTextureTypes::Monochrome, this->detail1RTextureName, textureName);
	}

	const Ogre::String DatablockTerraComponent::getDetail1RTextureName(void) const
	{
		return this->detail1RTextureName->getString();
	}
	
	void DatablockTerraComponent::setRoughness1(Ogre::Real roughness1)
	{
		this->roughness1->setValue(roughness1);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
			{
				ENQUEUE_RENDER_COMMAND_MULTI("DatablockPbsComponent::setRoughness1", _1(roughness1),
				{
					this->datablock->setRoughness(1, roughness1);
				});
			}
			else
			{
				this->roughness1->setValue(this->datablock->getRoughness(1));
			}
		}
	}

	Ogre::Real DatablockTerraComponent::getRoughness1(void) const
	{
		return this->roughness1->getReal();
	}
	
	void DatablockTerraComponent::setDetail2RTextureName(const Ogre::String& detail2RTextureName)
	{
		// Texture must not be empty, else painting on terra is not possible
		Ogre::String textureName = detail2RTextureName;
		if (false == this->newlyCreated && true == textureName.empty())
		{
			textureName = this->detail2TextureName->getString();
		}

		this->internalSetTextureName(Ogre::TerraTextureTypes::TERRA_DETAIL_ROUGHNESS2, Ogre::CommonTextureTypes::Monochrome, this->detail2RTextureName, textureName);
	}
	
	const Ogre::String DatablockTerraComponent::getDetail2RTextureName(void) const
	{
		return this->detail2RTextureName->getString();
	}
	
	void DatablockTerraComponent::setRoughness2(Ogre::Real roughness2)
	{
		this->roughness2->setValue(roughness2);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
			{
				ENQUEUE_RENDER_COMMAND_MULTI("DatablockPbsComponent::setRoughness2", _1(roughness2),
				{
					this->datablock->setRoughness(2, roughness2);
				});
			}
			else
			{
				this->roughness2->setValue(this->datablock->getRoughness(2));
			}
		}
	}

	Ogre::Real DatablockTerraComponent::getRoughness2(void) const
	{
		return this->roughness2->getReal();
	}
	
	void DatablockTerraComponent::setDetail3RTextureName(const Ogre::String& detail3RTextureName)
	{
		// Texture must not be empty, else painting on terra is not possible
		Ogre::String textureName = detail3RTextureName;
		if (false == this->newlyCreated && true == textureName.empty())
		{
			textureName = this->detail3TextureName->getString();
		}

		this->internalSetTextureName(Ogre::TerraTextureTypes::TERRA_DETAIL_ROUGHNESS3, Ogre::CommonTextureTypes::Monochrome, this->detail3RTextureName, textureName);
	}
	
	const Ogre::String DatablockTerraComponent::getDetail3RTextureName(void) const
	{
		return this->detail3RTextureName->getString();
	}
	
	void DatablockTerraComponent::setRoughness3(Ogre::Real roughness3)
	{
		this->roughness3->setValue(roughness3);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
			{
				ENQUEUE_RENDER_COMMAND_MULTI("DatablockPbsComponent::setRoughness3", _1(roughness3),
				{
					this->datablock->setRoughness(3, roughness3);
				});
			}
			else
			{
				this->roughness3->setValue(this->datablock->getRoughness(3));
			}
		}
	}

	Ogre::Real DatablockTerraComponent::getRoughness3(void) const
	{
		return this->roughness3->getReal();
	}
	
	void DatablockTerraComponent::setDetail0MTextureName(const Ogre::String& detail0MTextureName)
	{
		// Texture must not be empty, else painting on terra is not possible
		Ogre::String textureName = detail0MTextureName;
		if (false == this->newlyCreated && true == textureName.empty())
		{
			textureName = this->detail0TextureName->getString();
		}

		this->internalSetTextureName(Ogre::TerraTextureTypes::TERRA_DETAIL_METALNESS0, Ogre::CommonTextureTypes::Monochrome, this->detail0MTextureName, textureName);
	}
	
	const Ogre::String DatablockTerraComponent::getDetail0MTextureName(void) const
	{
		return this->detail0MTextureName->getString();
	}
	
	void DatablockTerraComponent::setMetalness0(Ogre::Real metalness0)
	{
		this->metalness0->setValue(metalness0);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
			{
				ENQUEUE_RENDER_COMMAND_MULTI("DatablockPbsComponent::setMetalness0", _1(metalness0),
				{
					this->datablock->setMetalness(0, metalness0);
				});
			}
			else
			{
				this->metalness0->setValue(this->datablock->getMetalness(0));
			}
		}
	}

	Ogre::Real DatablockTerraComponent::getMetalness0(void) const
	{
		return this->metalness0->getReal();
	}
	
	void DatablockTerraComponent::setDetail1MTextureName(const Ogre::String& detail1MTextureName)
	{
		// Texture must not be empty, else painting on terra is not possible
		Ogre::String textureName = detail1MTextureName;
		if (false == this->newlyCreated && true == textureName.empty())
		{
			textureName = this->detail1TextureName->getString();
		}

		this->internalSetTextureName(Ogre::TerraTextureTypes::TERRA_DETAIL_METALNESS1, Ogre::CommonTextureTypes::Monochrome, this->detail1MTextureName, textureName);
	}
	
	const Ogre::String DatablockTerraComponent::getDetail1MTextureName(void) const
	{
		return this->detail1MTextureName->getString();
	}
	
	void DatablockTerraComponent::setMetalness1(Ogre::Real metalness1)
	{
		this->metalness1->setValue(metalness1);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
			{
				ENQUEUE_RENDER_COMMAND_MULTI("DatablockPbsComponent::setMetalness1", _1(metalness1),
				{
					this->datablock->setMetalness(1, metalness1);
				});
			}
			else
			{
				this->metalness1->setValue(this->datablock->getMetalness(1));
			}
		}
	}

	Ogre::Real DatablockTerraComponent::getMetalness1(void) const
	{
		return this->metalness1->getReal();
	}
	
	void DatablockTerraComponent::setDetail2MTextureName(const Ogre::String& detail2MTextureName)
	{
		// Texture must not be empty, else painting on terra is not possible
		Ogre::String textureName = detail2MTextureName;
		if (false == this->newlyCreated && true == textureName.empty())
		{
			textureName = this->detail2TextureName->getString();
		}

		this->internalSetTextureName(Ogre::TerraTextureTypes::TERRA_DETAIL_METALNESS2, Ogre::CommonTextureTypes::Monochrome, this->detail2MTextureName, textureName);
	}
	
	const Ogre::String DatablockTerraComponent::getDetail2MTextureName(void) const
	{
		return this->detail2MTextureName->getString();
	}
	
	void DatablockTerraComponent::setMetalness2(Ogre::Real metalness2)
	{
		this->metalness2->setValue(metalness2);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
			{
				ENQUEUE_RENDER_COMMAND_MULTI("DatablockPbsComponent::setMetalness2", _1(metalness2),
				{
					this->datablock->setMetalness(2, metalness2);
				});
			}
			else
			{
				this->metalness2->setValue(this->datablock->getMetalness(2));
			}
		}
	}

	Ogre::Real DatablockTerraComponent::getMetalness2(void) const
	{
		return this->metalness2->getReal();
	}
	
	void DatablockTerraComponent::setDetail3MTextureName(const Ogre::String& detail3MTextureName)
	{
		// Texture must not be empty, else painting on terra is not possible
		Ogre::String textureName = detail3MTextureName;
		if (false == this->newlyCreated && true == textureName.empty())
		{
			textureName = this->detail3TextureName->getString();
		}

		this->internalSetTextureName(Ogre::TerraTextureTypes::TERRA_DETAIL_METALNESS3, Ogre::CommonTextureTypes::Monochrome, this->detail3MTextureName, textureName);
	}
	
	const Ogre::String DatablockTerraComponent::getDetail3MTextureName(void) const
	{
		return this->detail3MTextureName->getString();
	}
	
	void DatablockTerraComponent::setMetalness3(Ogre::Real metalness3)
	{
		this->metalness3->setValue(metalness3);
		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
			{
				ENQUEUE_RENDER_COMMAND_MULTI("DatablockPbsComponent::setMetalness3", _1(metalness3),
				{
					this->datablock->setMetalness(3, metalness3);
				});
			}
			else
			{
				this->metalness3->setValue(this->datablock->getMetalness(3));
			}
		}
	}

	Ogre::Real DatablockTerraComponent::getMetalness3(void) const
	{
		return this->metalness3->getReal();
	}

	void DatablockTerraComponent::setReflectionTextureName(const Ogre::String& reflectionTextureName)
	{
		this->internalSetTextureName(Ogre::TerraTextureTypes::TERRA_REFLECTION, Ogre::CommonTextureTypes::EnvMap, this->reflectionTextureName, reflectionTextureName);
	}

	const Ogre::String DatablockTerraComponent::getReflectionTextureName(void) const
	{
		return this->reflectionTextureName->getString();
	}

	Ogre::HlmsTerraDatablock* DatablockTerraComponent::getDataBlock(void) const
	{
		return this->datablock;
	}

	void DatablockTerraComponent::doNotCloneDataBlock(void)
	{
		this->alreadyCloned = true;
	}

}; // namespace end