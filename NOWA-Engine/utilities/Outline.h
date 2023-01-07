#ifndef OUTLINE_H
#define OUTLINE_H

#include "NOWAPrecompiled.h"
#include "defines.h"
#include "gameobject/gameobject.h"

namespace NOWA
{

	// http://katyscode.wordpress.com/2013/08/22/c-polymorphic-cloning-and-the-crtp-curiously-recurring-template-pattern/
	// http://eli.thegreenplace.net/2011/05/17/the-curiously-recurring-template-pattern-in-c/

	class DefaultOutLine;

	template <typename Strategy = DefaultOutLine>
	class EXPORTED OutLine
	{
	public:
		void highlight(GameObject* gameObject)
		{
			static_cast<Strategy*>(this)->highlight(gameObject);
		}

		void unHighlight(GameObject* gameObject)
		{
			static_cast<Strategy*>(this)->unHighlight(gameObject);
		}
	};

	class EXPORTED DefaultOutLine : public OutLine<DefaultOutLine>
	{
	public:
		void highlight(GameObject* gameObject)
		{
			gameObject->showBoundingBox(true);
		}

		void unHighlight(GameObject* gameObject)
		{
			gameObject->showBoundingBox(false);
		}
	};

	class EXPORTED RimEffectOutLine : public OutLine<RimEffectOutLine>
	{
	public:
		void highlight(GameObject* gameObject)
		{
			/*unsigned short count = entity->getNumSubEntities();

			const Ogre::String fileName = "rim.dds";
			const Ogre::String rimMaterialName = "_rim";

			for (unsigned short i = 0; i < count; ++i)
			{
				Ogre::SubEntity* subentity = entity->getSubEntity(i);

				const Ogre::String& oldMaterialName = subentity->getMaterialName();
				Ogre::String newMaterialName = oldMaterialName + rimMaterialName;

				Ogre::MaterialPtr newMaterial = Ogre::MaterialManager::getSingleton().getByName(newMaterialName);

				if (newMaterial.isNull())
				{
					Ogre::MaterialPtr oldMaterial = Ogre::MaterialManager::getSingleton().getByName(oldMaterialName);
					newMaterial = oldMaterial->clone(newMaterialName);

					Ogre::Pass* pass = newMaterial->getTechnique(0)->getPass(0);
					Ogre::TextureUnitState* texture = pass->createTextureUnitState();
					texture->setCubicTextureName(&fileName, true);
					texture->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);
					texture->setColourOperationEx(Ogre::LBX_ADD, Ogre::LBS_TEXTURE, Ogre::LBS_CURRENT);
					texture->setColourOpMultipassFallback (Ogre::SBF_ONE, Ogre::SBF_ONE);
					texture->setEnvironmentMap(true, Ogre::TextureUnitState::ENV_NORMAL);
				}

				subentity->setMaterial(newMaterial);
				// Ogre::MaterialSerializer mats;
				// mats.queueForExport(newMaterial);
				// mats.exportQueued("test.material", true);
				// mats.exportMaterial(newMaterial, "test.material");
			}*/
			
		}

		void unHighlight(GameObject* gameObject)
		{
			//unsigned short count = entity->getNumSubEntities();

			//for (unsigned short i = 0; i < count; ++i)
			//{

			//	Ogre::SubEntity* subentity = entity->getSubEntity (i);
			//	Ogre::SubMesh* submesh = subentity->getSubMesh();

			//	const Ogre::String& oldMaterialName = submesh->getMaterialName();
			//	const Ogre::String& newMaterialName = subentity->getMaterialName();

			//	// if the entity is already using the original material then we're done. 
			//	if (0 == stricmp(oldMaterialName.c_str(), newMaterialName.c_str()))
			//	{
			//		continue;
			//	}
			//	// otherwise restore the original material name.
			//	subentity->setMaterialName(oldMaterialName);
			//}
		}
	};

};  //namespace end

#endif