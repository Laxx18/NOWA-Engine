#include "NOWAPrecompiled.h"
#include "MorphAnimationComponent.h"

#include "gameobject/GameObjectFactory.h"
#include "main/AppStateManager.h"
#include "main/EventManager.h"
#include "modules/GraphicsModule.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"

#include "OgreAbiUtils.h"
#include "OgreItem.h"
#include "OgreMesh2.h"
#include "OgreSubItem.h"
#include "OgreSubMesh2.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    MorphAnimationComponent::MorphAnimationComponent() :
        GameObjectComponent(),
        name("MorphAnimationComponent"),
        activated(new Variant(MorphAnimationComponent::AttrActivated(), true, this->attributes)),
        poseAnimationCount(new Variant(MorphAnimationComponent::AttrPoseAnimationCount(), static_cast<unsigned int>(0), this->attributes)),
        accumulator(0.0f),
        item(nullptr)
    {
        this->poseAnimationCount->addUserData(GameObject::AttrActionNeedRefresh());
    }

    MorphAnimationComponent::~MorphAnimationComponent(void)
    {
    }

    void MorphAnimationComponent::initialise()
    {
    }

    const Ogre::String& MorphAnimationComponent::getName() const
    {
        return this->name;
    }

    void MorphAnimationComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<MorphAnimationComponent>(MorphAnimationComponent::getStaticClassId(), MorphAnimationComponent::getStaticClassName());
    }

    void MorphAnimationComponent::shutdown()
    {
        // Do nothing here, because its called far too late and nothing is there of NOWA-Engine anymore!
        // Use @onRemoveComponent in order to destroy something.
    }

    void MorphAnimationComponent::uninstall()
    {
        // Do nothing here, because its called far too late and nothing is there of NOWA-Engine anymore!
        // Use @onRemoveComponent in order to destroy something.
    }

    void MorphAnimationComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    bool MorphAnimationComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        bool success = GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PoseAnimationCount")
        {
            this->poseAnimationCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        // Read pose animation attributes
        for (unsigned int i = 0; i < this->poseAnimationCount->getUInt(); i++)
        {
            // Sub item index
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SubItemIndex" + Ogre::StringConverter::toString(i))
            {
                if (i < this->subItemIndices.size())
                {
                    this->subItemIndices[i]->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
                }
                else
                {
                    this->subItemIndices.push_back(new Variant(MorphAnimationComponent::AttrSubItemIndex() + Ogre::StringConverter::toString(i), XMLConverter::getAttribUnsignedInt(propertyElement, "data"), this->attributes));
                }
                propertyElement = propertyElement->next_sibling("property");
            }

            // Pose index
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PoseIndex" + Ogre::StringConverter::toString(i))
            {
                if (i < this->poseIndices.size())
                {
                    this->poseIndices[i]->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
                }
                else
                {
                    this->poseIndices.push_back(new Variant(MorphAnimationComponent::AttrPoseIndex() + Ogre::StringConverter::toString(i), XMLConverter::getAttribUnsignedInt(propertyElement, "data"), this->attributes));
                }
                propertyElement = propertyElement->next_sibling("property");
            }

            // Weight function
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WeightFunction" + Ogre::StringConverter::toString(i))
            {
                if (i < this->weightFunctions.size())
                {
                    this->weightFunctions[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
                }
                else
                {
                    this->weightFunctions.push_back(new Variant(MorphAnimationComponent::AttrWeightFunction() + Ogre::StringConverter::toString(i), XMLConverter::getAttrib(propertyElement, "data"), this->attributes));
                }
                propertyElement = propertyElement->next_sibling("property");
            }

            // Speed
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Speed" + Ogre::StringConverter::toString(i))
            {
                if (i < this->speeds.size())
                {
                    this->speeds[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
                }
                else
                {
                    this->speeds.push_back(new Variant(MorphAnimationComponent::AttrSpeed() + Ogre::StringConverter::toString(i), XMLConverter::getAttribReal(propertyElement, "data"), this->attributes));
                    this->speeds[i]->setConstraints(0.01f, 100.0f);
                }
                propertyElement = propertyElement->next_sibling("property");
            }

            // Time offset
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TimeOffset" + Ogre::StringConverter::toString(i))
            {
                if (i < this->timeOffsets.size())
                {
                    this->timeOffsets[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
                }
                else
                {
                    this->timeOffsets.push_back(new Variant(MorphAnimationComponent::AttrTimeOffset() + Ogre::StringConverter::toString(i), XMLConverter::getAttribReal(propertyElement, "data"), this->attributes));
                }
                propertyElement = propertyElement->next_sibling("property");
            }
        }

        return success;
    }

    GameObjectCompPtr MorphAnimationComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        MorphAnimationComponentPtr clonedCompPtr(boost::make_shared<MorphAnimationComponent>());

        clonedCompPtr->setActivated(this->activated->getBool());
        clonedCompPtr->setPoseAnimationCount(this->poseAnimationCount->getUInt());

        for (unsigned int i = 0; i < this->poseAnimationCount->getUInt(); i++)
        {
            clonedCompPtr->setSubItemIndex(i, this->subItemIndices[i]->getUInt());
            clonedCompPtr->setPoseIndex(i, this->poseIndices[i]->getUInt());
            clonedCompPtr->setWeightFunction(i, this->weightFunctions[i]->getString());
            clonedCompPtr->setSpeed(i, this->speeds[i]->getReal());
            clonedCompPtr->setTimeOffset(i, this->timeOffsets[i]->getReal());
        }

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
        return clonedCompPtr;
    }

    bool MorphAnimationComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MorphAnimationComponent] Init component for game object: " + this->gameObjectPtr->getName());

        // In Ogre-Next V2 there is no v1::Entity anymore, everything is an Item with SubItems.
        this->item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
        if (nullptr == this->item)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MorphAnimationComponent] Error: GameObject '" + this->gameObjectPtr->getName() + "' has no Ogre::Item!");
            return false;
        }

        this->initializePoseData();

        // Create attributes if there is a count but no attributes yet
        if (this->poseAnimationCount->getUInt() > 0 && this->poseIndices.empty())
        {
            this->createPoseAnimationAttributes(0);
        }

        if (this->poseAnimationCount->getUInt() > 0)
        {
            if (false == this->parseMathematicalFunctions())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MorphAnimationComponent] Failed to parse mathematical functions for game object: " + this->gameObjectPtr->getName());
            }
        }

        return true;
    }

    bool MorphAnimationComponent::connect(void)
    {
        GameObjectComponent::connect();

        this->accumulator = 0.0f;

        return true;
    }

    bool MorphAnimationComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();

        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(this->getClosureId());

        // Pose weights live in the sub item and are uploaded with the const buffer,
        // so they must be reset on the render thread or the mesh stays deformed
        // after the simulation has been stopped.
        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->clearPoseWeights();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MorphAnimationComponent::disconnect");

        this->accumulator = 0.0f;

        return true;
    }

    void MorphAnimationComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(this->getClosureId());

        if (nullptr != this->item)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
            {
                this->clearPoseWeights();
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MorphAnimationComponent::onRemoveComponent");
        }

        this->item = nullptr;
    }

    Ogre::String MorphAnimationComponent::getClosureId(void) const
    {
        return this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
    }

    void MorphAnimationComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (true == notSimulating || false == this->activated->getBool() || 0 == this->poseAnimationCount->getUInt())
        {
            return;
        }

        if (nullptr == this->item)
        {
            return;
        }

        this->accumulator += dt;

        auto closureFunction = [this](Ogre::Real renderDt)
        {
            this->applyPoseWeights();
        };

        NOWA::GraphicsModule::getInstance()->updateTrackedClosure(this->getClosureId(), closureFunction, false);
    }

    void MorphAnimationComponent::applyPoseWeights(void)
    {
        if (nullptr == this->item)
        {
            return;
        }

        const unsigned int subItemCount = static_cast<unsigned int>(this->item->getNumSubItems());

        for (unsigned int i = 0; i < this->poseAnimationCount->getUInt(); i++)
        {
            if (i >= this->subItemIndices.size() || i >= this->poseIndices.size() || i >= this->functionParsers.size())
            {
                continue;
            }

            const unsigned int subItemIndex = this->subItemIndices[i]->getUInt();
            if (subItemIndex >= subItemCount)
            {
                continue;
            }

            Ogre::SubItem* subItem = this->item->getSubItem(subItemIndex);
            if (nullptr == subItem)
            {
                continue;
            }

            const unsigned int poseIndex = this->poseIndices[i]->getUInt();
            if (poseIndex >= static_cast<unsigned int>(subItem->getNumPoses()))
            {
                continue;
            }

            Ogre::Real speed = 1.0f;
            if (i < this->speeds.size())
            {
                speed = this->speeds[i]->getReal();
            }

            Ogre::Real timeOffset = 0.0f;
            if (i < this->timeOffsets.size())
            {
                timeOffset = this->timeOffsets[i]->getReal();
            }

            double t = static_cast<double>(this->accumulator * speed + timeOffset);
            double varT[] = {t};
            double weight = this->functionParsers[i].Eval(varT);

            // Clamp the weight, because an unbounded function would blow up the mesh
            if (weight < 0.0)
            {
                weight = 0.0;
            }
            if (weight > 1.0)
            {
                weight = 1.0;
            }

            const Ogre::Real newWeight = static_cast<Ogre::Real>(weight);

            subItem->setPoseWeight(static_cast<size_t>(poseIndex), newWeight);
            this->currentPoseWeights[std::make_pair(subItemIndex, poseIndex)] = newWeight;
        }
    }

    void MorphAnimationComponent::clearPoseWeights(void)
    {
        if (nullptr == this->item)
        {
            return;
        }

        const unsigned int subItemCount = static_cast<unsigned int>(this->item->getNumSubItems());

        for (auto& pair : this->currentPoseWeights)
        {
            const unsigned int subItemIndex = pair.first.first;
            const unsigned int poseIndex = pair.first.second;

            if (subItemIndex >= subItemCount)
            {
                continue;
            }

            Ogre::SubItem* subItem = this->item->getSubItem(subItemIndex);
            if (nullptr == subItem)
            {
                continue;
            }

            if (poseIndex >= static_cast<unsigned int>(subItem->getNumPoses()))
            {
                continue;
            }

            subItem->setPoseWeight(static_cast<size_t>(poseIndex), 0.0f);
            pair.second = 0.0f;
        }
    }

    void MorphAnimationComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (MorphAnimationComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (MorphAnimationComponent::AttrPoseAnimationCount() == attribute->getName())
        {
            this->setPoseAnimationCount(attribute->getUInt());
        }
        else
        {
            for (unsigned int i = 0; i < this->poseAnimationCount->getUInt(); i++)
            {
                if ((MorphAnimationComponent::AttrSubItemIndex() + Ogre::StringConverter::toString(i)) == attribute->getName())
                {
                    this->setSubItemIndex(i, attribute->getUInt());
                }
                else if ((MorphAnimationComponent::AttrPoseIndex() + Ogre::StringConverter::toString(i)) == attribute->getName())
                {
                    this->setPoseIndex(i, attribute->getUInt());
                }
                else if ((MorphAnimationComponent::AttrWeightFunction() + Ogre::StringConverter::toString(i)) == attribute->getName())
                {
                    this->setWeightFunction(i, attribute->getString());
                }
                else if ((MorphAnimationComponent::AttrSpeed() + Ogre::StringConverter::toString(i)) == attribute->getName())
                {
                    this->setSpeed(i, attribute->getReal());
                }
                else if ((MorphAnimationComponent::AttrTimeOffset() + Ogre::StringConverter::toString(i)) == attribute->getName())
                {
                    this->setTimeOffset(i, attribute->getReal());
                }
            }
        }
    }

    void MorphAnimationComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "PoseAnimationCount"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->poseAnimationCount->getUInt())));
        propertiesXML->append_node(propertyXML);

        for (unsigned int i = 0; i < this->poseAnimationCount->getUInt(); i++)
        {
            // Sub item index
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "SubItemIndex" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->subItemIndices[i]->getUInt())));
            propertiesXML->append_node(propertyXML);

            // Pose index
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "PoseIndex" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->poseIndices[i]->getUInt())));
            propertiesXML->append_node(propertyXML);

            // Weight function
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "WeightFunction" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->weightFunctions[i]->getString())));
            propertiesXML->append_node(propertyXML);

            // Speed
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "Speed" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->speeds[i]->getReal())));
            propertiesXML->append_node(propertyXML);

            // Time offset
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "TimeOffset" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->timeOffsets[i]->getReal())));
            propertiesXML->append_node(propertyXML);
        }
    }

    Ogre::String MorphAnimationComponent::getClassName(void) const
    {
        return "MorphAnimationComponent";
    }

    Ogre::String MorphAnimationComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    void MorphAnimationComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);

        if (false == activated)
        {
            NOWA::GraphicsModule::getInstance()->removeTrackedClosure(this->getClosureId());

            if (nullptr != this->item)
            {
                NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
                {
                    this->clearPoseWeights();
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MorphAnimationComponent::setActivated");
            }
        }
    }

    bool MorphAnimationComponent::getActivated(void) const
    {
        return this->activated->getBool();
    }

    void MorphAnimationComponent::setPoseAnimationCount(unsigned int poseAnimationCount)
    {
        unsigned int prevCount = this->poseAnimationCount->getUInt();
        this->poseAnimationCount->setValue(poseAnimationCount);

        if (poseAnimationCount > prevCount)
        {
            this->createPoseAnimationAttributes(prevCount);
        }
        else if (poseAnimationCount < prevCount)
        {
            this->removePoseAnimationAttributes(poseAnimationCount);
        }

        this->parseMathematicalFunctions();
    }

    unsigned int MorphAnimationComponent::getPoseAnimationCount(void) const
    {
        return this->poseAnimationCount->getUInt();
    }

    void MorphAnimationComponent::setSubItemIndex(unsigned int index, unsigned int subItemIndex)
    {
        if (index < this->subItemIndices.size())
        {
            this->subItemIndices[index]->setValue(subItemIndex);
        }
    }

    unsigned int MorphAnimationComponent::getSubItemIndex(unsigned int index) const
    {
        if (index < this->subItemIndices.size())
        {
            return this->subItemIndices[index]->getUInt();
        }
        return 0;
    }

    void MorphAnimationComponent::setPoseIndex(unsigned int index, unsigned int poseIndex)
    {
        if (index < this->poseIndices.size())
        {
            this->poseIndices[index]->setValue(poseIndex);
        }
    }

    unsigned int MorphAnimationComponent::getPoseIndex(unsigned int index) const
    {
        if (index < this->poseIndices.size())
        {
            return this->poseIndices[index]->getUInt();
        }
        return 0;
    }

    void MorphAnimationComponent::setWeightFunction(unsigned int index, const Ogre::String& weightFunction)
    {
        if (index < this->weightFunctions.size())
        {
            this->weightFunctions[index]->setValue(weightFunction);
            this->parseMathematicalFunctions();
        }
    }

    Ogre::String MorphAnimationComponent::getWeightFunction(unsigned int index) const
    {
        if (index < this->weightFunctions.size())
        {
            return this->weightFunctions[index]->getString();
        }
        return "";
    }

    void MorphAnimationComponent::setSpeed(unsigned int index, Ogre::Real speed)
    {
        if (index < this->speeds.size())
        {
            this->speeds[index]->setValue(speed);
        }
    }

    Ogre::Real MorphAnimationComponent::getSpeed(unsigned int index) const
    {
        if (index < this->speeds.size())
        {
            return this->speeds[index]->getReal();
        }
        return 1.0f;
    }

    void MorphAnimationComponent::setTimeOffset(unsigned int index, Ogre::Real timeOffset)
    {
        if (index < this->timeOffsets.size())
        {
            this->timeOffsets[index]->setValue(timeOffset);
        }
    }

    Ogre::Real MorphAnimationComponent::getTimeOffset(unsigned int index) const
    {
        if (index < this->timeOffsets.size())
        {
            return this->timeOffsets[index]->getReal();
        }
        return 0.0f;
    }

    unsigned int MorphAnimationComponent::getSubItemCount(void) const
    {
        if (nullptr == this->item)
        {
            return 0;
        }
        return static_cast<unsigned int>(this->item->getNumSubItems());
    }

    unsigned int MorphAnimationComponent::getPoseCount(unsigned int subItemIndex) const
    {
        if (subItemIndex >= this->poseCountsPerSubItem.size())
        {
            return 0;
        }
        return this->poseCountsPerSubItem[subItemIndex];
    }

    void MorphAnimationComponent::setPoseWeight(unsigned int subItemIndex, unsigned int poseIndex, Ogre::Real weight)
    {
        if (nullptr == this->item)
        {
            return;
        }

        if (weight < 0.0f)
        {
            weight = 0.0f;
        }
        if (weight > 1.0f)
        {
            weight = 1.0f;
        }

        if (subItemIndex >= static_cast<unsigned int>(this->item->getNumSubItems()))
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MorphAnimationComponent] Cannot set pose weight, sub item index out of range: " + Ogre::StringConverter::toString(subItemIndex));
            return;
        }

        this->currentPoseWeights[std::make_pair(subItemIndex, poseIndex)] = weight;

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, subItemIndex, poseIndex, weight]()
        {
            Ogre::SubItem* subItem = this->item->getSubItem(subItemIndex);
            if (nullptr == subItem)
            {
                return;
            }
            if (poseIndex >= static_cast<unsigned int>(subItem->getNumPoses()))
            {
                return;
            }
            subItem->setPoseWeight(static_cast<size_t>(poseIndex), weight);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MorphAnimationComponent::setPoseWeight");
    }

    void MorphAnimationComponent::setPoseWeightByName(unsigned int subItemIndex, const Ogre::String& poseName, Ogre::Real weight)
    {
        if (nullptr == this->item)
        {
            return;
        }

        if (weight < 0.0f)
        {
            weight = 0.0f;
        }
        if (weight > 1.0f)
        {
            weight = 1.0f;
        }

        if (subItemIndex >= static_cast<unsigned int>(this->item->getNumSubItems()))
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MorphAnimationComponent] Cannot set pose weight, sub item index out of range: " + Ogre::StringConverter::toString(subItemIndex));
            return;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, subItemIndex, poseName, weight]()
        {
            Ogre::SubItem* subItem = this->item->getSubItem(subItemIndex);
            if (nullptr == subItem)
            {
                return;
            }
            subItem->setPoseWeight(poseName, weight);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MorphAnimationComponent::setPoseWeightByName");
    }

    Ogre::Real MorphAnimationComponent::getPoseWeight(unsigned int subItemIndex, unsigned int poseIndex) const
    {
        auto it = this->currentPoseWeights.find(std::make_pair(subItemIndex, poseIndex));
        if (it != this->currentPoseWeights.end())
        {
            return it->second;
        }
        return 0.0f;
    }

    void MorphAnimationComponent::resetTime(void)
    {
        this->accumulator = 0.0f;
    }

    Ogre::Real MorphAnimationComponent::getAccumulatedTime(void) const
    {
        return this->accumulator;
    }

    bool MorphAnimationComponent::parseMathematicalFunctions(void)
    {
        this->functionParsers.clear();
        this->functionParsers.resize(this->poseAnimationCount->getUInt());

        for (unsigned int i = 0; i < this->poseAnimationCount->getUInt(); i++)
        {
            this->functionParsers[i].AddConstant("PI", 3.1415926535897932);
            this->functionParsers[i].AddConstant("pi", 3.1415926535897932);

            if (i < this->weightFunctions.size())
            {
                Ogre::String function = this->weightFunctions[i]->getString();
                if (true == function.empty())
                {
                    // Default function similar to the Ogre sample: (sin(t) + 1) / 2
                    function = "(sin(t) + 1) / 2";
                    this->weightFunctions[i]->setValue(function);
                }

                int result = this->functionParsers[i].Parse(function, "t");
                if (result >= 0)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                        "[MorphAnimationComponent] Mathematical function parse error for entry " + Ogre::StringConverter::toString(i) + ": " + Ogre::String(this->functionParsers[i].ErrorMsg()));
                    return false;
                }
            }
        }
        return true;
    }

    void MorphAnimationComponent::initializePoseData(void)
    {
        this->poseCountsPerSubItem.clear();

        if (nullptr == this->item)
        {
            return;
        }

        const unsigned int subItemCount = static_cast<unsigned int>(this->item->getNumSubItems());
        unsigned int totalPoseCount = 0;

        for (unsigned int i = 0; i < subItemCount; i++)
        {
            Ogre::SubItem* subItem = this->item->getSubItem(i);
            unsigned int poseCount = 0;
            if (nullptr != subItem)
            {
                poseCount = static_cast<unsigned int>(subItem->getNumPoses());
            }
            this->poseCountsPerSubItem.push_back(poseCount);
            totalPoseCount += poseCount;

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                "[MorphAnimationComponent] Sub item " + Ogre::StringConverter::toString(i) + " has " + Ogre::StringConverter::toString(poseCount) + " pose(s) for game object: " + this->gameObjectPtr->getName());
        }

        if (0 == totalPoseCount)
        {
            // This is the most likely failure after a mesh conversion: the v1 mesh had shape keys,
            // but the v2 conversion did not carry the pose data over, so nothing can be animated.
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[MorphAnimationComponent] Warning: The mesh of game object '" + this->gameObjectPtr->getName() + "' has no poses at all. Check whether the v1 to v2 mesh conversion preserved the shape keys.");
        }
    }

    void MorphAnimationComponent::createPoseAnimationAttributes(unsigned int prevIndex)
    {
        for (unsigned int i = prevIndex; i < this->poseAnimationCount->getUInt(); i++)
        {
            // Sub item index
            Variant* subItemIndex = new Variant(MorphAnimationComponent::AttrSubItemIndex() + Ogre::StringConverter::toString(i), static_cast<unsigned int>(0), this->attributes);
            this->subItemIndices.push_back(subItemIndex);

            // Pose index, defaults to the running entry number so several entries do not all address pose 0
            unsigned int defaultPoseIndex = i;
            if (false == this->poseCountsPerSubItem.empty())
            {
                if (defaultPoseIndex >= this->poseCountsPerSubItem[0])
                {
                    defaultPoseIndex = 0;
                }
            }
            Variant* poseIndex = new Variant(MorphAnimationComponent::AttrPoseIndex() + Ogre::StringConverter::toString(i), defaultPoseIndex, this->attributes);
            this->poseIndices.push_back(poseIndex);

            // Weight function, default to the Ogre sample style function with a per entry time offset
            Ogre::String defaultFunction = "(sin(t) + 1) / 2";
            this->weightFunctions.push_back(new Variant(MorphAnimationComponent::AttrWeightFunction() + Ogre::StringConverter::toString(i), defaultFunction, this->attributes));

            // Speed
            Variant* speed = new Variant(MorphAnimationComponent::AttrSpeed() + Ogre::StringConverter::toString(i), 1.0f, this->attributes);
            speed->setConstraints(0.01f, 100.0f);
            this->speeds.push_back(speed);

            // Time offset
            Variant* timeOffset = new Variant(MorphAnimationComponent::AttrTimeOffset() + Ogre::StringConverter::toString(i), static_cast<Ogre::Real>(i), this->attributes);
            this->timeOffsets.push_back(timeOffset);
        }
    }

    void MorphAnimationComponent::removePoseAnimationAttributes(unsigned int count)
    {
        this->eraseVariants(this->subItemIndices, count);
        this->eraseVariants(this->poseIndices, count);
        this->eraseVariants(this->weightFunctions, count);
        this->eraseVariants(this->speeds, count);
        this->eraseVariants(this->timeOffsets, count);
    }

    bool MorphAnimationComponent::canStaticAddComponent(GameObject* gameObject)
    {
        Ogre::Item* item = gameObject->getMovableObject<Ogre::Item>();
        if (nullptr == item)
        {
            return false;
        }

        // Only offer this component if the mesh actually carries at least one pose
        bool hasAnyPose = false;
        const size_t subItemCount = item->getNumSubItems();
        for (size_t i = 0; i < subItemCount; i++)
        {
            Ogre::SubItem* subItem = item->getSubItem(i);
            if (nullptr != subItem && subItem->getNumPoses() > 0)
            {
                hasAnyPose = true;
                break;
            }
        }

        if (false == hasAnyPose)
        {
            return false;
        }

        auto morphCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<MorphAnimationComponent>());
        if (nullptr == morphCompPtr)
        {
            return true;
        }

        return false;
    }

    // Lua registration part

    MorphAnimationComponent* getMorphAnimationComponentFromIndex(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<MorphAnimationComponent>(gameObject->getComponentWithOccurrence<MorphAnimationComponent>(occurrenceIndex)).get();
    }

    MorphAnimationComponent* getMorphAnimationComponent(GameObject* gameObject)
    {
        return makeStrongPtr<MorphAnimationComponent>(gameObject->getComponent<MorphAnimationComponent>()).get();
    }

    MorphAnimationComponent* getMorphAnimationComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<MorphAnimationComponent>(gameObject->getComponentFromName<MorphAnimationComponent>(name)).get();
    }

    void MorphAnimationComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)[class_<MorphAnimationComponent, GameObjectComponent>("MorphAnimationComponent")
                .def("setActivated", &MorphAnimationComponent::setActivated)
                .def("getActivated", &MorphAnimationComponent::getActivated)
                .def("setPoseAnimationCount", &MorphAnimationComponent::setPoseAnimationCount)
                .def("getPoseAnimationCount", &MorphAnimationComponent::getPoseAnimationCount)
                .def("setSubItemIndex", &MorphAnimationComponent::setSubItemIndex)
                .def("getSubItemIndex", &MorphAnimationComponent::getSubItemIndex)
                .def("setPoseIndex", &MorphAnimationComponent::setPoseIndex)
                .def("getPoseIndex", &MorphAnimationComponent::getPoseIndex)
                .def("setWeightFunction", &MorphAnimationComponent::setWeightFunction)
                .def("getWeightFunction", &MorphAnimationComponent::getWeightFunction)
                .def("setSpeed", &MorphAnimationComponent::setSpeed)
                .def("getSpeed", &MorphAnimationComponent::getSpeed)
                .def("setTimeOffset", &MorphAnimationComponent::setTimeOffset)
                .def("getTimeOffset", &MorphAnimationComponent::getTimeOffset)
                .def("getSubItemCount", &MorphAnimationComponent::getSubItemCount)
                .def("getPoseCount", &MorphAnimationComponent::getPoseCount)
                .def("setPoseWeight", &MorphAnimationComponent::setPoseWeight)
                .def("setPoseWeightByName", &MorphAnimationComponent::setPoseWeightByName)
                .def("getPoseWeight", &MorphAnimationComponent::getPoseWeight)
                .def("resetTime", &MorphAnimationComponent::resetTime)
                .def("getAccumulatedTime", &MorphAnimationComponent::getAccumulatedTime)];

        LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "class inherits GameObjectComponent", MorphAnimationComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "void setActivated(bool activated)", "Sets whether the morph animation is activated.");
        LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "bool getActivated()", "Gets whether the morph animation is activated.");
        LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "void setPoseAnimationCount(unsigned int count)", "Sets the number of pose animation entries to control.");
        LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "unsigned int getPoseAnimationCount()", "Gets the number of pose animation entries.");
        LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "void setSubItemIndex(unsigned int index, unsigned int subItemIndex)", "Sets the sub item index the entry at the given position addresses.");
        LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "unsigned int getSubItemIndex(unsigned int index)", "Gets the sub item index the entry at the given position addresses.");
        LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "void setPoseIndex(unsigned int index, unsigned int poseIndex)", "Sets the pose index the entry at the given position addresses.");
        LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "unsigned int getPoseIndex(unsigned int index)", "Gets the pose index the entry at the given position addresses.");
        LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "void setWeightFunction(unsigned int index, String weightFunction)", "Sets the weight function for the entry at the given index. Example: '(sin(t) + 1) / 2'");
        LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "String getWeightFunction(unsigned int index)", "Gets the weight function for the entry at the given index.");
        LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "void setSpeed(unsigned int index, float speed)", "Sets the speed multiplier for the entry at the given index.");
        LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "float getSpeed(unsigned int index)", "Gets the speed multiplier for the entry at the given index.");
        LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "void setTimeOffset(unsigned int index, float timeOffset)", "Sets the time offset for the entry at the given index.");
        LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "float getTimeOffset(unsigned int index)", "Gets the time offset for the entry at the given index.");
        LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "unsigned int getSubItemCount()", "Gets the number of sub items of the item.");
        LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "unsigned int getPoseCount(unsigned int subItemIndex)", "Gets the number of poses of the given sub item.");
        LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "void setPoseWeight(unsigned int subItemIndex, unsigned int poseIndex, float weight)",
            "Manually sets the weight of a pose, addressed by sub item index and pose index.");
        LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "void setPoseWeightByName(unsigned int subItemIndex, String poseName, float weight)",
            "Manually sets the weight of a pose, addressed by sub item index and pose name.");
        LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "float getPoseWeight(unsigned int subItemIndex, unsigned int poseIndex)", "Gets the last weight this component wrote for the given pose.");
        LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "void resetTime()", "Resets the time accumulator to zero.");
        LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "float getAccumulatedTime()", "Gets the current time accumulator value.");

        gameObjectClass.def("getMorphAnimationComponentFromName", &getMorphAnimationComponentFromName);
        gameObjectClass.def("getMorphAnimationComponent", (MorphAnimationComponent * (*)(GameObject*)) & getMorphAnimationComponent);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MorphAnimationComponent getMorphAnimationComponentFromName(String name)", "Gets the morph animation component by the given name. Returns nil if the component does not exist.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MorphAnimationComponent getMorphAnimationComponent()", "Gets the morph animation component. Returns nil if the component does not exist.");

        gameObjectControllerClass.def("castMorphAnimationComponent", &GameObjectController::cast<MorphAnimationComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "MorphAnimationComponent castMorphAnimationComponent(MorphAnimationComponent other)", "Casts an incoming type from function for lua auto completion.");
    }

}; // namespace end