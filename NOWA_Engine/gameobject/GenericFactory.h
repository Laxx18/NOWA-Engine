#ifndef GENERIC_FACTORY_H
#define GENERIC_FACTORY_H

#include "defines.h"
#include <map>
#include <set>
#include <boost/weak_ptr.hpp>
#include <boost/make_shared.hpp>

class GameObject;
class GameObjectController;

struct lua_state;

namespace NOWA
{
	template <class BaseType, class SubType>
	BaseType* genericObjectCreationFunction(void)
	{ 
		return new SubType;
	}

	template <class SubType>
	bool genericCanAddFunction(GameObject* gameObject)
	{ 
		return SubType::canStaticAddComponent(gameObject);
	}

	template <class SubType>
	void genericCreateApiForLuaFunction(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController)
	{ 
		return SubType::createStaticApiForLua(lua, gameObject, gameObjectController);
	}

	template <class BaseClass, class IdType>
	class EXPORTED GenericObjectFactory
	{
		typedef bool (*CanAddFunction)(GameObject* gameObject);

		typedef BaseClass* (*ObjectCreationFunction)(void);

		typedef void (*CreateForLuaApiFunction)(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController);
	public:

		struct ComponentProperties
		{
			Ogre::String infoText;
			bool isPlugin = false;
		};
	public:
		template <class SubClass>
		bool registerClass(IdType id, const Ogre::String& name)
		{
			GenericObjectFactory::ComponentProperties componentProperties;
			componentProperties.isPlugin = false;
			componentProperties.infoText = SubClass::getStaticInfoText();
			this->registeredComponentNames.emplace(name, componentProperties);
			auto findIt = this->creationFunctions.find(id);
			if (findIt == this->creationFunctions.end())
			{
				this->canAddFunctions[id] = &genericCanAddFunction<SubClass>;
				this->creationFunctions[id] = &genericObjectCreationFunction<BaseClass, SubClass>;  // insert() is giving compiler errors
				this->createApiForLuaFunctions[id] = &genericCreateApiForLuaFunction<SubClass>;
				return true;
			}

			return false;
		}

		template <class SubClass>
		bool registerPluginComponentClass(IdType id, const Ogre::String& name)
		{
			GenericObjectFactory::ComponentProperties componentProperties;
			componentProperties.isPlugin = true;
			componentProperties.infoText = SubClass::getStaticInfoText();

			this->registeredComponentNames.emplace(name, componentProperties);
			auto findIt = this->creationFunctions.find(id);
			if (findIt == this->creationFunctions.end())
			{
				this->canAddFunctions[id] = &genericCanAddFunction<SubClass>;
				this->creationFunctions[id] = &genericObjectCreationFunction<BaseClass, SubClass>;  // insert() is giving compiler errors
				this->createApiForLuaFunctions[id] = &genericCreateApiForLuaFunction<SubClass>;
				return true;
			}

			return false;
		}

		bool hasComponent(const Ogre::String& componentName)
		{
			auto it = this->registeredComponentNames.find(componentName);
			if (it != this->registeredComponentNames.cend())
			{
				return true;
			}
			return false;
		}

		Ogre::String getComponentInfoText(const Ogre::String& name)
		{
			auto it = this->registeredComponentNames.find(name);
			if (it != this->registeredComponentNames.cend())
			{
				return it->second.infoText;
			}

			return "";
		}

		bool getComponentIsPlugin(const Ogre::String& name)
		{
			auto it = this->registeredComponentNames.find(name);
			if (it != this->registeredComponentNames.cend())
			{
				return it->second.isPlugin;
			}

			return false;
		}

		BaseClass* create(IdType id)
		{
			if (id == 0)
			{
				return nullptr;
			}
			auto findIt = this->creationFunctions.find(id);
			if (findIt != this->creationFunctions.end())
			{
				ObjectCreationFunction pFunc = findIt->second;
				return pFunc();
			}
			return nullptr;
		}

		bool canAddComponent(IdType id, GameObject* gameObject)
		{
			if (id == 0)
			{
				return true;
			}
			auto findIt = this->canAddFunctions.find(id);
			if (findIt != this->canAddFunctions.end())
			{
				CanAddFunction pFunc = findIt->second;
				return pFunc(gameObject);
			}

			return true;
		}

		void createForLuaApi(IdType id, lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController)
		{
			if (id == 0)
			{
				return;
			}
			auto findIt = this->createApiForLuaFunctions.find(id);
			if (findIt != this->createApiForLuaFunctions.end())
			{
				CreateForLuaApiFunction pFunc = findIt->second;

				pFunc(lua, gameObject, gameObjectController);
				return;
			}
		}

		const std::map<Ogre::String, GenericObjectFactory::ComponentProperties>& getRegisteredComponentNames(void) const
		{
			return this->registeredComponentNames;
		}
	private:
		std::map<IdType, ObjectCreationFunction> creationFunctions;
		std::map<IdType, CanAddFunction> canAddFunctions;
		std::map<IdType, CreateForLuaApiFunction> createApiForLuaFunctions;
		std::map<IdType, BaseClass*> pluginComponents;

		std::map<Ogre::String, GenericObjectFactory::ComponentProperties> registeredComponentNames;
	};

}; //Namespace end

#endif