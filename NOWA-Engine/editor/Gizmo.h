#ifndef GIZMO_H
#define GIZMO_H

#include "defines.h"
#include "utilities/ObjectTitle.h"

namespace NOWA
{
	class Ogre::SceneManager;
	class Ogre::Camera;
	class Ogre::Viewport;
	class Ogre::SceneNode;
	class GameObject;

	class EXPORTED Gizmo
	{
	public:
		enum eGizmoState
		{
			GIZMO_NONE = 0,
			GIZMO_ARROW_X = 1,
			GIZMO_ARROW_Y = 2,
			GIZMO_ARROW_Z = 3,
			GIZMO_SPHERE = 4
		};

		Gizmo();
		~Gizmo();

		/**
		* @brief		Initializes the gizmo
		* @param[in]	sceneManager			The ogre scene manager to use
		* @param[in]	camera					The ogre camera to use
		* @param[in]	thickness				The thickness of the arrows
		* @param[in]	materialNameX			The material name for x arrow
		* @param[in]	materialNameY			The material name for y arrow
		* @param[in]	materialNameZ			The material name for z arrow
		* @param[in]	materialNameHighlight	The material name for highlight
		* @Note			When the gizmo is used, 4 categories (GizmoX, GizmoY, GizmoZ, GizmoSphere), will be occupied in the GameObjectController. So overall, there are 4 categories less available.
		*/
		void init(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, Ogre::Real thickness = 0.6f, 
			const Ogre::String& materialNameX = "BaseRedLine", const Ogre::String& materialNameY = "BaseGreenLine", const Ogre::String& materialNameZ = "BaseBlueLine",
			const Ogre::String& materialNameHighlight = "BaseYellowLine");

		void update(void);

		void changeToTranslateGizmo(void);

		void changeToScaleGizmo(void);

		void changeToRotateGizmo(void);

		void setEnabled(bool enable);

		bool isEnabled() const;

		void setSphereEnabled(bool enable);

		void highlightXArrow(void);

		void highlightYArrow(void);

		void highlightZArrow(void);

		void highlightSphere(void);

		void unHighlightGizmo(void);

		Ogre::Real getGizmoBoundingRadius(void) const;

		Ogre::Plane& getFirstPlane(void);
		
		Ogre::Plane& getSecondPlane(void);
		
		Ogre::Plane& getThirdPlane(void);

		void redefineFirstPlane(const Ogre::Vector3& normal, const Ogre::Vector3& position);

		void redefineSecondPlane(const Ogre::Vector3& normal, const Ogre::Vector3& position);

		void redefineThirdPlane(const Ogre::Vector3& normal, const Ogre::Vector3& position);

		void _debugShowResultPlane(unsigned char planeId);

		void setState(eGizmoState gizmoState);

		eGizmoState getState(void) const;
		
		void changeThickness(Ogre::Real thickness);

		void changeMaterials(const Ogre::String& materialNameX, const Ogre::String& materialNameY, const Ogre::String& materialNameZ, const Ogre::String& materialNameHighlight);

		Ogre::SceneNode* getArrowNodeX(void);

		Ogre::SceneNode* getArrowNodeY(void);

		Ogre::SceneNode* getArrowNodeZ(void);
		
		Ogre::SceneNode* getSelectedNode(void);

		Ogre::v1::Entity* getArrowEntityX(void);

		Ogre::v1::Entity* getArrowEntityY(void);

		Ogre::v1::Entity* getArrowEntityZ(void);

		Ogre::v1::Entity* getSphereEntity(void);

		void drawLine(const Ogre::Vector3& startPosition, const Ogre::Vector3& endPosition, Ogre::Real thickness, const Ogre::String& materialName);

		void hideLine(void);

		void drawCircle(const Ogre::Quaternion& orientation, Ogre::Real fromAngle, Ogre::Real toAngle, bool counterClockWise, Ogre::Real thickness, const Ogre::String& materialName);

		void hideCircle(void);

		void setTranslationCaption(const Ogre::String& caption, const Ogre::ColourValue& color);

		void setRotationCaption(const Ogre::String& caption, const Ogre::ColourValue& color);

		void setPosition(const Ogre::Vector3& position);

		Ogre::Vector3 getPosition(void) const;

		void setOrientation(const Ogre::Quaternion& orientation);

		Ogre::Quaternion getOrientation(void) const;

		void translate(const Ogre::Vector3& translateVector);

		void rotate(const Ogre::Quaternion& rotateQuaternion);

		void setConstraintAxis(const Ogre::Vector3& constraintAxis);
	private:
		void setupFlag(void);
		void createLine(void);
		void destroyLine(void);
		void createCircle(void);
		void destroyCircle(void);
	private:
		Ogre::SceneManager* sceneManager;
		Ogre::Camera* camera;
		Ogre::SceneNode* sphereNode;
		Ogre::SceneNode* selectNode;

		Ogre::v1::Entity* arrowEntityX;
		Ogre::v1::Entity* arrowEntityY;
		Ogre::v1::Entity* arrowEntityZ;
		Ogre::v1::Entity* sphereEntity;

		Ogre::Plane firstPlane;
		Ogre::Plane secondPlane;
		Ogre::Plane thirdPlane;

		Ogre::SceneNode* arrowNodeX;
		Ogre::SceneNode* arrowNodeY;
		Ogre::SceneNode* arrowNodeZ;

		Ogre::String materialNameX;
		Ogre::String materialNameY;
		Ogre::String materialNameZ;
		Ogre::String materialNameHighlight;

		unsigned int defaultCategory;

		eGizmoState gizmoState;
		Ogre::Real thickness;
		bool enabled;

		Ogre::ManualObject* translationLine;
		Ogre::SceneNode* translationLineNode;
		Ogre::ManualObject* rotationCircle;
		Ogre::SceneNode* rotationCircleNode;

		ObjectTitle* translationCaption;
		ObjectTitle* rotationCaption;

		Ogre::v1::Entity* firstPlaneEntity;
		Ogre::SceneNode* firstPlaneNode;
		Ogre::v1::Entity* secondPlaneEntity;
		Ogre::SceneNode* secondPlaneNode;
		Ogre::v1::Entity* thirdPlaneEntity;
		Ogre::SceneNode* thirdPlaneNode;
		bool debugPlane;
		Ogre::Vector3 constraintAxis;
	};

}; // namespace end

#endif
