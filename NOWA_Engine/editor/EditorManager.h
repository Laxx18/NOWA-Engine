#ifndef EDITOR_MANAGER_H
#define EDITOR_MANAGER_H

#include "defines.h"
#include "Gizmo.h"
#include "SelectionManager.h"
#include "Picker.h"
#include "ViewportGrid.h"
#include "modules/CommandModule.h"
#include "utilities/Variant.h"
#include <unordered_map>

namespace NOWA
{
	class Ogre::SceneManager;
	class Ogre::Camera;
	class Ogre::Viewport;
	class Ogre::SceneNode;
	class GameObject;
	class GameObjectComponent;
	class PhysicsActiveComponent;
	class TerraComponent;

	/**
	* @class EditorManager
	* @brief		The editor manager is a complex class, which uses a gizmo for object manipulation like translation, rotation, scale. It posses a grid mode, a object stack mode for placement.
	*				It also uses internally the selection manager in order to select game objects
	*/

	class EXPORTED EditorManager
	{
	public:
		enum eManipulationMode
		{
			EDITOR_PLACE_MODE,
			EDITOR_SELECT_MODE,
			EDITOR_TRANSLATE_MODE,
			EDITOR_PICKER_MODE,
			EDITOR_SCALE_MODE,
			EDITOR_ROTATE_MODE1,
			EDITOR_ROTATE_MODE2,
			EDITOR_TERRAIN_MODIFY_MODE,
			EDITOR_TERRAIN_SMOOTH_MODE,
			EDITOR_TERRAIN_PAINT_MODE,
			// EDITOR_PLAY_MODE,
			EDITOR_NO_MODE
		};

		enum ePlaceMode
		{
			EDITOR_PLACE_MODE_NORMAL,
			EDITOR_PLACE_MODE_STACK,
			EDITOR_PLACE_MODE_STACK_ORIENTATED
		};

		enum eTranslateMode
		{
			EDITOR_TRANSLATE_MODE_NORMAL,
			EDITOR_TRANSLATE_MODE_STACK,
			EDITOR_TRANSLATE_MODE_STACK_ORIENTATED
		};


		enum eCameraView
		{
			EDITOR_CAMERA_VIEW_FRONT,
			EDITOR_CAMERA_VIEW_TOP,
			EDITOR_CAMERA_VIEW_BACK,
			EDITOR_CAMERA_VIEW_BOTTOM,
			EDITOR_CAMERA_VIEW_LEFT,
			EDITOR_CAMERA_VIEW_RIGHT
		};

		struct GameObjectData
		{
			GameObjectData()
				: gameObjectId(0),
				oldPosition(Ogre::Vector3::ZERO),
				newPosition(Ogre::Vector3::ZERO),
				oldScale(Ogre::Vector3::UNIT_SCALE),
				newScale(Ogre::Vector3::UNIT_SCALE),
				oldOrientation(Ogre::Quaternion::IDENTITY),
				newOrientation(Ogre::Quaternion::IDENTITY),
				oldAttribute(nullptr),
				newAttribute(nullptr),
				componentIndex(0)
			{

			}

			~GameObjectData()
			{
				
			}

			// Never work with pointer directly, since game object may be deleted and recreated and the pointer will change!
			unsigned long gameObjectId;
			Ogre::Vector3 oldPosition;
			Ogre::Vector3 newPosition;
			Ogre::Vector3 oldScale;
			Ogre::Vector3 newScale;
			Ogre::Quaternion oldOrientation;
			Ogre::Quaternion newOrientation;
			Variant* oldAttribute;
			Variant* newAttribute;
			unsigned int componentIndex;
		};
		
		EditorManager();
		~EditorManager();

		/**
		* @brief		Initializes the gizmo manager
		* @param[in]	sceneManager			The ogre scene manager to use
		* @param[in]	camera					The camera to render
		* @param[in]	mouseButtonId			Which mouse button should handle the selection
		* @param[in]	categories				The categories of game objects that are selectable
		* @param[in]	selectionObserver		Sets the selection observer to react at the moment when a game object has been selected.
		*										A newly created selection observer heap pointer must be passed for the ISpawnObserver. It will be deleted internally,
		*										if the object is not required anymore.
		* @param[in]	thickness				The thickness of the arrows
		* @param[in]	materialNameX			The material name for x arrow
		* @param[in]	materialNameY			The material name for y arrow
		* @param[in]	materialNameZ			The material name for z arrow
		* @param[in]	materialNameHighlight	The material name for highlight
		* @param[in]	materialName			The material name to use for the rectangle selection. Default is "Select" which comes from the NOWA folder.
		* @Note			When the gizmo manager is used, 4 categories (GizmoX, GizmoY, GizmoZ, GizmoSphere), will be occupied in the GameObjectController. So overall, there are 4 categories less available.
		*/
		void init(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, Ogre::String& categories, OIS::MouseButtonID mouseButtonId,
			SelectionManager::ISelectionObserver* selectionObserver, Ogre::Real thickness = 0.6f,
			const Ogre::String& materialNameX = "BaseRedLine", const Ogre::String& materialNameY = "BaseGreenLine", const Ogre::String& materialNameZ = "BaseBlueLine",
			const Ogre::String& materialNameHighlight = "BaseYellowLine", const Ogre::String& materialNameSelection = "Select");
		
		bool handleKeyPress(const OIS::KeyEvent& keyEventRef);

		void handleKeyRelease(const OIS::KeyEvent& keyEventRef);

		void handleMouseMove(const OIS::MouseEvent& evt);
		
		void handleMousePress(const OIS::MouseEvent& evt, OIS::MouseButtonID id);

		void handleMouseRelease(const OIS::MouseEvent& evt, OIS::MouseButtonID id);

		void selectGizmo(const OIS::MouseEvent& evt, const Ogre::Ray& hitRay);

		void manipulateObjects(const Ogre::Ray& hitRay);

		void movePlaceNode(const Ogre::Ray& hitRay);

		void rotatePlaceNode(void);

		void moveObjects(Ogre::Vector3& offset);

		void scaleObjects(Ogre::Vector3& offset);

		void setConstraintAxis(const Ogre::Vector3& constraintAxis);

		void attachMeshToPlaceNode(const Ogre::String& meshName, GameObject::eType type, Ogre::v1::Mesh* mesh = nullptr);

		void attachOtherResourceToPlaceNode(GameObject::eType type);

		void attachGroupToPlaceNode(const std::vector<unsigned long>& gameObjectIds);

		Ogre::Vector3 calculateUnitScaleFactor(const Ogre::Vector3& translateVector);

		void rotateObjects(void);

		void update(Ogre::Real dt);

		Ogre::Vector3 calculateCenter(void);

		void setGizmoToGameObjectsCenter(void);

		Ogre::SceneManager* getSceneManager(void) const;
		
		Gizmo* getGizmo(void);

		SelectionManager* getSelectionManager(void) const;

		void setManipulationMode(eManipulationMode manipulationMode);

		eManipulationMode getManipulationMode(void) const;

		void setPlaceMode(ePlaceMode placeMode);

		ePlaceMode getPlaceMode(void) const;

		void setTranslateMode(eTranslateMode translateMode);

		eTranslateMode getTranslateMode(void) const;

		void setCameraView(eCameraView cameraView);

		eCameraView getCameraView(void) const;

		void setGridStep(Ogre::Real gridStep);

		Ogre::Real getGridStep(void) const;

		void setViewportGridEnabled(bool enable);

		bool getViewportGridEnabled(void) const;

		/*
		 * @brief Starts the simulation and creates a snapshot of all game objects for undo and redo
		 */
		void startSimulation(void);

		void stopSimulation(bool withUndo = true);

		void snapshotSelectedGameObjects(void);

		void snapshotOldGameObjectAttribute(std::vector<GameObject*> gameObjects, const Ogre::String& attributeName);

		void snapshotOldGameObjectComponentAttribute(std::vector<GameObject*> gameObjects, std::vector<GameObjectComponent*> gameObjectComponts, const Ogre::String& attributeName);

		void snapshotNewGameObjectAttribute(Variant* newAttribute);

		void snapshotNewGameObjectComponentAttribute(Variant* newAttribute);

		void snapshotCameraTransform(void);

		void snapshotCameraPosition(void);

		void snapshotCameraOrientation(void);

		void snapshotTerraHeightMap(const std::vector<Ogre::uint16>& oldHeightData, const std::vector<Ogre::uint16>& newHeightData, TerraComponent* terraComponent);

		void snapshotTerraBlendMap(const std::vector<Ogre::uint8>& oldBlendWeightData, const std::vector<Ogre::uint8>& newBlendWeightData, TerraComponent* terraComponent);

		void deleteGameObjects(const std::vector<unsigned long> gameObjectIds);

		void addComponent(const std::vector<unsigned long> gameObjectIds, const Ogre::String& componentClassName);

		void deleteComponent(const std::vector<unsigned long> gameObjectIds, unsigned int index);

		void cloneGameObjects(const std::vector<unsigned long> gameObjectIds);

		void undo(void);

		void redo(void);

		bool canUndo(void);

		bool canRedo(void);

		void cameraUndo(void);

		void cameraRedo(void);

		bool canCameraUndo(void);

		bool canCameraRedo(void);

		void setUseUndoRedoForPicker(bool useUndoRedoForPicker);

		bool getUseUndoRedoForPicker(void) const;

		/**
		* @brief		Adapts the category (query mask) at runtime, to change which game object types can be selected.
		* @param[in]	categories	The categories to use
		*/
		void filterCategories(Ogre::String& categories);

		void focusCameraGameObject(GameObject* gameObject);

		/**
		 * @brief		Gets the current pick force
		 * @return		pickForce The pick force to get
		 */
		Ogre::Real getPickForce(void) const;

		/**
		* @brief		Optimizes the scene, for better performance.
		* @note			It will go through all game objects that are having a component, that indicates, that this game object will not be moved.
		*				It then sets the the behavior to "static" for graphics optimization.
		*				Attention: After that, if this game object is static and the designer decides to move it, weird behavior will occur!
		* @param[in]	optimize	If set to true the scene will be optimized. Else all "static" game object will be set to "dynamic" again!
		*/
		void optimizeScene(bool optimize);

		void activateTestSelectedGameObjects(bool bActivate);
	private:
		bool getRayHitPoint(const Ogre::Ray& hitRay);
		bool getRayStartPoint(const Ogre::Ray& hitRay);
		Ogre::Vector3 getHitPointOnFloor(const Ogre::Ray& hitRay);
		std::vector<unsigned long> getSelectedGameObjectIds(void);
		std::vector<unsigned long> getAllGameObjectIds(void);
		void deactivatePlaceMode(void);
		void applyPlaceMovableTransform(void);
		void applyGroupTransform(void);
		void destroyTempPlaceMovableObjectNode(void);
		std::tuple<bool, Ogre::Real, Ogre::Vector3> getTranslateYData(GameObject* gameObject);
		void applySelectedObjectsStartOrientation(void);
	private:
		Ogre::SceneManager* sceneManager;
		Ogre::Camera* camera;
		Gizmo* gizmo;
		ViewportGrid* viewportGrid;
		SelectionManager* selectionManager;
		Ogre::SceneNode* placeNode;
		Ogre::SceneNode* tempPlaceMovableNode;
		Ogre::MovableObject* tempPlaceMovableObject;

		Ogre::RaySceneQuery* gizmoQuery;
		Ogre::RaySceneQuery* placeObjectQuery;
		Ogre::RaySceneQuery* toBePlacedObjectQuery;

		Ogre::Ray mouseHitRay;
		
		CommandModule sceneManipulationCommandModule;
		CommandModule cameraCommandModule;

		std::vector<GameObjectData> oldGameObjectDataList;
		GameObjectData oldGameObjectDataAttribute;

		bool isGizmoMoving;
		bool mouseIdPressed;
		eManipulationMode manipulationMode;
		ePlaceMode placeMode;
		eTranslateMode translateMode;
		eCameraView cameraView;
		OIS::MouseButtonID mouseButtonId;
		Ogre::Vector3 oldHitPoint;
		Ogre::Vector3 hitPoint;
		Ogre::Real gridStep;
		Ogre::Plane resultPlane;
		Ogre::Vector3 translateStartPoint;
		Ogre::Quaternion rotateStartOrientation;
		Ogre::Quaternion rotateStartOrientationGizmoX;
		Ogre::Quaternion rotateStartOrientationGizmoY;
		Ogre::Quaternion rotateStartOrientationGizmoZ;
		Ogre::Quaternion oldGizmoOrientation;
		Ogre::Vector3 oldGizmoPosition;
		Ogre::Vector3 startHitPoint;
		Ogre::Real absoluteAngle;
		Ogre::Real stepAngleDelta;
		OIS::KeyCode currentKey;
		bool useUndoRedoForPicker;
		KinematicPicker* movePicker;
		Picker* movePicker2;
		Ogre::Real timeSinceLastUpdate;
		GameObject::eType currentPlaceType;
		std::vector<std::tuple<unsigned long, Ogre::Vector3, Ogre::Quaternion>> groupGameObjectIds;
		std::vector<Ogre::Quaternion> selectedObjectsStartOrientations;
		Ogre::Real pickForce;
		Ogre::Vector3 constraintAxis;
		bool isInSimulation;
		Ogre::Real rotateFactor;

		TerraComponent* terraComponent;

		std::unordered_map<unsigned long, std::vector<bool>> oldActivatedMap;
	};

}; // namespace end

#endif
