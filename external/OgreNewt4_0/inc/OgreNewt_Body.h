/*
OgreNewt Library 4.0

Ogre implementation of Newton Game Dynamics SDK 4.0
*/

#ifndef _INCLUDE_OGRENEWT_BODY
#define _INCLUDE_OGRENEWT_BODY

#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_MaterialID.h"
#include "OgreNewt_Collision.h"
#include "OgreMesh2.h"
#include "OgreItem.h"
#include "ndBezierSpline.h"

namespace OgreNewt
{
	class ContactJoint;
	class Contact;
	class BodyNotify; // Forward declaration

	//! main class for all Rigid Bodies in the system (Newton 4.0)
	class _OgreNewtExport Body : public _DestructorCallback<Body>
	{
	public:
		typedef OgreNewt::function<void(OgreNewt::Body*, ndFloat32 timeStep, int threadIndex)> ForceCallback;
		typedef OgreNewt::function<void(OgreNewt::Body*)> NodeUpdateNotifyCallback;
		typedef OgreNewt::function<void(OgreNewt::Body*, OgreNewt::Contact*)> ContactCallback;
		typedef OgreNewt::function<void(Ogre::SceneNode*, const Ogre::Vector3&, const Ogre::Quaternion&, bool updateRot, bool updateStatic)> RenderUpdateCallback;

		Body(World* world, Ogre::SceneManager* sceneManager, const OgreNewt::CollisionPtr& col, Ogre::SceneMemoryMgrTypes memoryType = Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC);
		Body(World* world, Ogre::SceneManager* sceneManager, ndBodyKinematic* body, Ogre::SceneMemoryMgrTypes memoryType = Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC);
		Body(World* world, Ogre::SceneManager* sceneManager, Ogre::SceneMemoryMgrTypes memoryType = Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC);

		virtual ~Body();

#ifndef OGRENEWT_NO_OGRE_ANY
		void setUserData(const OgreNewt::Any& data) { m_userdata = data; }
		const OgreNewt::Any& getUserData() const { return m_userdata; }
#else
		void setUserData(void* data) { m_userdata = data; }
		void* getUserData() const { return m_userdata; }
#endif

		ndBodyKinematic* getNewtonBody() const { return m_body; }

		Ogre::SceneNode* getOgreNode() const { return m_node; }
		Ogre::ManualObject* getDebugCollisionLines() { return m_debugCollisionLines; }
		OgreNewt::World* getWorld() const { return m_world; }

		void setType(unsigned int categoryType);
		unsigned int getType() const { return m_categoryType; }

		void attachNode(Ogre::SceneNode* node, bool updateRotation = true);
		void detachNode(void);

		void setStandardForceCallback();
		void setCustomForceAndTorqueCallback(ForceCallback callback);

		void removeForceAndTorqueCallback();

		template<class c> void setCustomForceAndTorqueCallback(OgreNewt::function<void(c*, Body*, float, int)> callback, c* instancedClassPointer)
		{
			setCustomForceAndTorqueCallback(OgreNewt::bind(callback, instancedClassPointer, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		}

		ForceCallback getForceTorqueCallback() { return m_forcecallback; }

		void setNodeUpdateNotify(NodeUpdateNotifyCallback callback);

		template<class c>
		void setNodeUpdateNotify(OgreNewt::function<void(c*, OgreNewt::Body*)> callback, c* instancedClassPointer)
		{
			setNodeUpdateNotify(OgreNewt::bind(callback, instancedClassPointer, std::placeholders::_1));
		}

		void removeNodeUpdateNotify() { m_nodeupdatenotifycallback = nullptr; }

		void setContactCallback(ContactCallback callback);

		template<class c>
		void setContactCallback(OgreNewt::function<void(c*, OgreNewt::Body*, OgreNewt::Contact*)> callback, c* instancedClassPointer)
		{
			setContactCallback(OgreNewt::bind(callback, instancedClassPointer, std::placeholders::_1, std::placeholders::_2));
		}

		void removeContactCallback() { m_contactCallback = nullptr; }

		void setGravity(const Ogre::Vector3& gravity);
		Ogre::Vector3 getGravity(void) const;

		void setPositionOrientation(const Ogre::Vector3& pos, const Ogre::Quaternion& orient, int threadIndex = -1);
		void setMassMatrix(Ogre::Real mass, const Ogre::Vector3& inertia);
		void setCenterOfMass(const Ogre::Vector3& centerOfMass);
		Ogre::Vector3 getCenterOfMass() const;

		void freeze();
		void unFreeze();
		bool isFreezed();

		CollisionPrimitiveType getCollisionPrimitiveType() const;

		void enableGyroscopicTorque(bool enable);

		void setMaterialGroupID(const MaterialID* materialId);
		void setContinuousCollisionMode(unsigned state);
		void setJointRecursiveCollision(unsigned state);
		void setCollidable(bool collidable);
		bool getCollidable(void) const;

		void setOmega(const Ogre::Vector3& omega);
		void setVelocity(const Ogre::Vector3& vel);
		void setLinearDamping(Ogre::Real damp);
		void setAngularDamping(const Ogre::Vector3& damp);

		void setCollision(const OgreNewt::CollisionPtr& col);
		void setAutoSleep(int flag);
		int getAutoSleep();

		void scaleCollision(const Ogre::Vector3& scale);

		const OgreNewt::MaterialID* getMaterialGroupID() const;
		int getContinuousCollisionMode() const;
		int getJointRecursiveCollision() const;

		void getPositionOrientation(Ogre::Vector3& pos, Ogre::Quaternion& orient) const;
		Ogre::Vector3 getPosition() const;
		Ogre::Quaternion getOrientation() const;

		void getVisualPositionOrientation(Ogre::Vector3& pos, Ogre::Quaternion& orient) const;
		Ogre::Vector3 getVisualPosition() const;
		Ogre::Quaternion getVisualOrientation() const;

		Ogre::AxisAlignedBox getAABB() const;

		void getMassMatrix(Ogre::Real& mass, Ogre::Vector3& inertia) const;
		Ogre::Real getMass() const;
		Ogre::Vector3 getInertia() const;
		void getInvMass(Ogre::Real& mass, Ogre::Vector3& inertia) const;

		Ogre::Vector3 getOmega() const;
		Ogre::Vector3 getVelocity() const;
		Ogre::Vector3 getVelocityAtPoint(const Ogre::Vector3& point) const;
		Ogre::Vector3 getForce() const;
		Ogre::Vector3 getTorque() const;

		Ogre::Real getLinearDamping() const;
		Ogre::Vector3 getAngularDamping() const;

		Ogre::Vector3 calculateInverseDynamicsForce(Ogre::Real timestep, Ogre::Vector3 desiredVelocity);

		void addImpulse(const Ogre::Vector3& deltav, const Ogre::Vector3& posit, Ogre::Real timeStep);
		void addForce(const Ogre::Vector3& force);
		void addTorque(const Ogre::Vector3& torque);
		void setForce(const Ogre::Vector3& force);
		void setTorque(const Ogre::Vector3& torque);
		void setBodyAngularVelocity(const Ogre::Vector3& desiredOmega, Ogre::Real timestep);

		Body* getNext() const;

		void addGlobalForce(const Ogre::Vector3& force, const Ogre::Vector3& pos);
		void addGlobalForceAccumulate(const Ogre::Vector3& force, const Ogre::Vector3& pos);
		void addLocalForce(const Ogre::Vector3& force, const Ogre::Vector3& pos);

		ndShapeInstance* getNewtonCollision() const;

		void showDebugCollision(bool isStatic, bool show, const Ogre::ColourValue& color = Ogre::ColourValue::Green);

		void updateNode(Ogre::Real interpolatParam);

		Ogre::SceneMemoryMgrTypes getSceneMemoryType(void) const;

		void clampAngularVelocity(Ogre::Real clampValue);

		Ogre::Vector3 getLastPosition(void) const;
		Ogre::Quaternion getLastOrientation(void) const;

		void setRenderUpdateCallback(RenderUpdateCallback renderUpdateCallback);
		bool hasRenderUpdateCallback(void) const;

		void onTransformCallback(const ndMatrix& matrix);
		void onForceAndTorqueCallback(ndFloat32 timestep, int threadIndex);

		void setConvexIntertialMatrix(const Ogre::Vector3& inertia, const Ogre::Vector3& massOrigin);

	protected:
		ndBodyKinematic* m_body;
		BodyNotify* m_bodyNotify; // Store the notification object
		const MaterialID* m_matid;
		World* m_world;
		Ogre::Vector3 m_gravity;

		std::vector<std::pair<Ogre::Vector3, Ogre::Vector3> > m_accumulatedGlobalForces;

		Ogre::Vector3 m_nodePosit;
		Ogre::Vector3 m_curPosit;
		Ogre::Vector3 m_prevPosit;
		Ogre::Vector3 m_lastPosit;
		Ogre::Quaternion m_nodeRotation;
		Ogre::Quaternion m_curRotation;
		Ogre::Quaternion m_prevRotation;
		Ogre::Quaternion m_lastOrientation;

#ifndef OGRENEWT_NO_OGRE_ANY
		OgreNewt::Any m_userdata;
		OgreNewt::Any m_userdata2;
#else
		void* m_userdata;
#endif

		unsigned int m_categoryType;
		Ogre::SceneNode* m_node;

		ForceCallback m_forcecallback;
		NodeUpdateNotifyCallback m_nodeupdatenotifycallback;
		ContactCallback m_contactCallback;

		bool m_updateRotation;
		bool m_validToUpdateStatic;
		Ogre::SceneMemoryMgrTypes m_memoryType;

		Ogre::SceneManager* m_sceneManager;
		Ogre::ManualObject* m_debugCollisionLines;
		Ogre::MeshPtr m_debugCollisionMesh;
		Ogre::Item* m_debugCollisionItem;
		Ogre::SceneMemoryMgrTypes m_sceneMemoryType;
		bool m_isOwner;
		RenderUpdateCallback m_renderUpdateCallback;

	protected:
		static void newtonDestructor(ndBodyKinematic* body);
		static void standardForceCallback(Body* me, float timeStep, int threadIndex);
	};
}

#endif
