#include "NOWAPrecompiled.h"
#include "TimeLineComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	TimeLineComponent::TimeLineComponent()
		: GameObjectComponent(),
		name("TimeLineComponent"),
		timePointIndex(0),
		totalIndex(0),
		timeDt(0.0f),
		callForEndPoint(false),
		activated(new Variant(TimeLineComponent::AttrActivated(), true, this->attributes))
	{
		this->timePointCount = new Variant(TimeLineComponent::AttrTimePointCount(), 0, this->attributes);

		// Since when node game object count is changed, the whole properties must be refreshed, so that new field may come for node tracks
		this->timePointCount->addUserData(GameObject::AttrActionNeedRefresh());
		this->timePointCount->addUserData(GameObject::AttrActionSeparator());
	}

	TimeLineComponent::~TimeLineComponent(void)
	{
		
	}

	const Ogre::String& TimeLineComponent::getName() const
	{
		return this->name;
	}

	void TimeLineComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<TimeLineComponent>(TimeLineComponent::getStaticClassId(), TimeLineComponent::getStaticClassName());
	}
	
	void TimeLineComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool TimeLineComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TimePointCount")
		{
			this->timePointCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		// Only create new variant, if fresh loading. If snapshot is done, no new variant
		// must be created! Because the algorithm is working changed flag of each existing variant!
		if (this->gameObjectIds.size() < this->timePointCount->getUInt())
		{
			this->gameObjectIds.resize(this->timePointCount->getUInt());
			this->startTimes.resize(this->timePointCount->getUInt());
			this->durations.resize(this->timePointCount->getUInt());
			this->repeatCounts.resize(this->timePointCount->getUInt());
			this->timePointStartEventNames.resize(this->timePointCount->getUInt());
			this->timePointEndEventNames.resize(this->timePointCount->getUInt());
		}

		for (size_t i = 0; i < this->gameObjectIds.size(); i++)
		{
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "GameObjectId" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->gameObjectIds[i])
				{
					this->gameObjectIds[i] = new Variant(TimeLineComponent::AttrGameObjectId() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribUnsignedLong(propertyElement, "data", 0), this->attributes);
				}
				else
				{
					this->gameObjectIds[i]->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
				this->gameObjectIds[i]->setDescription("Id's of other game objects can be added optionally, which will be activated at a specifig time point and deactivated after a duration of time.");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "StartTime" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->startTimes[i])
				{
					this->startTimes[i] = new Variant(TimeLineComponent::AttrStartTime() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
					this->startTimes[i]->setDescription("Sets the start time in seconds, at which the time point starts. Note: The start point must be higher than its prior start point plus the duration.");
					this->startTimes[i]->addUserData(GameObject::AttrActionNeedRefresh());
					this->setStartTime(i, XMLConverter::getAttribReal(propertyElement, "data"));
				}
				else
				{
					this->startTimes[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Duration" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->durations[i])
				{
					this->durations[i] = new Variant(TimeLineComponent::AttrDuration() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
					this->durations[i]->setDescription("Sets the duration in seconds, after which the next (if exists) time point starts.");
					this->setDurationSec(i, XMLConverter::getAttribReal(propertyElement, "data"));
				}
				else
				{
					this->durations[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RepeatCount" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->repeatCounts[i])
				{
					this->repeatCounts[i] = new Variant(TimeLineComponent::AttrRepeatCount() + Ogre::StringConverter::toString(i), static_cast<unsigned int>(0), this->attributes);
					this->repeatCounts[i]->setDescription("Sets the repeat count. That is, how often will the timePointStartEventName be called until the next time point is active.");
					this->setRepeatCount(i, XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
				}
				else
				{
					this->repeatCounts[i]->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TimePointStartEventName" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->timePointStartEventNames[i])
				{
					this->timePointStartEventNames[i] = new Variant(TimeLineComponent::AttrTimePointStartEventName() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttrib(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->timePointStartEventNames[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
				this->timePointStartEventNames[i]->setDescription("Sets the lua start event name, in order to react when the time point is met. E.g. onStartSpawnEnemyTimePoint()");
				this->timePointStartEventNames[i]->addUserData(GameObject::AttrActionGenerateLuaFunction(), this->timePointStartEventNames[i]->getString() + "(timePointSec)");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TimePointEndEventName" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->timePointEndEventNames[i])
				{
					this->timePointEndEventNames[i] = new Variant(TimeLineComponent::AttrTimePointEndEventName() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttrib(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->timePointEndEventNames[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
				this->timePointEndEventNames[i]->setDescription("Sets the lua end event name, in order to react when the time point is met. Which is start point + duration in seconds. E.g. onEndSpawnEnemyTimePoint()");
				this->timePointEndEventNames[i]->addUserData(GameObject::AttrActionGenerateLuaFunction(), this->timePointEndEventNames[i]->getString() + "(timePointSec)");
				this->timePointEndEventNames[i]->addUserData(GameObject::AttrActionSeparator());
			}
		}
		return true;
	}

	GameObjectCompPtr TimeLineComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool TimeLineComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TimeLineComponent] Init time line component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool TimeLineComponent::connect(void)
	{
		bool success = GameObjectComponent::connect();

		this->timePointIndex = 0;
		this->totalIndex = 0;
		this->timeDt = 0.0f;
		this->gameObjects.clear();
		this->alreadyActivatedList.clear();
		this->globalSubIndex.clear();
		this->callForEndPoint = false;
		this->globalSubIndex.resize(this->gameObjectIds.size(), 0);

		unsigned int sumSubTimePoints = 0;
		for (size_t i = 0; i < this->gameObjectIds.size(); i++)
		{
			// Start point
			sumSubTimePoints += 1;

			// Repeat counts
			if (this->repeatCounts[i]->getUInt() > 1)
				sumSubTimePoints += this->repeatCounts[i]->getUInt() - 1;

			// End point
			sumSubTimePoints += 1;
		}

		this->alreadyActivatedList.resize(sumSubTimePoints, false);

		if (true == this->activated->getBool())
		{
			for (size_t i = 0; i < this->gameObjectIds.size(); i++)
			{
				GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->gameObjectIds[i]->getULong());
				if (nullptr != gameObjectPtr)
				{
					this->gameObjects.emplace_back(gameObjectPtr.get());
				}
				else
				{
					// Push a null game object, because its optionally
					this->gameObjects.emplace_back(nullptr);
				}
			}
		}
		return success;
	}

	bool TimeLineComponent::disconnect(void)
	{
		this->timePointIndex = 0;
		this->globalSubIndex.clear();
		this->totalIndex = 0;
		this->timeDt = 0.0f;
		this->gameObjects.clear();
		this->alreadyActivatedList.clear();
		this->callForEndPoint = false;
		return true;
	}

	void TimeLineComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			this->timeDt += dt;
			if (this->timePointIndex < this->gameObjects.size() && this->totalIndex < this->alreadyActivatedList.size())
			{
				Ogre::Real subTimePoint = this->startTimes[this->timePointIndex]->getReal();
				Ogre::Real stepSec = 0.0f;
				if (this->repeatCounts[this->timePointIndex]->getInt() > 0)
				{
					// get the step in seconds, e.g. if duration = 4 and repeat count = 4, sub start timepoint will be triggered each second
					stepSec = (this->durations[this->timePointIndex]->getReal() / static_cast<Ogre::Real>(this->repeatCounts[this->timePointIndex]->getInt()) * this->globalSubIndex[this->timePointIndex]);
				}

				if (true == this->bShowDebugData)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[TimeLineComponent] Current timepoint Index: " + Ogre::StringConverter::toString(this->timePointIndex)
						+ " current global sub Index: " + Ogre::StringConverter::toString(this->globalSubIndex[this->timePointIndex]) + " current sub time point: " + Ogre::StringConverter::toString(subTimePoint + stepSec));
				}
				if (this->timeDt >= subTimePoint + stepSec && false == this->callForEndPoint)
				{
					// Only activate once
					if (false == this->alreadyActivatedList[this->totalIndex])
					{
						if (true == this->bShowDebugData)
						{
							Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[TimeLineComponent] Trigger start timepoint Index: " + Ogre::StringConverter::toString(this->timePointIndex)
								+ " global sub Index: " + Ogre::StringConverter::toString(this->globalSubIndex[this->timePointIndex]) + " time: " + Ogre::StringConverter::toString(this->timeDt));
						}
						GameObject* gameObject = this->gameObjects[this->timePointIndex];
						if (nullptr != gameObject)
						{
							gameObject->setActivated(true);
						}

						// Call lua function name in script, if it does exist
						if (nullptr != this->gameObjectPtr->getLuaScript() && false == this->timePointStartEventNames[this->timePointIndex]->getString().empty())
						{
							NOWA::AppStateManager::LogicCommand logicCommand = [this]()
							{
								this->gameObjectPtr->getLuaScript()->callTableFunction(this->timePointStartEventNames[this->timePointIndex]->getString(), this->timeDt);
							};
							NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
						}

						this->alreadyActivatedList[this->totalIndex] = true;

						int repeatCount = this->repeatCounts[this->timePointIndex]->getInt() - 1;
						if (repeatCount < 0)
							repeatCount = 0;

						if (this->globalSubIndex[this->timePointIndex] < repeatCount)
						{
							this->globalSubIndex[this->timePointIndex]++;
						}
						else
						{
							// In order to not repeat the start point, disable so that next time end point is called
							this->callForEndPoint = true;
						}
						this->totalIndex++;
					}
				}
				// After the duration is over deactivate again and proceed to next game object by index
				else if (this->timeDt >= this->startTimes[this->timePointIndex]->getReal() + this->durations[this->timePointIndex]->getReal())
				{
					if (false == this->alreadyActivatedList[this->totalIndex])
					{
						if (true == this->bShowDebugData)
						{
							Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[TimeLineComponent] Trigger end timepoint Index: " + Ogre::StringConverter::toString(this->timePointIndex)
								+ " global sub Index: " + Ogre::StringConverter::toString(this->globalSubIndex[this->timePointIndex]) + " time: " + Ogre::StringConverter::toString(this->timeDt));
						}
						GameObject* gameObject = this->gameObjects[this->timePointIndex];
						if (nullptr != gameObject)
						{
							gameObject->setActivated(false);
						}

						// Call lua function name in script, if it does exist
						if (nullptr != this->gameObjectPtr->getLuaScript() && false == this->timePointEndEventNames[this->timePointIndex]->getString().empty())
						{
							NOWA::AppStateManager::LogicCommand logicCommand = [this]()
							{
								this->gameObjectPtr->getLuaScript()->callTableFunction(this->timePointEndEventNames[this->timePointIndex]->getString(), this->timeDt);
							};
							NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
						}

						this->alreadyActivatedList[this->totalIndex] = true;

						this->totalIndex++;
						this->timePointIndex++;
						this->callForEndPoint = false;
					}
				}
			}
		}
	}

	void TimeLineComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (TimeLineComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (TimeLineComponent::AttrTimePointCount() == attribute->getName())
		{
			this->setTimePointCount(attribute->getUInt());
		}
		else
		{
			for (unsigned int i = 0; i < static_cast<unsigned int>(this->gameObjectIds.size()); i++)
			{
				if (TimeLineComponent::AttrGameObjectId() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->gameObjectIds[i]->setValue(attribute->getULong());
				}
				else if (TimeLineComponent::AttrStartTime() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setStartTime(i, attribute->getReal());
				}
				else if (TimeLineComponent::AttrDuration() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setDurationSec(i, attribute->getReal());
				}
				else if (TimeLineComponent::AttrRepeatCount() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setRepeatCount(i, attribute->getUInt());
				}
				else if (TimeLineComponent::AttrTimePointStartEventName() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setTimePointStartEventName(i, attribute->getString());
				}
				else if (TimeLineComponent::AttrTimePointEndEventName() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setTimePointEndEventName(i, attribute->getString());
				}
			}
		}
	}

	void TimeLineComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "TimePointCount"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->timePointCount->getUInt())));
		propertiesXML->append_node(propertyXML);

		for (size_t i = 0; i < this->gameObjectIds.size(); i++)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "GameObjectId" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->gameObjectIds[i]->getULong())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "StartTime" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->startTimes[i]->getReal())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "Duration" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->durations[i]->getReal())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "RepeatCount" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->repeatCounts[i]->getUInt())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "TimePointStartEventName" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->timePointStartEventNames[i]->getString())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "TimePointEndEventName" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->timePointEndEventNames[i]->getString())));
			propertiesXML->append_node(propertyXML);
		}
	}

	Ogre::String TimeLineComponent::getClassName(void) const
	{
		return "TimeLineComponent";
	}

	Ogre::String TimeLineComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void TimeLineComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
	}

	bool TimeLineComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void TimeLineComponent::setTimePointCount(unsigned int timePointCount)
	{
		this->timePointCount->setValue(timePointCount);

		size_t oldSize = this->gameObjectIds.size();

		if (timePointCount > oldSize)
		{
			// Resize the array for count
			this->gameObjectIds.resize(timePointCount);
			this->startTimes.resize(timePointCount);
			this->durations.resize(timePointCount);
			this->repeatCounts.resize(timePointCount);
			this->timePointStartEventNames.resize(timePointCount);
			this->timePointEndEventNames.resize(timePointCount);

			for (size_t i = oldSize; i < this->gameObjectIds.size(); i++)
			{
				this->gameObjectIds[i] = new Variant(TimeLineComponent::AttrGameObjectId() + Ogre::StringConverter::toString(i), static_cast<unsigned long>(0), this->attributes, true);
				this->gameObjectIds[i]->setDescription("Id's of other game objects can be added optionally, which will be activated at a specifig time point and deactivated after a duration of time.");
				this->startTimes[i] = new Variant(TimeLineComponent::AttrStartTime() + Ogre::StringConverter::toString(i), static_cast<Ogre::Real>(i), this->attributes);
				this->startTimes[i]->setDescription("Sets the start time in seconds, at which the time point starts. Note: The start point must be higher than its prior start point plus the duration.");
				this->startTimes[i]->addUserData(GameObject::AttrActionNeedRefresh());
				// Force correct start time adaptation
				this->setStartTime(i, i);
				this->durations[i] = new Variant(TimeLineComponent::AttrDuration() + Ogre::StringConverter::toString(i), static_cast<Ogre::Real>(i), this->attributes);
				this->durations[i]->setDescription("Sets the duration in seconds, after which the next (if exists) time point starts.");
				this->repeatCounts[i] = new Variant(TimeLineComponent::AttrRepeatCount() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
				this->repeatCounts[i]->setDescription("Sets the repeat count. That is, how often will the timePointStartEventName be called until the next time point is active.");

				this->timePointStartEventNames[i] = new Variant(TimeLineComponent::AttrTimePointStartEventName() + Ogre::StringConverter::toString(i), "", this->attributes);
				this->timePointStartEventNames[i]->setDescription("Sets the lua start event name, in order to react when the time point is met. E.g. onStartSpawnEnemyTimePoint()");
				this->timePointStartEventNames[i]->addUserData(GameObject::AttrActionGenerateLuaFunction(), "(timePointSec)");
				// this->timePointStartEventNames[i]->addUserData(GameObject::AttrActionNeedRefresh());

				this->timePointEndEventNames[i] = new Variant(TimeLineComponent::AttrTimePointEndEventName() + Ogre::StringConverter::toString(i), "", this->attributes);
				this->timePointEndEventNames[i]->addUserData(GameObject::AttrActionGenerateLuaFunction(), "(timePointSec)");
				// this->timePointEndEventNames[i]->addUserData(GameObject::AttrActionNeedRefresh());
				this->timePointEndEventNames[i]->addUserData(GameObject::AttrActionSeparator());
			}
		}
		else if (timePointCount < oldSize)
		{
			this->eraseVariants(this->gameObjectIds, timePointCount);
			this->eraseVariants(this->startTimes, timePointCount);
			this->eraseVariants(this->durations, timePointCount);
			this->eraseVariants(this->repeatCounts, timePointCount);
			this->eraseVariants(this->timePointStartEventNames, timePointCount);
			this->eraseVariants(this->timePointEndEventNames, timePointCount);
		}
	}

	unsigned int TimeLineComponent::getTimePointCount(void) const
	{
		return this->timePointCount->getUInt();
	}

	void TimeLineComponent::setGameObjectId(unsigned int index, unsigned long id)
	{
		if (index > this->gameObjectIds.size())
			index = static_cast<unsigned int>(this->gameObjectIds.size()) - 1;
		this->gameObjectIds[index]->setValue(id);
	}

	unsigned long TimeLineComponent::getGameObjectId(unsigned int index)
	{
		if (index > this->gameObjectIds.size())
			return 0;
		return this->gameObjectIds[index]->getULong();
	}

	void TimeLineComponent::setStartTime(unsigned int index, Ogre::Real timePosition)
	{
		if (index > this->startTimes.size())
			index = static_cast<unsigned int>(this->startTimes.size()) - 1;


		if (timePosition < 0.0f)
			timePosition = 0.0f;

		Ogre::Real offsetTime = 0.0f;
		Ogre::Real newStartTime = 0.0f;

		if (index > 0)
			offsetTime = this->startTimes[index - 1]->getReal() + this->durations[index - 1]->getReal();

		if (timePosition >= offsetTime)
			newStartTime = timePosition;
		else
			newStartTime = offsetTime;

		this->startTimes[index]->setValue(newStartTime);
	}

	Ogre::Real TimeLineComponent::getStartTime(unsigned int index)
	{
		if (index > this->startTimes.size())
			return 0;
		return this->startTimes[index]->getReal();
	}

	void TimeLineComponent::setDurationSec(unsigned int index, Ogre::Real duration)
	{
		if (index > this->durations.size())
			index = static_cast<unsigned int>(this->durations.size()) - 1;

		if (duration < 0.0f)
			duration = 0.0f;

		Ogre::Real oldDuration = 0.0f;
		Ogre::Real newDuration = 0.0f;

		/*if (index > 0)
			oldDuration = this->durations[index - 1]->getReal();

		if (duration >= oldDuration)
			newDuration = duration;
		else
			newDuration = oldDuration + 1;*/

		this->durations[index]->setValue(duration /*newDuration*/);
	}

	Ogre::Real TimeLineComponent::getDurationSec(unsigned int index)
	{
		if (index > this->durations.size())
			return 0;
		return this->durations[index]->getReal();
	}

	void TimeLineComponent::setRepeatCount(unsigned int index, unsigned int repeatCount)
	{
		if (index > this->repeatCounts.size())
			index = static_cast<unsigned int>(this->repeatCounts.size()) - 1;

		// Allow max 2 times a second
		if (repeatCount > this->durations[index]->getReal() * 2)
			repeatCount = this->durations[index]->getReal() * 2;

		this->repeatCounts[index]->setValue(repeatCount);
	}

	unsigned int TimeLineComponent::getRepeatCount(unsigned int index)
	{
		if (index > this->repeatCounts.size())
			return 0;
		return this->repeatCounts[index]->getUInt();
	}

	void TimeLineComponent::setTimePointStartEventName(unsigned int index, const Ogre::String& timePointStartEventName)
	{
		if (index > this->timePointStartEventNames.size())
			index = static_cast<unsigned int>(this->timePointStartEventNames.size()) - 1;
		this->timePointStartEventNames[index]->setValue(timePointStartEventName);
		this->timePointStartEventNames[index]->addUserData(GameObject::AttrActionGenerateLuaFunction(), timePointStartEventName + "(timePointSec)");
	}

	Ogre::String TimeLineComponent::getTimePointStartEventName(unsigned int index)
	{
		if (index > this->timePointStartEventNames.size())
			return "";
		return this->timePointStartEventNames[index]->getString();
	}

	void TimeLineComponent::setTimePointEndEventName(unsigned int index, const Ogre::String& timePointEndEventName)
	{
		if (index > this->timePointEndEventNames.size())
			index = static_cast<unsigned int>(this->timePointEndEventNames.size()) - 1;
		this->timePointEndEventNames[index]->setValue(timePointEndEventName);
		this->timePointEndEventNames[index]->addUserData(GameObject::AttrActionGenerateLuaFunction(), timePointEndEventName + "(timePointSec)");
	}

	Ogre::String TimeLineComponent::getTimePointEndEventName(unsigned int index)
	{
		if (index > this->timePointEndEventNames.size())
			return "";
		return this->timePointEndEventNames[index]->getString();
	}

	bool TimeLineComponent::setCurrentTimeSec(Ogre::Real seconds)
	{
		this->connect();

		if (seconds < 0.0f)
			seconds = 0.0f;
		this->timeDt = seconds;

		Ogre::Real sumDurations = 0.0f;
		unsigned int sumSubTimePoints = 0;
		for (size_t i = 0; i < this->durations.size(); i++)
		{
			// Note subTimePoint is absolute!
			Ogre::Real subTimePoint = this->startTimes[i]->getReal();
			Ogre::Real sumDurations = subTimePoint;

			// Found the time point
			if (sumDurations >= seconds)
			{
				// Found matching time point index (nearest time point)
				this->timePointIndex = i;
				this->totalIndex = sumSubTimePoints;
				return true;
			}

			// Start point
			sumSubTimePoints += 1;

			if (this->repeatCounts[i]->getInt() > 0)
			{
				// removed -1, because the end point also counts
				for (size_t j = 0; j < this->repeatCounts[i]->getInt(); j++)
				{
					// Get the step in seconds, e.g. if duration = 4 and repeat count = 4, sub start timepoint will be triggered each second
					Ogre::Real stepSec = (this->durations[i]->getReal() / static_cast<Ogre::Real>(this->repeatCounts[i]->getInt()));

					sumDurations += stepSec;

					// Found the time point
					if (sumDurations >= seconds)
					{
						// Found matching time point index (nearest time point)
						this->timePointIndex = i;
						this->totalIndex = sumSubTimePoints;
						return true;
					}

					// Repeat count + end point
					sumSubTimePoints += 1;
				}
			}
			else
			{
				// No repeat counts, get the whole duration
				sumSubTimePoints += 1;
				sumDurations += this->durations[i]->getReal();
			}
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[TimeLineComponent] The given time of " + Ogre::StringConverter::toString(seconds)
			+ " exceeds the time line! Hence no time points will be triggered for game object: " + this->gameObjectPtr->getName());
		return false;
	}

	Ogre::Real TimeLineComponent::getCurrentTimeSec(void) const
	{
		return this->timeDt;
	}

	Ogre::Real TimeLineComponent::getMaxTimeLineDuration(void)
	{
		Ogre::Real sumDurations = 0.0f;
		for (size_t i = 0; i < this->durations.size(); i++)
		{
			// Note subTimePoint is absolute!
			Ogre::Real subTimePoint = this->startTimes[i]->getReal();
			Ogre::Real sumDurations = subTimePoint;

			if (this->repeatCounts[i]->getInt() > 0)
			{
				// removed -1, because the end point also counts
				for (size_t j = 0; j < this->repeatCounts[i]->getInt(); j++)
				{
					// Get the step in seconds, e.g. if duration = 4 and repeat count = 4, sub start timepoint will be triggered each second
					Ogre::Real stepSec = (this->durations[i]->getReal() / static_cast<Ogre::Real>(this->repeatCounts[i]->getInt()));

					sumDurations += stepSec;
				}
			}
			else
			{
				sumDurations += this->durations[i]->getReal();
			}
		}
		return sumDurations;
	}

	// Lua registration part

	TimeLineComponent* getTimeLineComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<TimeLineComponent>(gameObject->getComponentWithOccurrence<TimeLineComponent>(occurrenceIndex)).get();
	}

	TimeLineComponent* getTimeLineComponent(GameObject* gameObject)
	{
		return makeStrongPtr<TimeLineComponent>(gameObject->getComponent<TimeLineComponent>()).get();
	}

	TimeLineComponent* getTimeLineComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<TimeLineComponent>(gameObject->getComponentFromName<TimeLineComponent>(name)).get();
	}

	void TimeLineComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<TimeLineComponent, GameObjectComponent>("TimeLineComponent")
				.def("setActivated", &TimeLineComponent::setActivated)
				.def("isActivated", &TimeLineComponent::isActivated)
				.def("setCurrentTimeSec", &TimeLineComponent::setCurrentTimeSec)
				.def("getCurrentTimeSec", &TimeLineComponent::getCurrentTimeSec)
				.def("getMaxTimeLineDuration", &TimeLineComponent::getMaxTimeLineDuration)
		];

		LuaScriptApi::getInstance()->addClassToCollection("TimeLineComponent", "class inherits GameObjectComponent", TimeLineComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("TimeLineComponent", "void setActivated(bool activated)", "Sets whether time line can start or not.");
		LuaScriptApi::getInstance()->addClassToCollection("TimeLineComponent", "bool isActivated()", "Gets whether this time line is activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("TimeLineComponent", "bool setCurrentTimeSec(float timeSec)", "Sets the current time in seconds. Note: The next time point is determined, and the corresponding game object or lua function (if existing) called. Note: If the given time exceeds the overwhole time line duration, false is returned.");
		LuaScriptApi::getInstance()->addClassToCollection("TimeLineComponent", "float getCurrentTimeSec()", "Gets the current time in seconds, since this component is running.");
		LuaScriptApi::getInstance()->addClassToCollection("TimeLineComponent", "float getMaxTimeLineDuration()", "Calculates and gets the maximum time line duration in seconds. Note: Do not call this to often, because the max time is calculated each time from the scratch, in order to be as flexible as possible. E.g. a time point has been removed during runtime.");

		gameObjectClass.def("getTimeLineComponentFromName", &getTimeLineComponentFromName);
		gameObjectClass.def("getTimeLineComponent", (TimeLineComponent * (*)(GameObject*)) & getTimeLineComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getTimeLineComponentFromIndex", (TimeLineComponent * (*)(GameObject*, unsigned int)) & getTimeLineComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "TimeLineComponent getTimeLineComponentFromIndex(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "TimeLineComponent getTimeLineComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "TimeLineComponent getTimeLineComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castTimeLineComponent", &GameObjectController::cast<TimeLineComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "TimeLineComponent castTimeLineComponent(TimeLineComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool TimeLineComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Can only be added once
		if (gameObject->getComponentCount<TimeLineComponent>() < 2)
		{
			return true;
		}
	}

}; //namespace end