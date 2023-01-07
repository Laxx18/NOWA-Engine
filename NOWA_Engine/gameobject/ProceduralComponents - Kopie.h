#ifndef PROCEDURAL_COMPONENTS_H
#define PROCEDURAL_COMPONENTS_H

#include "GameObjectComponent.h"
#include "Procedural.h"

namespace NOWA
{
	class EXPORTED ProceduralComponent : public GameObjectComponent
	{
	public:
		typedef boost::shared_ptr<ProceduralComponent> ProceduralCompPtr;
	public:

		ProceduralComponent();

		virtual ~ProceduralComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		/**
		* @see		GameObjectComponent::postInit
		*/
		virtual bool postInit(void) override;

		/**
		* @see		GameObjectComponent::connect
		*/
		virtual bool connect(void) override;

		/**
		* @see		GameObjectComponent::disconnect
		*/
		virtual bool disconnect(void) override;

		/**
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void) override;

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
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) = 0;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("ProceduralComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "ProceduralComponent";
		}

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;
		
		void setName(const Ogre::String& name);

		Ogre::String getName(void) const;
		
		/**
		 * @brief	Gets the unique id of this procedural component instance
		 * @return	id		The id of this procedural component instance
		 */
		unsigned long getId(void) const;

		Ogre::v1::MeshPtr getMeshPtr(void) const;
		
		Procedural::TriangleBuffer& getTriangleBuffer(void);

		virtual bool make(bool forTriangleBuffer);
	public:
		static const Ogre::String AttrName(void) { return "Name"; }
		static const Ogre::String AttrId(void) { return "Id"; }
	protected:
		Variant* name;
		Variant* id;
		Ogre::v1::MeshPtr meshPtr; // could be dangerous, as this ptr is shared amongs x-components for one game object
		Procedural::TriangleBuffer triangleBuffer;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED ProceduralPrimitiveComponent : public ProceduralComponent
	{
	public:

		typedef boost::shared_ptr<ProceduralPrimitiveComponent> ProceduralPrimitiveCompPtr;
	public:

		ProceduralPrimitiveComponent();

		virtual ~ProceduralPrimitiveComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		/**
		* @see		GameObjectComponent::postInit
		*/
		virtual bool postInit(void) override;
		
		/**
		* @see		GameObjectComponent::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		/**
		* @see		GameObjectComponent::connect
		*/
		virtual bool connect(void) override;

		/**
		* @see		GameObjectComponent::disconnect
		*/
		virtual bool disconnect(void) override;

		/**
		* @see		GameObjectComponent::getClassName
		*/
		virtual Ogre::String getClassName(void) const override;

		/**
		* @see		GameObjectComponent::getParentClassName
		*/
		virtual Ogre::String getParentClassName(void) const override;



		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("ProceduralPrimitiveComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "ProceduralPrimitiveComponent";
		}

		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;

		virtual bool make(bool forTriangleBuffer) override;

		void setDataBlockName(const Ogre::String& dataBlockName);

		Ogre::String getDataBlockName(void) const;
		
		void setUVTile(const Ogre::Vector2& uvTile);
		
		Ogre::Vector2 getUVTile(void) const;

		void setEnableNormals(bool enable);
		
		bool getEnableNormals(void) const;
		
		void setNumTexCoords(unsigned int numTexCoords);
		
		unsigned int getNumTexCoords(void) const;
		
		void setUVOrigin(const Ogre::Vector2& uvOrigin);
		
		Ogre::Vector2 getUVOrigin(void) const;
		
		void setSwitchUV(bool switchUV);
		
		bool getSwitchUV(void) const;
		
		void setPosition(const Ogre::Vector3& position);
		
		Ogre::Vector3 getPosition(void) const;
		
		void setOrientation(const Ogre::Quaternion& orientation);
		
		Ogre::Quaternion getOrientation(void) const;
		
		void setScale(const Ogre::Vector3& scale);
		
		Ogre::Vector3 getUVScale(void) const;
		
		void setShapeType(const Ogre::String& shapeType);
		
		Ogre::String getShapeType(void) const;
		
		/**
		 * @brief For Box, RoundedBox
		 */
		void setSizeXYZ(const Ogre::Vector3& size);
		
		Ogre::Vector3 getSizeXYZ(void) const;
		
		/**
		 * @brief For Box, RoundedBox
		 */
		void setNumSegXYZ(const Ogre::Vector3& segments);
		
		Ogre::Vector3 getNumSegXYZ(void) const;
		
		/**
		 * @brief For Capsule, Sphere
		 */
		void setNumRings(unsigned int numRings);
		
		unsigned int getNumRings(void) const;
		
		/**
		 * @brief For Capsule, Cone, Cylinder, Sphere, Tube
		 */
		void setNumSegments(unsigned int numSegments);
		
		unsigned int getNumSegments(void) const;
		
		/**
		 * @brief For Capsule, Cone, Cylinder, Prism, Tube
		 */
		void setNumSegHeight(unsigned int numSegHeight);
		
		unsigned int getNumSegHeight(void) const;
		
		/**
		 * @brief For Capsule, Cone, Cylinder, IcoSphere, Prism, Sphere, Spring, Torus, TorusKnot
		 */
		void setRadius(Ogre::Real radius);
		
		Ogre::Real getRadius(void) const;
		
		/**
		 * @brief For Capsule, Cone, Cylinder, Prism, Spring, Tube
		 */
		void setHeight(Ogre::Real height);
		
		Ogre::Real getHeight(void) const;
		
		/**
		 * @brief For Cylinder, Prism
		 */
		void setCapped(bool capped);
		
		bool getCapped(void) const;
		
		/**
		 * @brief For IcoSphere
		 */
		void setNumIterations(unsigned int numIterations);
		
		unsigned int getNumIterations(void) const;
		
		/**
		 * @brief For Plane
		 */
		void setNumSegXY(const Ogre::Vector2& numSeg);
		
		Ogre::Vector2 getNumSegXY(void) const;
		
		/**
		 * @brief For Plane
		 */
		void setNormalXYZ(const Ogre::Vector3& normal);
		
		Ogre::Vector3 getNormalXYZ(void) const;
		
		/**
		 * @brief For Plane
		 */
		void setSizeXY(const Ogre::Vector2& size);
		
		Ogre::Vector2 getSizeXY(void) const;
		
		/**
		 * @brief For Prism
		 */
		void setNumSides(unsigned int numSides);
		
		unsigned int getNumSides(void) const;
		
		/**
		 * @brief For RoundedBox
		 */
		void setChamferSize(Ogre::Real chamferSize);
		
		Ogre::Real getChamferSize(void) const;
		
		/**
		 * @brief For RoundedBox
		 */
		void setChamferNumSeg(unsigned int chamferNumSeg);
		
		unsigned int getChamferNumSeg(void) const;
		
		/**
		 * @brief For Spring
		 */
		void setNumSegPath(unsigned int numSegPath);
		
		unsigned int getNumSegPath(void) const;
		
		/**
		 * @brief For Spring
		 */
		void setNumRounds(unsigned int numRounds);
		
		unsigned int getNumRounds(void) const;
		
		/**
		 * @brief For Torus, TorusKnot
		 */
		void setNumSegSection(unsigned int numSegSection);
		
		unsigned int getNumSegSection(void) const;
		
		/**
		 * @brief For Torus, TorusKnot
		 */
		void setNumSegCircle(unsigned int numSegCircle);
		
		unsigned int getNumSegCircle(void) const;
		
		/**
		 * @brief For Torus, TorusKnot
		 */
		void setSectionRadius(Ogre::Real sectionRadius);
		
		Ogre::Real getSectionRadius(void) const;
		
		/**
		 * @brief For TorusKnot
		 */
		void setTorusKnotPQ(const Ogre::Vector2& qp);
		
		Ogre::Vector2 getTorusKnotPQ(void) const;
		
		/**
		 * @brief For Tube
		 */
		void setOuterInnerRadius(const Ogre::Vector2& outerInnerRadius);
		
		Ogre::Vector2 getOuterInnerRadius(void) const;
	private:
		void exportMesh(void);
	public:
		static const Ogre::String AttrDataBlockName(void) { return "Datablock"; }
		static const Ogre::String AttrExportMesh(void) { return "Export Mesh"; }
		static const Ogre::String AttrUVTile(void) { return "U,V-Tile"; }
		static const Ogre::String AttrEnableNormals(void) { return "Enable Normals"; }
		static const Ogre::String AttrNumTexCoords(void) { return "Num. Tex. Coords."; }
		static const Ogre::String AttrUVOrigin(void) { return "U,V Origin"; }
		static const Ogre::String AttrSwitchUV(void) { return "Switch U,V"; }
		static const Ogre::String AttrPosition(void) { return "Position"; }
		static const Ogre::String AttrOrientation(void) { return "Orientation"; }
		static const Ogre::String AttrScale(void) { return "Scale"; }
		static const Ogre::String AttrShapeType(void) { return "Shape Type"; }
		
		static const Ogre::String AttrSizeXYZ(void) { return "Size X,Y,Z"; }
		static const Ogre::String AttrNumSegXYZ(void) { return "Num. Seg. X,Y,Z"; }
		static const Ogre::String AttrNumRings(void) { return "Num. Rings"; }
		static const Ogre::String AttrNumSegments(void) { return "Num. Segments"; }
		static const Ogre::String AttrNumSegHeight(void) { return "Num. Seg. Height"; }
		static const Ogre::String AttrRadius(void) { return "Radius"; }
		static const Ogre::String AttrHeight(void) { return "Height"; }
		static const Ogre::String AttrCapped(void) { return "Capped"; }
		static const Ogre::String AttrNumIterations(void) { return "Num. Iterations"; }
		static const Ogre::String AttrNumSegXY(void) { return "Num. Seg. X,Y"; }
		static const Ogre::String AttrNormalXYZ(void) { return "Normal X,Y,Z"; }
		static const Ogre::String AttrSizeXY(void) { return "Size X,Y"; }
		static const Ogre::String AttrNumSides(void) { return "Num. Sides"; }
		static const Ogre::String AttrChamferSize(void) { return "Chamfer Size"; }
		static const Ogre::String AttrChamferNumSeg(void) { return "Chamfer Num. Seg."; }
		static const Ogre::String AttrNumSegPath(void) { return "Num. Seg. Path"; }
		static const Ogre::String AttrNumRounds(void) { return "Num. Rounds"; }
		static const Ogre::String AttrNumSegSection(void) { return "Num. Seg. Section"; }
		static const Ogre::String AttrNumSegCircle(void) { return "Num. Seg. Circle"; }
		static const Ogre::String AttrSectionRadius(void) { return "Section Radius"; }
		static const Ogre::String AttrTorusKnotPQ(void) { return "Torus Knot-P,Q"; }
		
		static const Ogre::String AttrOuterInnerRadius(void) { return "Outer-Inner Radius"; }
		
		static const Ogre::String AttrMake(void) { return "Make"; }
	private:
		Variant* makeButton;
		Variant* exportMeshButton;
		Variant* dataBlockName;
		Variant* shapeType;

		Variant* uvTile;
		Variant* enableNormals;
		Variant* numTexCoords;
		Variant* uvOrigin;
		Variant* switchUV;
		Variant* position;
		Variant* orientation;
		Variant* scale;
	
		Variant* sizeXYZ; // Box, RoundedBox
		Variant* numSegXYZ; // Box, RoundedBox
		Variant* numRings; // Capsule, Sphere
		Variant* numSegments; // Capsule, Cone, Cylinder, Sphere, Tube
		Variant* numSegHeight; // Capsule, Cone, Cylinder, Prism, Tube
		Variant* radius; // Capsule, Cone, Cylinder, IcoSphere, Prism, Sphere, Spring, Torus, TorusKnot
		Variant* height; // Capsule, Cone, Cylinder, Prism, Spring, Tube
		Variant* capped; // Cylinder, Prism
		Variant* numIterations; // IcoSphere
		Variant* numSegXY; // Plane
		Variant* normalXYZ; // Plane
		Variant* sizeXY; // Plane
		Variant* numSides; // Prism
		Variant* chamferSize; // RoundedBox
		Variant* chamferNumSeg; // RoundedBox
		Variant* numSegPath; // Spring
		Variant* numRounds; // Spring
		Variant* numSegSection; // Torus, TorusKnot
		Variant* numSegCircle; // Torus, TorusKnot
		Variant* sectionRadius; // Torus, TorusKnot
		Variant* torusKnotPQ; // TorusKnot
		Variant* outerInnerRadius; // Tube
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED ProceduralBooleanComponent : public ProceduralComponent
	{
	public:

		typedef boost::shared_ptr<ProceduralBooleanComponent> ProceduralBooleanCompPtr;
	public:

		ProceduralBooleanComponent();

		virtual ~ProceduralBooleanComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		/**
		* @see		GameObjectComponent::postInit
		*/
		virtual bool postInit(void) override;

		/**
		* @see		GameObjectComponent::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		/**
		* @see		GameObjectComponent::connect
		*/
		virtual bool connect(void) override;

		/**
		* @see		GameObjectComponent::disconnect
		*/
		virtual bool disconnect(void) override;

		/**
		* @see		GameObjectComponent::onCloned
		*/
		virtual bool onCloned(void) override;

		/**
		* @see		GameObjectComponent::getClassName
		*/
		virtual Ogre::String getClassName(void) const override;

		/**
		* @see		GameObjectComponent::getParentClassName
		*/
		virtual Ogre::String getParentClassName(void) const override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("ProceduralBooleanComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "ProceduralBooleanComponent";
		}

		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;

		virtual bool make(bool forTriangleBuffer) override;

		void setTargetId1(unsigned long targetId1);

		unsigned long getTargetId1(void) const;

		void setTargetId2(unsigned long targetId2);

		unsigned long getTargetId2(void) const;

		void setBoolOperation(const Ogre::String& boolOperation);

		Ogre::String getBoolOperation(void) const;

		void setDataBlockName(const Ogre::String& dataBlockName);

		Ogre::String getDataBlockName(void) const;
	private:
		void exportMesh(void);
	public:
		static const Ogre::String AttrMake(void) { return "Make"; }
		static const Ogre::String AttrDataBlockName(void) { return "Datablock"; }
		static const Ogre::String AttrTargetId1(void) { return "Target Id 1"; }
		static const Ogre::String AttrTargetId2(void) { return "Target Id 2"; }
		static const Ogre::String AttrBooleanOperation(void) { return "Bool-Operation"; }
		
	private:
		Variant* makeButton;
		Variant* exportMeshButton;
		Variant* dataBlockName;
		Variant* targetId1;
		Variant* targetId2;
		Variant* boolOperation;
	};

}; //namespace end

#endif