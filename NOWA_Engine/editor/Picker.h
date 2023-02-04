#ifndef PICKER_H
#define PICKER_H

#include "defines.h"
#include "gameobject/PhysicsActiveComponent.h"

namespace NOWA
{
	class Ogre::SceneManager;
	class Ogre::Camera;
	class Ogre::Viewport;
	class Ogre::SceneNode;
	class Ogre::v1::ManualObject;
	class OgreNewt::World;
	class GameObject;
	class OgreNewt::KinematicController;

	class EXPORTED IPicker
	{
	public:
		virtual Ogre::Real getPickForce(void) const = 0;

		virtual Ogre::Ray getRayFromMouse(void) const = 0;

		virtual Ogre::Real getDragDistance(void) const = 0;

		virtual Ogre::Vector3 getDragPoint(void) const = 0;

		virtual bool getDrawLine(void) const = 0;

		virtual void createLine(void) = 0;

		virtual void drawLine(const Ogre::Vector3& startPosition, const Ogre::Vector3& endPosition) = 0;

		virtual void destroyLine() = 0;
	};

	/**
	* @class Picker
	* @brief This class can be used to drag game objects physically via an input device
	*/
	class EXPORTED Picker : public IPicker
	{
	public:
		class PickForceObserver : public PhysicsActiveComponent::IForceObserver
		{
		public:
			PickForceObserver(Picker* picker)
				: PhysicsActiveComponent::IForceObserver(picker)
			{

			}

			virtual void onForceAdd(OgreNewt::Body* body, Ogre::Real timeStep, int threadIndex) override
			{
				Ogre::Ray mouseRay = this->picker->getRayFromMouse();
				// Get the global position our cursor is at
				Ogre::Vector3 cursorPos = mouseRay.getPoint(this->picker->getDragDistance());

				Ogre::Quaternion bodyOrientation;
				Ogre::Vector3 bodyPos;

				// Now find the global point on the body
				body->getPositionOrientation(bodyPos, bodyOrientation);

				// Find the handle position we are holding the body from
				Ogre::Vector3 dragPos = (bodyOrientation * this->picker->getDragPoint()) + bodyPos;

				Ogre::Vector3 inertia;
				Ogre::Real mass;

				body->getMassMatrix(mass, inertia);

				Ogre::Vector3 dragForce = Ogre::Vector3::ZERO;

				if (Ogre::Math::RealEqual(this->picker->getPickForce(), 50.0f))
				{
					body->setPositionOrientation(cursorPos, Ogre::Quaternion::IDENTITY);
					// Annulate gravity
					body->addForce(body->getGravity() * mass);
					this->picker->destroyLine();
				}
				else
				{
					// Calculate picking spring force
					Ogre::Vector3 dragForce = ((cursorPos - dragPos) * mass * this->picker->getPickForce()) - body->getVelocity();
					if (this->picker->getDrawLine())
					{
						this->picker->drawLine(cursorPos, dragPos);
					}

					// body->addForce(body->getGravity() * mass);

					// Add the picking spring force at the handle
					body->addGlobalForce(dragForce, dragPos);
				}
			}
		};

		/**
		* @class IPickObserver
		* @brief This interface can be implemented to react when a game object has been picked or released
		*/
		class EXPORTED IPickObserver
		{
		public:
			virtual ~IPickObserver()
			{
			}

			/**
			* @brief		Called when a game object has been picked
			* @param[in]	pickedGameObject	The game object to react on
			*/
			virtual void onPick(NOWA::GameObject* pickedGameObject) = 0;

			/**
			* @brief		Called when the picked game object has been released
			*/
			virtual void onRelease(void) = 0;
		};
	public:
		Picker();
		~Picker();

		/**
		* @brief		Initializes the picker
		* @param[in]	sceneManager	The ogre scene manager to use
		* @param[in]	camera			The ogre camera to use
		* @param[in]	maxDistance		The the maximum drag distance in meters
		* @param[in]	queryMask		The query mask to filter game objects that should be dragable. The query mask can be adapted via the GameObjectController.
		* @param[in]	drawLines		If set to true, the drag line will be visible.
		*/
		void init(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, Ogre::Real maxDistance = 100.0f, unsigned int queryMask = 0xFFFFFFFF, bool drawLines = true);

		
		/**
		 * @brief		Attaches a pick observer to react at the moment when a game object has been picked or released.
		 * @param[in]	pickObserver	The pick observer to attach
		 * @note		The pick observer must be created on heap, but will be destroy either by calling @detachAndDestroyPickObserver or @detachAndDestroyAllPickObserver or will be destroyed
		 *				at last automatically.
		 */
		void attachPickObserver(IPickObserver* pickObserver);

		/**
		 * @brief		Detaches and destroys a pick observer
		 * @param[in]	pickObserver	The pick observer to detach and destroy
		 */
		void detachAndDestroyPickObserver(IPickObserver* pickObserver);

		/**
		 * @brief		Detaches and destroys all pick observer
		 */
		void detachAndDestroyAllPickObserver(void);

		// x, y absolute mouse cooridnates ms.X.abs for example, viewport for area size, since viewport can be resized at runtime

		/**
		* @brief		Executes the grab process. This function can be used e.g. when holding a mouse button.
		* @param[in]	ogreNewt				The ogre newt physics to grab game objects physically
		* @param[in]	position				The position of the input device, e.g. x, y of the mouse.
		* @note									The x, y coordinates are absolute cooridnates, ms.X.abs for example
		* @param[in]	renderWindow			The render window, which is used to get the area size and correct mapped x, y coordinates. Since these coordinates are depending on the render window size.
		* @param[in]	pickForce				The absolute pick force in newton meters. This values determines the force at which a gameobject will be dragged. The pick force is always added to the
		*										already existing mass of the dragged game object. That means, each active game object, no matter what mass it has will be dragged.
		*										Increasing the pick force dragges the game object with more violence.
		* @return		physicsActiveGameObject The resulting game object that is being dragged.
		*/

		PhysicsComponent* grab(OgreNewt::World* ogreNewt, const Ogre::Vector2 position, Ogre::Window* renderWindow, Ogre::Real pickForce = 20.0f);

		/**
		* @brief	Releases the dragged game object
		*/
		void release(void);

		/**
		* @brief		Adapts the query mask at runtime, to change which game object types can be dragged.
		* @param[in]	newQueryMask	The new query mask to use
		*/
		void updateQueryMask(unsigned int newQueryMask);

		/**
		 * @brief		Gets the current pick force
		 * @return		pickForce The pick force to get
		 */
		virtual Ogre::Real getPickForce(void) const override;
	protected:
		Picker(const Picker&);
		Picker& operator = (const Picker&);

		virtual Ogre::Ray getRayFromMouse(void) const override;

		virtual Ogre::Real getDragDistance(void) const override;

		virtual Ogre::Vector3 getDragPoint(void) const override;

		virtual bool getDrawLine(void) const override;

		virtual void createLine(void) override;

		virtual void drawLine(const Ogre::Vector3& startPosition, const Ogre::Vector3& endPosition) override;

		virtual void destroyLine() override;

		void dragCallback(OgreNewt::Body* body, Ogre::Real timeStep, int threadIndex);
	private:
		bool active;
		bool dragging;
		Ogre::Real maxDistance;
		Ogre::Real pickForce;
		bool drawLines;
		Ogre::Vector2 mousePosition;

		Ogre::SceneManager* sceneManager;
		Ogre::Camera* camera;
		Ogre::Window* renderWindow;
		Ogre::Vector3 dragPoint;
		Ogre::Real dragDistance;
		PhysicsComponent* dragComponent;
		OgreNewt::Body* hitBody;
		unsigned int queryMask;
		unsigned long gameObjectId;
		Ogre::SceneNode* dragLineNode;
		Ogre::ManualObject* dragLineObject;
		std::vector<IPickObserver*> pickObservers;
		bool hasMoveCallback;
		OgreNewt::Body::ForceCallback oldForceTorqueCallback;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/**
	* @class GameObjectPicker
	* @brief This class can be used to drag a game object physically via an input device
	*/
	class EXPORTED GameObjectPicker : public IPicker
	{
	public:
		class PickForceObserver : public PhysicsActiveComponent::IForceObserver
		{
		public:
			PickForceObserver(IPicker* picker)
				: PhysicsActiveComponent::IForceObserver(picker)
			{

			}

			virtual void onForceAdd(OgreNewt::Body* body, Ogre::Real timeStep, int threadIndex) override
			{
				Ogre::Ray mouseRay = this->picker->getRayFromMouse();
				// Get the global position our cursor is at
				Ogre::Vector3 cursorPos = mouseRay.getPoint(this->picker->getDragDistance());

				Ogre::Quaternion bodyOrientation;
				Ogre::Vector3 bodyPos;

				// Now find the global point on the body
				body->getPositionOrientation(bodyPos, bodyOrientation);

				// Find the handle position we are holding the body from
				Ogre::Vector3 dragPos = (bodyOrientation * this->picker->getDragPoint()) + (bodyPos + static_cast<GameObjectPicker*>(this->picker)->offsetPosition);

				Ogre::Vector3 inertia;
				Ogre::Real mass;

				body->getMassMatrix(mass, inertia);

				// Calculate picking spring force
				Ogre::Vector3 dragForce = Ogre::Vector3::ZERO;

				if (Ogre::Math::RealEqual(this->picker->getPickForce(), 50.0f))
				{
					body->setPositionOrientation(cursorPos, Ogre::Quaternion::IDENTITY);
					// Annulate gravity
					body->addForce(body->getGravity() * mass);
					this->picker->destroyLine();
				}
				else
				{
					Ogre::Real length = (cursorPos - dragPos).length();
					// Ogre::LogManager::getSingletonPtr()->logMessage("length: " + Ogre::StringConverter::toString(this->dragDistance));
					if (length < static_cast<GameObjectPicker*>(this->picker)->dragAffectDistance)
					{
						Ogre::Vector3 dragForce = ((cursorPos - dragPos) * mass * (static_cast<Ogre::Real>(this->picker->getPickForce())))/* - body->getVelocity()*/;
						if (this->picker->getDrawLine())
						{
							this->picker->drawLine(cursorPos, dragPos);
						}
						// Add the picking spring force at the handle
						body->addGlobalForce(dragForce, dragPos);
					}
					else
					{
						if (this->picker->getDrawLine())
						{
							this->picker->destroyLine();
						}
					}
				}
			}
		};
	public:
		GameObjectPicker();
		~GameObjectPicker();

		/**
		* @brief		Initializes the picker.
		* @param[in]	maxDistance		The the maximum drag distance in meters
		* @param[in]	gameObjectId	The game object id, which shall be dragged.
		*/
		void init(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, Ogre::Real maxDistance, unsigned long gameObjectId);

		/**
		* @brief		Actualizes the picker.
		* @param[in]	gameObjectId	The game object id, which shall be dragged.
		* @param[in]	drawLines		Whether to draw a line
		*/
		void actualizeData(Ogre::Camera* camera, unsigned long gameObjectId, bool drawLines);

		/**
		* @brief		Actualizes the picker.
		* @param[in]	jointId			The joint id (to get the physics body from) to drag. Useful if ragdoll bone is involved.
		* @param[in]	drawLines		Whether to draw a line
		*/
		void actualizeData2(Ogre::Camera* camera, unsigned long jointId, bool drawLines);

		/**
		* @brief		Actualizes the picker.
		* @param[in]	body			The direct body pointer to drag.
		* @param[in]	drawLines		Whether to draw a line
		*/
		void actualizeData3(Ogre::Camera* camera, OgreNewt::Body* body, bool drawLines);

		/**
		* @brief		Executes the grab process. This function can be used e.g. when holding a mouse button.
		* @param[in]	ogreNewt				The ogre newt physics to grab game objects physically
		* @param[in]	position				The position of the input device, e.g. x, y of the mouse.
		* @note									The x, y coordinates are absolute cooridnates, ms.X.abs for example
		* @param[in]	renderWindow			The render window, which is used to get the area size and correct mapped x, y coordinates. Since these coordinates are depending on the render window size.
		* @param[in]	pickForce				The absolute pick force in newton meters. This values determines the force at which a gameobject will be dragged. The pick force is always added to the
		*										already existing mass of the dragged game object. That means, each active game object, no matter what mass it has will be dragged.
		*										Increasing the pick force dragges the game object with more violence.
		* @param[in]	dragAffectDistance		The drag affect distance in meters at which the target game object is affected by the picker.
		* @param[in]	offsetPositoin			The offset position at which the target game object shall be picked.
		* @return		physicsActiveGameObject The resulting game object that is being dragged.
		*/

		void grabGameObject(OgreNewt::World* ogreNewt, const Ogre::Vector2 position, Ogre::Window* renderWindow, Ogre::Real pickForce = 20.0f, Ogre::Real dragAffectDistance = 5.0f, const Ogre::Vector3& offsetPosition = Ogre::Vector3::ZERO);

		/**
		* @brief	Releases the dragged game object
		*/
		void release(bool resetBody = false);

		/**
		* @brief		Adapts the query mask at runtime, to change which game object types can be dragged.
		* @param[in]	newQueryMask	The new query mask to use
		*/
		void updateQueryMask(unsigned int newQueryMask);

		/**
		 * @brief		Gets the current pick force
		 * @return		pickForce The pick force to get
		 */
		virtual Ogre::Real getPickForce(void) const override;
	protected:
		GameObjectPicker(const GameObjectPicker&);
		GameObjectPicker& operator = (const GameObjectPicker&);

		virtual Ogre::Ray getRayFromMouse(void) const override;

		virtual Ogre::Real getDragDistance(void) const override;

		virtual Ogre::Vector3 getDragPoint(void) const override;

		virtual bool getDrawLine(void) const override;

		virtual void createLine(void) override;

		virtual void drawLine(const Ogre::Vector3& startPosition, const Ogre::Vector3& endPosition) override;

		virtual void destroyLine() override;

		void dragCallbackGameObject(OgreNewt::Body* body, Ogre::Real timeStep, int threadIndex);
	private:
		void deleteJointDelegate(EventDataPtr eventData);

		void deleteBodyDelegate(EventDataPtr eventData);
	private:
		bool active;
		bool dragging;
		Ogre::Real maxDistance;
		Ogre::Real pickForce;
		bool drawLines;
		Ogre::Vector2 mousePosition;

		Ogre::SceneManager* sceneManager;
		Ogre::Camera* camera;
		Ogre::Window* renderWindow;
		Ogre::Vector3 dragPoint;
		Ogre::Real dragDistance;
		PhysicsComponent* dragComponent;
		OgreNewt::Body* hitBody;
		unsigned long jointId;
		unsigned int queryMask;
		unsigned long gameObjectId;
		Ogre::SceneNode* dragLineNode;
		Ogre::ManualObject* dragLineObject;
		bool hasMoveCallback;
		OgreNewt::Body::ForceCallback oldForceTorqueCallback;
		Ogre::Real dragAffectDistance;
		Ogre::Vector3 offsetPosition;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/**
	* @class KinematicPicker
	* @brief This class can be used to drag game objects physically via an input device (without spring).
	*/
	class EXPORTED KinematicPicker
	{
	public:
		KinematicPicker();
		~KinematicPicker();

		/**
		* @brief		Initializes the picker
		* @param[in]	sceneManager	The ogre scene manager to use
		* @param[in]	camera			The ogre camera to use
		* @param[in]	maxDistance		The the maximum drag distance in meters
		* @param[in]	queryMask		The query mask to filter game objects that should be dragable. The query mask can be adapted via the GameObjectController.
		*/
		void init(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, Ogre::Real maxDistance = 100.0f, unsigned int queryMask = 0xFFFFFFFF);

		/**
		* @brief		Executes the grab process. This function can be used e.g. when holding a mouse button.
		* @param[in]	ogreNewt				The ogre newt physics to grab game objects physically
		* @param[in]	position				The position of the input device, e.g. x, y of the mouse.
		* @note									The x, y coordinates are absolute cooridnates, ms.X.abs for example
		* @param[in]	renderWindow			The render window, which is used to get the area size and correct mapped x, y coordinates. Since these coordinates are depending on the render window size.
		*										Increasing the pick force dragges the game object with more violence.
		* @return		physicsActiveGameObject The resulting game object that is being dragged.
		*/

		PhysicsComponent* grab(OgreNewt::World* ogreNewt, const Ogre::Vector2 position, Ogre::Window* renderWindow);

		/**
		* @brief	Releases the dragged game object
		*/
		void release(void);

		/**
		* @brief		Adapts the query mask at runtime, to change which game object types can be dragged.
		* @param[in]	newQueryMask	The new query mask to use
		*/
		void updateQueryMask(unsigned int newQueryMask);

		Ogre::Real getDragDistance(void) const;

		Ogre::Vector3 getDragNormal(void) const;

		Ogre::Vector3 getDragPoint(void) const;
	private:
		KinematicPicker(const KinematicPicker&);
		KinematicPicker& operator = (const KinematicPicker&);

		Ogre::Ray getRayFromMouse(void) const;

	private:
		Ogre::SceneManager* sceneManager;
		Ogre::Camera* camera;
		Ogre::Window* renderWindow;
		Ogre::Real maxDistance;
		Ogre::Vector3 dragNormal;
		Ogre::Vector3 dragPoint;
		Ogre::Real dragDistance;
		unsigned int queryMask;
		OgreNewt::Body* hitBody;
		Ogre::Vector2 mousePosition;
		PhysicsComponent* dragComponent;
		OgreNewt::KinematicController* kinematicController;
		bool active;
		bool dragging;
	};

}; //namespace end

#endif