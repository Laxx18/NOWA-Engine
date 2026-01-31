#include "NOWAPrecompiled.h"
#include "DatablockUnlitComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "modules/DeployResourceModule.h"
#include "main/AppStateManager.h"

namespace
{
	Ogre::String getResourceFilePathName(const Ogre::String& resourceName)
	{
		if (true == resourceName.empty())
		{
			return "";
		}

		Ogre::Archive* archive = nullptr;
		try
		{
			Ogre::String resourceGroupName = Ogre::ResourceGroupManager::getSingletonPtr()->findGroupContainingResource(resourceName);
			archive = Ogre::ResourceGroupManager::getSingletonPtr()->_getArchiveToResource(resourceName, resourceGroupName);
		}
		catch (...)
		{
			return "";
		}

		if (nullptr != archive)
		{
			return archive->getName();
		}
		return "";
	}

	Ogre::String getDirectoryNameFromFilePathName(const Ogre::String& filePathName)
	{
		size_t pos = filePathName.find_last_of("\\/");
		return (std::string::npos == pos) ? "" : filePathName.substr(0, pos);
	}
}

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	DatablockUnlitComponent::DatablockUnlitComponent()
		: GameObjectComponent(),
		subEntityIndex(new Variant(DatablockUnlitComponent::AttrSubEntityIndex(), static_cast<unsigned int>(0), this->attributes)),
		useColor(new Variant(DatablockUnlitComponent::AttrUseColor(), true, this->attributes)),
		backgroundColor(new Variant(DatablockUnlitComponent::AttrBackgroundColor(), Ogre::Vector4(1.0f, 1.0f, 1.0f, 1.0f), this->attributes)),
		textureCount(new Variant(DatablockUnlitComponent::AttrTextureCount(), 0, this->attributes)),
		datablock(nullptr),
		alreadyCloned(false),
		isCloned(false),
		newlyCreated(true),
		originalDatablock(nullptr),
		oldSubIndex(0)
	{
		this->detailTextureNames.resize(this->textureCount->getUInt());
		this->blendModes.resize(this->textureCount->getUInt());
		this->textureSwizzles.resize(this->textureCount->getUInt());
		this->useAnimations.resize(this->textureCount->getUInt());
		this->animationTranslations.resize(this->textureCount->getUInt());
		this->animationScales.resize(this->textureCount->getUInt());
		this->animationRotations.resize(this->textureCount->getUInt());

		// Texture tiling approach!
		// https://forums.ogre3d.org/viewtopic.php?t=97074

		this->textureCount->addUserData(GameObject::AttrActionNeedRefresh());
		this->subEntityIndex->setDescription("Specifies the sub entity index, for which the datablock should be shown.");
		this->subEntityIndex->addUserData(GameObject::AttrActionNeedRefresh());
		this->useColor->addUserData(GameObject::AttrActionNeedRefresh());
		this->backgroundColor->addUserData(GameObject::AttrActionColorDialog());
	}

	DatablockUnlitComponent::~DatablockUnlitComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DatablockUnlitComponent] Destructor datablock unlit component for game object: " + this->gameObjectPtr->getName());
	}

	void DatablockUnlitComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		if (this->alreadyCloned)
		{
			// Make safe copies of pointers & index
			Ogre::v1::Entity* entityCopy = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
			Ogre::Item* itemCopy = this->gameObjectPtr->getMovableObject<Ogre::Item>();
			auto datablockCopy = this->datablock;
			auto originalDatablockCopy = this->originalDatablock;
			size_t oldSubIndexCopy = this->oldSubIndex;

			// Nullify members immediately to avoid use-after-free
			this->datablock = nullptr;
			this->originalDatablock = nullptr;

			NOWA::GraphicsModule::RenderCommand renderCommand = [entityCopy, itemCopy, datablockCopy, originalDatablockCopy, oldSubIndexCopy]()
			{
				// Safely reset datablock for entity or item
				if (entityCopy && originalDatablockCopy)
				{
					if (oldSubIndexCopy < entityCopy->getNumSubEntities())
					{
						entityCopy->getSubEntity(oldSubIndexCopy)->setDatablock(originalDatablockCopy);
					}
				}
				else if (itemCopy && originalDatablockCopy)
				{
					if (oldSubIndexCopy < itemCopy->getNumSubItems())
					{
						itemCopy->getSubItem(oldSubIndexCopy)->setDatablock(originalDatablockCopy);
					}
				}

				// Destroy datablock only if no linked renderables remain
				if (datablockCopy)
				{
					const auto& linkedRenderables = datablockCopy->getLinkedRenderables();
					if (linkedRenderables.empty())
					{
						datablockCopy->getCreator()->destroyDatablock(datablockCopy->getName());
					}
				}
			};
			NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "DatablockUnlitComponent::onRemoveComponent");
		}

		this->gameObjectPtr->actualizeDatablocks();
	}

	bool DatablockUnlitComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SubEntityIndex")
		{
			this->subEntityIndex->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseColor")
		{
			this->setUseColor(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BackgroundColor")
		{
			this->setBackgroundColor(XMLConverter::getAttribVector4(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TextureCount")
		{
			this->textureCount->setValue(XMLConverter::getAttribInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (this->detailTextureNames.size() < this->textureCount->getUInt())
		{
			this->detailTextureNames.resize(this->textureCount->getUInt());
			this->blendModes.resize(this->textureCount->getUInt());
			this->textureSwizzles.resize(this->textureCount->getUInt());
			this->useAnimations.resize(this->textureCount->getUInt());
			this->animationTranslations.resize(this->textureCount->getUInt());
			this->animationScales.resize(this->textureCount->getUInt());
			this->animationRotations.resize(this->textureCount->getUInt());
		}

		for (size_t i = 0; i < this->textureCount->getUInt(); i++)
		{
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DetailTextureName" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->detailTextureNames[i])
				{
					this->detailTextureNames[i] = new Variant(DatablockUnlitComponent::AttrDetailTexture() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttrib(propertyElement, "data"), this->attributes);
					this->detailTextureNames[i]->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
				}
				else
				{
					this->detailTextureNames[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
					this->detailTextureNames[i]->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BlendMode" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->blendModes[i])
				{
					this->blendModes[i] = new Variant(DatablockUnlitComponent::AttrBlendMode() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttrib(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->blendModes[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TextureSwizzle" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->textureSwizzles[i])
				{
					this->textureSwizzles[i] = new Variant(DatablockUnlitComponent::AttrTextureSwizzle() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribVector4(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->textureSwizzles[i]->setValue(XMLConverter::getAttribVector4(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseAnimation" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->useAnimations[i])
				{
					this->useAnimations[i] = new Variant(DatablockUnlitComponent::AttrUseAnimation() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribBool(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->useAnimations[i]->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimationTranslation" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->animationTranslations[i])
				{
					this->animationTranslations[i] = new Variant(DatablockUnlitComponent::AttrAnimationTranslation() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribVector3(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->animationTranslations[i]->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimationScale" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->animationScales[i])
				{
					this->animationScales[i] = new Variant(DatablockUnlitComponent::AttrAnimationScale() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribVector3(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->animationScales[i]->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimationRotation" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->animationRotations[i])
				{
					this->animationRotations[i] = new Variant(DatablockUnlitComponent::AttrAnimationRotation() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribVector3(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->animationRotations[i]->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
		}

		// Some attribute changed, component is no longer new! So textures can change
		this->newlyCreated = false;

		return true;
	}

	GameObjectCompPtr DatablockUnlitComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		DatablockUnlitCompPtr clonedCompPtr(boost::make_shared<DatablockUnlitComponent>());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setSubEntityIndex(this->subEntityIndex->getUInt());
		clonedCompPtr->setUseColor(this->useColor->getBool());
		clonedCompPtr->setBackgroundColor(this->backgroundColor->getVector4());
		clonedCompPtr->setTextureCount(this->textureCount->getUInt());

		for (unsigned int i = 0; i < this->textureCount->getUInt(); i++)
		{
			clonedCompPtr->setDetailTextureName(i, this->detailTextureNames[i]->getString());
			clonedCompPtr->setBlendMode(i, this->blendModes[i]->getListSelectedValue());
			clonedCompPtr->setTextureSwizzle(i, this->textureSwizzles[i]->getVector4());
			clonedCompPtr->setUseAnimation(i, this->useAnimations[i]->getBool());
			clonedCompPtr->setAnimationTranslation(i, this->animationTranslations[i]->getVector3());
			clonedCompPtr->setAnimationScale(i, this->animationScales[i]->getVector3());
			clonedCompPtr->setAnimationRotation(i, this->animationRotations[i]->getVector3());
		}

		clonedCompPtr->isCloned = true;
		
		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool DatablockUnlitComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DatablockUnlitComponent] Init datablock unlit component for game object: " + this->gameObjectPtr->getName());
		
		this->preReadDatablock();

		return true;
	}

	Ogre::String DatablockUnlitComponent::getUnlitTextureName(Ogre::HlmsUnlitDatablock* datablock, unsigned char textureIndex)
	{
		Ogre::TextureGpu* texture = datablock->getTexture(textureIndex);
		if (nullptr != texture)
		{
			return texture->getNameStr();
		}
		return "";
	}

	void DatablockUnlitComponent::internalSetTextureName(unsigned char textureIndex, Ogre::CommonTextureTypes::CommonTextureTypes textureType, Variant* attribute, const Ogre::String& textureName)
	{
		ENQUEUE_RENDER_COMMAND_MULTI("DatablockUnlitComponent::internalSetTextureName", _4(textureIndex, textureType, attribute, textureName),
		{
			Ogre::TextureGpuManager * hlmsTextureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();

			Ogre::String previousTextureName = attribute->getString();
			// If the data block component has just been created, get texture name from existing data block
			Ogre::String tempTextureName = textureName;

			// If the data block component has just been created, get texture name from existing data block
			if (true == newlyCreated && nullptr != this->datablock)
			{
				tempTextureName = this->getUnlitTextureName(this->datablock, textureType);
			}
			attribute->setValue(tempTextureName);
			this->addAttributeFilePathData(attribute);

			// Store the old texture name as user data
			if (false == tempTextureName.empty())
			{
				attribute->addUserData("OldTexture", tempTextureName);
			}
			else if (nullptr != this->datablock)
			{
				attribute->addUserData("OldTexture", this->getUnlitTextureName(this->datablock, textureType));
			}

			if (nullptr != this->datablock /*&& false == newlyCreated*/)
			{
				// If no texture has been loaded, but still the data block has an internal one, get this one and remove it manually!
				if (true == attribute->getUserDataValue("OldTexture").empty())
				{
					tempTextureName = this->getUnlitTextureName(this->datablock, textureType);
					attribute->addUserData(tempTextureName);
				}

				// createOrRetrieveTexture crashes, when texture alias name is empty
				Ogre::String oldTextureName = attribute->getUserDataValue("OldTexture");
				if (false == oldTextureName.empty())
				{
					if (false == Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(oldTextureName))
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DatablockUnlitComponent] Cannot set texture: '" + oldTextureName +
																		"', because it does not exist in any resource group! for game object: " + this->gameObjectPtr->getName());
						attribute->setValue(previousTextureName);
						this->addAttributeFilePathData(attribute);
						return;
					}

					Ogre::uint32 textureFlags = Ogre::TextureFlags::AutomaticBatching;

					Ogre::TextureTypes::TextureTypes internalTextureType = Ogre::TextureTypes::Type2D;

					Ogre::uint32 filters = Ogre::TextureFilter::FilterTypes::TypeGenerateDefaultMipmaps;

					// Really important: createOrRetrieveTexture when its created, its width/height is 0 etc. so the texture is just prepared
					// it will be filled with correct data when setDataBlock is called
					Ogre::TextureGpu* texture = hlmsTextureManager->createOrRetrieveTexture(oldTextureName,
																							Ogre::GpuPageOutStrategy::SaveToSystemRam, textureFlags, internalTextureType,
																							Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, filters, 0u);
					// Check if its a valid texture
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
						datablock->setTexture(textureType, texture);
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
	
	Ogre::String DatablockUnlitComponent::mapBlendModeToString(Ogre::UnlitBlendModes blendMode)
	{
		Ogre::String strBlendMode = "UNLIT_BLEND_NORMAL_NON_PREMUL";
		if (Ogre::UnlitBlendModes::UNLIT_BLEND_NORMAL_PREMUL == blendMode)
			strBlendMode = "UNLIT_BLEND_NORMAL_PREMUL";
		else if (Ogre::UnlitBlendModes::UNLIT_BLEND_ADD == blendMode)
			strBlendMode = "UNLIT_BLEND_ADD";
		else if (Ogre::UnlitBlendModes::UNLIT_BLEND_SUBTRACT == blendMode)
			strBlendMode = "UNLIT_BLEND_SUBTRACT";
		else if (Ogre::UnlitBlendModes::UNLIT_BLEND_MULTIPLY == blendMode)
			strBlendMode = "UNLIT_BLEND_MULTIPLY";
		else if (Ogre::UnlitBlendModes::UNLIT_BLEND_MULTIPLY2X == blendMode)
			strBlendMode = "UNLIT_BLEND_MULTIPLY2X";
		else if (Ogre::UnlitBlendModes::UNLIT_BLEND_SCREEN == blendMode)
			strBlendMode = "UNLIT_BLEND_SCREEN";
		else if (Ogre::UnlitBlendModes::UNLIT_BLEND_OVERLAY == blendMode)
			strBlendMode = "UNLIT_BLEND_OVERLAY";
		else if (Ogre::UnlitBlendModes::UNLIT_BLEND_LIGHTEN == blendMode)
			strBlendMode = "UNLIT_BLEND_LIGHTEN";
		else if (Ogre::UnlitBlendModes::UNLIT_BLEND_DARKEN == blendMode)
			strBlendMode = "UNLIT_BLEND_DARKEN";
		else if (Ogre::UnlitBlendModes::UNLIT_BLEND_GRAIN_EXTRACT == blendMode)
			strBlendMode = "UNLIT_BLEND_GRAIN_EXTRACT";
		else if (Ogre::UnlitBlendModes::UNLIT_BLEND_GRAIN_MERGE == blendMode)
			strBlendMode = "UNLIT_BLEND_GRAIN_MERGE";
		else if (Ogre::UnlitBlendModes::UNLIT_BLEND_DIFFERENCE == blendMode)
			strBlendMode = "UNLIT_BLEND_DIFFERENCE";
		
		return strBlendMode;
	}
	
	Ogre::UnlitBlendModes DatablockUnlitComponent::mapStringToBlendMode(const Ogre::String& strBlendMode)
	{
		Ogre::UnlitBlendModes blendMode = Ogre::UnlitBlendModes::UNLIT_BLEND_NORMAL_NON_PREMUL;
		if ("UNLIT_BLEND_NORMAL_PREMUL" == strBlendMode)
			blendMode = Ogre::UnlitBlendModes::UNLIT_BLEND_NORMAL_PREMUL;
		else if ("UNLIT_BLEND_ADD" == strBlendMode)
			blendMode = Ogre::UnlitBlendModes::UNLIT_BLEND_ADD;
		else if ("UNLIT_BLEND_SUBTRACT" == strBlendMode)
			blendMode = Ogre::UnlitBlendModes::UNLIT_BLEND_SUBTRACT;
		else if ("UNLIT_BLEND_MULTIPLY" == strBlendMode)
			blendMode = Ogre::UnlitBlendModes::UNLIT_BLEND_MULTIPLY;
		else if ("UNLIT_BLEND_MULTIPLY2X" == strBlendMode)
			blendMode = Ogre::UnlitBlendModes::UNLIT_BLEND_MULTIPLY2X;
		else if ("UNLIT_BLEND_SCREEN" == strBlendMode)
			blendMode = Ogre::UnlitBlendModes::UNLIT_BLEND_SCREEN;
		else if ("UNLIT_BLEND_OVERLAY" == strBlendMode)
			blendMode = Ogre::UnlitBlendModes::UNLIT_BLEND_OVERLAY;
		else if ("UNLIT_BLEND_LIGHTEN" == strBlendMode)
			blendMode = Ogre::UnlitBlendModes::UNLIT_BLEND_LIGHTEN;
		else if ("UNLIT_BLEND_DARKEN" == strBlendMode)
			blendMode = Ogre::UnlitBlendModes::UNLIT_BLEND_DARKEN;
		else if ("UNLIT_BLEND_GRAIN_EXTRACT" == strBlendMode)
			blendMode = Ogre::UnlitBlendModes::UNLIT_BLEND_GRAIN_EXTRACT;
		else if ("UNLIT_BLEND_GRAIN_MERGE" == strBlendMode)
			blendMode = Ogre::UnlitBlendModes::UNLIT_BLEND_GRAIN_MERGE;
		else if ("UNLIT_BLEND_DIFFERENCE" == strBlendMode)
			blendMode = Ogre::UnlitBlendModes::UNLIT_BLEND_DIFFERENCE;

		return blendMode;
	}

	void DatablockUnlitComponent::preReadDatablock(void)
	{
		bool success = true;
		
		if (nullptr == this->gameObjectPtr)
			return;

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("DatablockUnlitComponent::preReadDatablock", _1(&success),
		{
			Ogre::v1::Entity * entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
			if (nullptr != entity)
			{
				success = this->readDatablockEntity(entity);
			}
		});

		if (false == success)
		{
			this->datablock = nullptr;
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DatablockUnlitComponent] Unlit datablock reading failed for game object: " + this->gameObjectPtr->getName());
			return;
		}
	}

	bool DatablockUnlitComponent::readDatablockEntity(Ogre::v1::Entity* entity)
	{
		// Two data block components with the same entity index can not exist
		for (unsigned int i = 0; i < static_cast<unsigned int>(this->gameObjectPtr->getComponents()->size()); i++)
		{
			auto& priorUnlitComponent = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<DatablockUnlitComponent>(DatablockUnlitComponent::getStaticClassName(), i));
			if (nullptr != priorUnlitComponent && priorUnlitComponent.get() != this)
			{
				if (this->subEntityIndex->getUInt() == priorUnlitComponent->getSubEntityIndex())
				{
					this->subEntityIndex->setValue(priorUnlitComponent->getSubEntityIndex() + 1);
				}
			}
		}

		if (this->subEntityIndex->getUInt() >= entity->getNumSubEntities())
		{
			this->datablock = nullptr;
			Ogre::String message = "[DatablockUnlitComponent] Datablock reading failed, because there is no such sub entity index: "
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
		this->originalDatablock = dynamic_cast<Ogre::HlmsUnlitDatablock*>(entity->getSubEntity(this->subEntityIndex->getUInt())->getDatablock());
		// Datablock could not be received, pbs entity got unlit data block component?
		if (nullptr == this->originalDatablock)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DatablockUnlitComponent] Warning: Could not use datablock unlit component, because this game object has a pbs data block for game object: "
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
			this->datablock = dynamic_cast<Ogre::HlmsUnlitDatablock*>(this->originalDatablock->clone(originalDataBlockName
				+ "__" + Ogre::StringConverter::toString(this->gameObjectPtr->getId())));

			Ogre::String clonedDataBlockName = *this->datablock->getNameStr();

			this->alreadyCloned = true;
		}
		else
		{
			this->datablock = dynamic_cast<Ogre::HlmsUnlitDatablock*>(entity->getSubEntity(this->subEntityIndex->getUInt())->getDatablock());
		}

		if (nullptr == this->datablock)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DatablockUnlitComponent] Unlit datablock reading failed, because there is no unlit data block for game object: "
				+ this->gameObjectPtr->getName());
			return false;
		}

		entity->getSubEntity(this->subEntityIndex->getUInt())->setDatablock(this->datablock);
		this->oldSubIndex = this->subEntityIndex->getUInt();

		const Ogre::String* finalDatablockName = this->datablock->getNameStr();

		if (nullptr != finalDatablockName)
		{
			this->gameObjectPtr->actualizeDatablockName(*finalDatablockName, this->subEntityIndex->getUInt());
		}

		this->postReadDatablock();

		return true;
	}

	void DatablockUnlitComponent::postReadDatablock(void)
	{
		if (nullptr != this->datablock)
		{
			this->setUseColor(this->useColor->getBool());
			if (true == this->useColor->getBool())
			{
				this->setBackgroundColor(this->backgroundColor->getVector4());
			}

			Ogre::String tempTextureName;
			unsigned int tempTextureCount = 0;

			// Check how many textures are currently set in data block
			do
			{
				tempTextureName = this->getUnlitTextureName(this->datablock, tempTextureCount);
				if (false == tempTextureName.empty())
					tempTextureCount++;

			} while (false == tempTextureName.empty());

			this->textureCount->setValue(tempTextureCount);

			// this->setTextureCount(this->textureCount->getUInt());

			for (unsigned int i = 0; i < this->textureCount->getUInt(); i++)
			{
				this->setDetailTextureName(i, this->detailTextureNames[i]->getString());

				this->setBlendMode(i, this->blendModes[i]->getListSelectedValue());

				this->setTextureSwizzle(i, this->textureSwizzles[i]->getVector4());

				this->setUseAnimation(i, this->useAnimations[i]->getBool());
				if (true == this->useAnimations[i]->getBool())
				{
					this->setAnimationTranslation(i, this->animationTranslations[i]->getVector3());
					this->setAnimationScale(i, this->animationScales[i]->getVector3());
					this->setAnimationRotation(i, this->animationRotations[i]->getVector3());
				}
			}
		}
	}

	Ogre::String DatablockUnlitComponent::getClassName(void) const
	{
		return "DatablockUnlitComponent";
	}

	Ogre::String DatablockUnlitComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void DatablockUnlitComponent::update(Ogre::Real dt, bool notSimulating)
	{
		
	}

	void DatablockUnlitComponent::actualizeValue(Variant* attribute)
	{
		// Some attribute changed, component is no longer new! So textures can change
		this->newlyCreated = false;

		GameObjectComponent::actualizeValue(attribute);
		
		if (DatablockUnlitComponent::AttrSubEntityIndex() == attribute->getName())
		{
			this->setSubEntityIndex(attribute->getUInt());
		}
		else if (DatablockUnlitComponent::AttrUseColor() == attribute->getName())
		{
			this->setUseColor(attribute->getBool());
		}
		else if (DatablockUnlitComponent::AttrBackgroundColor() == attribute->getName())
		{
			this->setBackgroundColor(attribute->getVector4());
		}
		else if (DatablockUnlitComponent::AttrTextureCount() == attribute->getName())
		{
			this->setTextureCount(attribute->getUInt());
		}
		else
		{
			for (unsigned int i = 0; i < this->textureCount->getUInt(); i++)
			{
				if (DatablockUnlitComponent::AttrDetailTexture() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setDetailTextureName(i, attribute->getString());
				}
				else if (DatablockUnlitComponent::AttrBlendMode() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setBlendMode(i, attribute->getListSelectedValue());
				}
				else if (DatablockUnlitComponent::AttrTextureSwizzle() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setTextureSwizzle(i, attribute->getVector4());
				}
				else if (DatablockUnlitComponent::AttrUseAnimation() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setUseAnimation(i, attribute->getBool());
				}
				else if (DatablockUnlitComponent::AttrAnimationTranslation() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setAnimationTranslation(i, attribute->getVector3());
				}
				else if (DatablockUnlitComponent::AttrAnimationScale() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setAnimationScale(i, attribute->getVector3());
				}
				else if (DatablockUnlitComponent::AttrAnimationRotation() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setAnimationRotation(i, attribute->getVector3());
				}
			}
		}
	}

	void DatablockUnlitComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseColor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useColor->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BackgroundColor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->backgroundColor->getVector4())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TextureCount"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->textureCount->getUInt())));
		propertiesXML->append_node(propertyXML);
		
		for (size_t i = 0; i < this->textureCount->getUInt(); i++)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "DetailTextureName" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->detailTextureNames[i]->getString())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "BlendMode" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->blendModes[i]->getListSelectedValue())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "TextureSwizzle" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->textureSwizzles[i]->getVector4())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "UseAnimation" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useAnimations[i]->getBool())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "AnimationTranslation" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animationTranslations[i]->getVector3())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "AnimationScale" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animationScales[i]->getVector3())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "AnimationRotation" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animationRotations[i]->getVector3())));
			propertiesXML->append_node(propertyXML);
		}
	}

	void DatablockUnlitComponent::setSubEntityIndex(unsigned int subEntityIndex)
	{
		this->subEntityIndex->setValue(subEntityIndex);

		if (nullptr != this->gameObjectPtr)
		{
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("DatablockUnlitComponent::setSubEntityIndex", _1(&subEntityIndex),
			{
				// Two data block components with the same entity index can not exist
				for (size_t i = 0; i < this->gameObjectPtr->getComponents()->size(); i++)
				{
					auto& priorUnlitComponent = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<DatablockUnlitComponent>(DatablockUnlitComponent::getStaticClassName(), static_cast<unsigned int>(i)));
					if (nullptr != priorUnlitComponent && priorUnlitComponent.get() != this)
					{
						if (subEntityIndex == priorUnlitComponent->getSubEntityIndex())
						{
							subEntityIndex = priorUnlitComponent->getSubEntityIndex() + 1;
						}
					}
				}

				Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
				if (nullptr != entity)
				{
					if (subEntityIndex >= entity->getNumSubEntities())
					{
						subEntityIndex = static_cast<unsigned int>(entity->getNumSubEntities()) - 1;
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
						entity->getSubEntity(this->oldSubIndex)->setDatablock(this->originalDatablock);
						if (nullptr != this->datablock)
						{
							Ogre::Hlms* hlmsUnlit = Ogre::Root::getSingletonPtr()->getHlmsManager()->getHlms(Ogre::HLMS_UNLIT);
							hlmsUnlit->destroyDatablock(this->datablock->getName());
							this->datablock = nullptr;
						}
					}

					this->alreadyCloned = false;
				}
			});

			this->preReadDatablock();

			this->oldSubIndex = subEntityIndex;
		}
	}

	unsigned int DatablockUnlitComponent::getSubEntityIndex(void) const
	{
		return this->subEntityIndex->getUInt();
	}
	
	void DatablockUnlitComponent::setUseColor(bool useColor)
	{
		this->useColor->setValue(useColor);
		this->backgroundColor->setVisible(useColor);
	}
	
	bool DatablockUnlitComponent::getUseColor(void) const
	{
		return this->useColor->getBool();
	}
	
	void DatablockUnlitComponent::setBackgroundColor(const Ogre::Vector4& backgroundColor)
	{
		this->backgroundColor->setValue(backgroundColor);
		
		if (nullptr != datablock)
		{
			ENQUEUE_RENDER_COMMAND_MULTI("DatablockUnlitComponent::setBackgroundColor", _1(backgroundColor),
			{
				datablock->setColour(Ogre::ColourValue(backgroundColor.x, backgroundColor.y, backgroundColor.z, backgroundColor.w));
			});
		}
	}

	Ogre::Vector4 DatablockUnlitComponent::getBackgroundColor(void) const
	{
		return this->backgroundColor->getVector4();
	}

	void DatablockUnlitComponent::setTextureCount(unsigned int textureCount)
	{
		// Max texture count in Ogre is 16
		if (textureCount > 16)
			textureCount = 16;

		this->textureCount->setValue(textureCount);

		size_t oldSize = this->detailTextureNames.size();

		if (textureCount > oldSize)
		{
			this->detailTextureNames.resize(textureCount);
			this->blendModes.resize(textureCount);
			this->textureSwizzles.resize(textureCount);
			this->useAnimations.resize(textureCount);
			this->animationTranslations.resize(textureCount);
			this->animationScales.resize(textureCount);
			this->animationRotations.resize(textureCount);

			for (size_t i = oldSize; i < this->detailTextureNames.size(); i++)
			{
				this->detailTextureNames[i] = new Variant(DatablockUnlitComponent::AttrDetailTexture() + Ogre::StringConverter::toString(i), Ogre::String(), this->attributes);
				this->detailTextureNames[i]->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");

				this->blendModes[i] = new Variant(DatablockUnlitComponent::AttrBlendMode() + Ogre::StringConverter::toString(i), 
					{ "UNLIT_BLEND_NORMAL_NON_PREMUL", "UNLIT_BLEND_NORMAL_PREMUL", "UNLIT_BLEND_ADD"
					, "UNLIT_BLEND_SUBTRACT", "UNLIT_BLEND_MULTIPLY", "UNLIT_BLEND_MULTIPLY2X"
					, "UNLIT_BLEND_SCREEN", "UNLIT_BLEND_OVERLAY", "UNLIT_BLEND_LIGHTEN", "UNLIT_BLEND_DARKEN"
					, "UNLIT_BLEND_GRAIN_EXTRACT", "UNLIT_BLEND_GRAIN_MERGE", "UNLIT_BLEND_DIFFERENCE" }, this->attributes);

				this->textureSwizzles[i] = new Variant(DatablockUnlitComponent::AttrTextureSwizzle() + Ogre::StringConverter::toString(i), Ogre::Vector4(0.0f, 0.0f, 0.0f, 0.0f), this->attributes);
				this->useAnimations[i] = new Variant(DatablockUnlitComponent::AttrUseAnimation() + Ogre::StringConverter::toString(i), false, this->attributes);
				this->animationTranslations[i] = new Variant(DatablockUnlitComponent::AttrAnimationTranslation() + Ogre::StringConverter::toString(i), Ogre::Vector3::ZERO, this->attributes);
				this->animationScales[i] = new Variant(DatablockUnlitComponent::AttrAnimationScale() + Ogre::StringConverter::toString(i), Ogre::Vector3::ZERO, this->attributes);
				this->animationRotations[i] = new Variant(DatablockUnlitComponent::AttrAnimationRotation() + Ogre::StringConverter::toString(i), Ogre::Vector3::ZERO, this->attributes);

				this->animationTranslations[i]->setVisible(false);
				this->animationScales[i]->setVisible(false);
				this->animationRotations[i]->setVisible(false);
				this->useAnimations[i]->addUserData(GameObject::AttrActionNeedRefresh());
			}
		}
		else if (textureCount < oldSize)
		{
			this->eraseVariants(this->detailTextureNames, textureCount);
			this->eraseVariants(this->blendModes, textureCount);
			this->eraseVariants(this->textureSwizzles, textureCount);
			this->eraseVariants(this->useAnimations, textureCount);
			this->eraseVariants(this->animationTranslations, textureCount);
			this->eraseVariants(this->animationScales, textureCount);
			this->eraseVariants(this->animationRotations, textureCount);
		}
	}
	
	unsigned int DatablockUnlitComponent::getTextureCount(void) const
	{
		return this->textureCount->getUInt();
	}

	void DatablockUnlitComponent::setDetailTextureName(unsigned int index, const Ogre::String& detailTextureName)
	{
		if (index > this->detailTextureNames.size())
			index = static_cast<unsigned int>(this->detailTextureNames.size()) - 1;
		this->detailTextureNames[index]->setValue(detailTextureName);

// Attention: There is no Ogre::HlmsTextureManager::TEXTURE_TYPE_DETAIL
		this->internalSetTextureName(index, Ogre::CommonTextureTypes::Diffuse, this->detailTextureNames[index], detailTextureName);
	}
	
	const Ogre::String DatablockUnlitComponent::getDetailTextureName(unsigned int index) const
	{
		if (index > this->detailTextureNames.size())
			return "";
		return this->detailTextureNames[index]->getString();
	}
	
	void DatablockUnlitComponent::setBlendMode(unsigned int index, const Ogre::String& blendMode)
	{
		if (index > this->blendModes.size())
			index = static_cast<unsigned int>(this->blendModes.size()) - 1;
		this->blendModes[index]->setListSelectedValue(blendMode);

		if (nullptr != this->datablock)
		{
			if (false == newlyCreated)
			{
				ENQUEUE_RENDER_COMMAND_MULTI("DatablockUnlitComponent::setBlendMode", _2(index, blendMode),
				{
					this->datablock->setBlendMode(index, this->mapStringToBlendMode(blendMode));
				});
			}
			else
			{
				this->blendModes[index]->setListSelectedValue(this->mapBlendModeToString(this->datablock->getBlendMode(index)));
			}
		}
	}
	
    Ogre::String DatablockUnlitComponent::getBlendMode(unsigned int index) const
	{
		if (index > this->blendModes.size())
			return "";
		return this->blendModes[index]->getListSelectedValue();
	}
	
	void DatablockUnlitComponent::setTextureSwizzle(unsigned int index, const Ogre::Vector4& textureSwizzle)
	{
		if (index > this->textureSwizzles.size())
			index = static_cast<unsigned int>(this->textureSwizzles.size()) - 1;
		this->textureSwizzles[index]->setValue(textureSwizzle);
		
		if (nullptr != datablock && (textureSwizzle.x != 0.0f || textureSwizzle.y != 0.0f || textureSwizzle.z != 0.0f || textureSwizzle.w != 0.0f))
		{
			ENQUEUE_RENDER_COMMAND_MULTI("DatablockUnlitComponent::setTextureSwizzle", _2(index, textureSwizzle),
			{
				datablock->setTextureSwizzle(static_cast<Ogre::uint8>(index), textureSwizzle.x, textureSwizzle.y, textureSwizzle.z, textureSwizzle.w);
			});
		}
	}

	Ogre::Vector4 DatablockUnlitComponent::getTextureSwizzle(unsigned int index) const
	{
		if (index > this->textureSwizzles.size())
			return Ogre::Vector4::ZERO;
		return this->textureSwizzles[index]->getVector4();
	}
	
	void DatablockUnlitComponent::setUseAnimation(unsigned int index, bool useAnimation)
	{
		if (index > this->useAnimations.size())
			index = static_cast<unsigned int>(this->useAnimations.size()) - 1;
		this->useAnimations[index]->setValue(useAnimation);

		this->animationTranslations[index]->setVisible(useAnimation);
		this->animationScales[index]->setVisible(useAnimation);
		this->animationRotations[index]->setVisible(useAnimation);
		if (nullptr != this->datablock)
		{
			ENQUEUE_RENDER_COMMAND_MULTI("DatablockUnlitComponent::setUseAnimation", _2(index, useAnimation),
			{
				this->datablock->setEnableAnimationMatrix(index, useAnimation);
			});
		}
	}

	bool DatablockUnlitComponent::getUseAnimation(unsigned int index) const
	{
		if (index > this->useAnimations.size())
			return false;
		return this->useAnimations[index]->getBool();
	}

	void DatablockUnlitComponent::setAnimationTranslation(unsigned int index, const Ogre::Vector3& animationTranslation)
	{
		if (index > this->animationTranslations.size())
			index = static_cast<unsigned int>(this->animationTranslations.size()) - 1;
		this->animationTranslations[index]->setValue(animationTranslation);

		if (true == this->useAnimations[index]->getBool())
		{
			if (nullptr != this->datablock)
			{
				Ogre::Matrix4 animationMatrix(Ogre::Matrix4::IDENTITY);
				animationMatrix.setTrans(this->animationTranslations[index]->getVector3());
				animationMatrix.setScale(this->animationScales[index]->getVector3());
				
				animationMatrix.makeTransform(
					Ogre::Vector3(0.5f + 0.70710678118f * Ogre::Math::Cos(this->animationRotations[index]->getVector3().y + 5.0f * Ogre::Math::PI / 4.0f), 0.5f + 0.70710678118f * Ogre::Math::Sin(this->animationRotations[index]->getVector3().y + 5.0f * Ogre::Math::PI / 4.0f), 0),
					Ogre::Vector3(1, 1, 1),
					Ogre::Quaternion(Ogre::Radian(this->animationRotations[index]->getVector3().y), Ogre::Vector3(0, 0, 1)));

				ENQUEUE_RENDER_COMMAND_MULTI("DatablockUnlitComponent::setAnimationTranslation", _3(index, animationTranslation, animationMatrix),
				{
					this->datablock->setEnableAnimationMatrix(0, true);
					this->datablock->setAnimationMatrix(0, animationMatrix);
				});
			}
		}
	}

	Ogre::Vector3 DatablockUnlitComponent::getAnimationTranslation(unsigned int index) const
	{
		if (index > this->animationTranslations.size())
			return Ogre::Vector3::ZERO;
		return this->animationTranslations[index]->getVector3();
	}

	void DatablockUnlitComponent::setAnimationScale(unsigned int index, const Ogre::Vector3& animationScale)
	{
		if (index > this->animationScales.size())
			index = static_cast<unsigned int>(this->animationScales.size()) - 1;
		this->animationScales[index]->setValue(animationScale);

		if (true == this->useAnimations[index]->getBool())
		{
			if (nullptr != this->datablock)
			{
				Ogre::Matrix4 animationMatrix(Ogre::Matrix4::IDENTITY);
				animationMatrix.setTrans(this->animationTranslations[index]->getVector3());
				animationMatrix.setScale(this->animationScales[index]->getVector3());

				animationMatrix.makeTransform(
					Ogre::Vector3(0.5f + 0.70710678118f * Ogre::Math::Cos(this->animationRotations[index]->getVector3().y + 5.0f * Ogre::Math::PI / 4.0f), 0.5f + 0.70710678118f * Ogre::Math::Sin(this->animationRotations[index]->getVector3().y + 5.0f * Ogre::Math::PI / 4.0f), 0),
					Ogre::Vector3(1, 1, 1),
					Ogre::Quaternion(Ogre::Radian(this->animationRotations[index]->getVector3().y), Ogre::Vector3(0, 0, 1)));

				ENQUEUE_RENDER_COMMAND_MULTI("DatablockUnlitComponent::setAnimationScale", _3(index, animationScale, animationMatrix),
				{
					this->datablock->setEnableAnimationMatrix(0, true);
					this->datablock->setAnimationMatrix(0, animationMatrix);
				});
			}
		}
	}

	Ogre::Vector3 DatablockUnlitComponent::getAnimationScale(unsigned int index) const
	{
		if (index > this->animationScales.size())
			return Ogre::Vector3::ZERO;
		return this->animationScales[index]->getVector3();
	}
	
	void DatablockUnlitComponent::setAnimationRotation(unsigned int index, const Ogre::Vector3& animationRotation)
	{
		if (index > this->animationRotations.size())
			index = static_cast<unsigned int>(this->animationRotations.size()) - 1;
		this->animationRotations[index]->setValue(animationRotation);

		if (true == this->useAnimations[index]->getBool())
		{
			if (nullptr != this->datablock)
			{
				Ogre::Matrix4 animationMatrix(Ogre::Matrix4::IDENTITY);
				animationMatrix.setTrans(this->animationTranslations[index]->getVector3());
				animationMatrix.setScale(this->animationScales[index]->getVector3());


				// sqrt(0.5) = 0.70710678118
				animationMatrix.makeTransform(
					Ogre::Vector3(0.5f + 0.70710678118f * Ogre::Math::Cos(animationRotation.y + 5.0f * Ogre::Math::PI / 4.0f), 0.5f + 0.70710678118f * Ogre::Math::Sin(animationRotation.y + 5.0f * Ogre::Math::PI / 4.0f), 0),
					Ogre::Vector3(1, 1, 1),
					Ogre::Quaternion(Ogre::Radian(animationRotation.y), Ogre::Vector3(0, 0, 1)));

				ENQUEUE_RENDER_COMMAND_MULTI("DatablockUnlitComponent::setAnimationRotation", _3(index, animationRotation, animationMatrix),
				{
					this->datablock->setEnableAnimationMatrix(0, true);
					this->datablock->setAnimationMatrix(0, animationMatrix);

					// rot.FromEulerAnglesXYZ(Ogre::Degree(0), Ogre::Degree(0), Ogre::Degree(90));
					// collisionOrientation.FromRotationMatrix(rot);
					// ??
					// animationMatrix.setRotation(this->animationRotations[index]->getVector3());  // What here?
					this->datablock->setAnimationMatrix(0, animationMatrix);
				});
			}
		}
	}

	Ogre::Vector3 DatablockUnlitComponent::getAnimationRotation(unsigned int index) const
	{
		if (index > this->animationRotations.size())
			return Ogre::Vector3::ZERO;
		return this->animationRotations[index]->getVector3();
	}

	Ogre::HlmsDatablock* DatablockUnlitComponent::getDataBlock(void) const
	{
		return this->datablock;
	}

	void DatablockUnlitComponent::doNotCloneDataBlock(void)
	{
		this->alreadyCloned = true;
	}
	
}; // namespace end