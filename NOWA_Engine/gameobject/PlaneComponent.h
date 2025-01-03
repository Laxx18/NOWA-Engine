#ifndef PLANE_COMPONENT_H
#define PLANE_COMPONENT_H

#include "GameObjectComponent.h"

namespace NOWA
{
	class EXPORTED PlaneComponent : public GameObjectComponent
	{
	public:
		typedef boost::shared_ptr<NOWA::PlaneComponent> PlaneCompPtr;
	public:
	
		PlaneComponent();

		virtual ~PlaneComponent();

		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		virtual bool postInit(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("PlaneComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "PlaneComponent";
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
			return "Usage: This component is used to create a plane.";
		}

		virtual void update(Ogre::Real dt, bool notSimulating) override { };

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		void setDistance(Ogre::Real distance);

		Ogre::Real getDistance(void) const;

		void setWidth(Ogre::Real width);

		Ogre::Real getWidth(void) const;

		void setHeight(Ogre::Real height);

		Ogre::Real getHeight(void) const;

		void setXSegments(int xSegments);

		int getXSegments(void) const;

		void setYSegments(int ySegments);

		int getYSegments(void) const;

		void setNumTexCoordSets(int numTexCoordSets);

		int getNumTexCoordSets(void) const;

		void setUTile(Ogre::Real uTile);

		Ogre::Real getUTile(void) const;

		void setVTile(Ogre::Real vTile);

		Ogre::Real getVTile(void) const;

		void setNormal(const Ogre::Vector3& normal);

		Ogre::Vector3 getNormal(void) const;

		void setUp(const Ogre::Vector3& up);

		Ogre::Vector3 getUp(void) const;

		virtual void setDatablockName(const Ogre::String& datablockName);

		virtual Ogre::String getDatablockName(void) const;
	public:
		static const Ogre::String AttrDistance(void) { return "Distance"; }
		static const Ogre::String AttrWidth(void) { return "Width"; }
		static const Ogre::String AttrHeight(void) { return "Height"; }
		static const Ogre::String AttrXSegments(void) { return "X-Segments"; }
		static const Ogre::String AttrYSegments(void) { return "Y-Segments"; }
		static const Ogre::String AttrNumTexCoordSets(void) { return "Num-TexCoordSets"; }
		static const Ogre::String AttrUTile(void) { return "U-Tile"; }
		static const Ogre::String AttrVTile(void) { return "V-Tile"; }
		static const Ogre::String AttrMaterialFile(void) { return "Material"; }
		static const Ogre::String AttrHasNormals(void) { return "Normals"; }
		static const Ogre::String AttrNormal(void) { return "Normal"; }
		static const Ogre::String AttrUp(void) { return "Up"; }
	private:
		void createPlane(void);
	protected:
		Variant* distance;
		Variant* width;
		Variant* height;
		Variant* xSegments;
		Variant* ySegments;
		Variant* numTexCoordSets;
		Variant* uTile;
		Variant* vTile;
		Variant* hasNormals;
		Variant* normal;
		Variant* up;
		Ogre::String dataBlockName;
		Ogre::HlmsDatablock* clonedDatablock;
	};

}; //namespace end

#endif