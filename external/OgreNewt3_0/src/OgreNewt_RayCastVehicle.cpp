#include "OgreNewt_Stdafx.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_World.h"
#include "OgreNewt_Tools.h"
#include "OgreNewt_CollisionPrimitives.h"
#include "OgreNewt_RayCastVehicle.h"

#include <CustomDGRayCastCar.h>

namespace OgreNewt
{
	////////////////////////////////////////////////////////
	// TIRE FUNCTIONS

		// constructor
	//    Vehicle::Tire::Tire( OgreNewt::Vehicle* vehicle, Ogre::Quaternion localorient, Ogre::Vector3 localpos, Ogre::Vector3 pin,
	//              Ogre::Real mass, Ogre::Real width, Ogre::Real radius, Ogre::Real susShock, Ogre::Real susSpring, Ogre::Real susLength, int colID)

	RayCastVehicle::RayCastVehicle(OgreNewt::Body* carBody, int maxTiresCount)
		:Joint()
	{
		CustomDGRayCastCar* controller;

		// make a matrix to indicate the cal local coordinate system
		dMatrix carCordenatSytem;
		carCordenatSytem.m_front = dVector(0.0f, 0.0f, -1.0f, 0.0f);					// Ogre front direction is is -z axis
		carCordenatSytem.m_up = dVector(0.0f, 1.0f, 0.0f, 0.0f);					// Ogre vertical direction is y up axis
		carCordenatSytem.m_right = carCordenatSytem.m_front * carCordenatSytem.m_up;	// tire coordinate system perpendicular front and up
		carCordenatSytem.m_posit = dVector(0.0f, 0.0f, 0.0f, 1.0f);					// this most be a zero vector

		controller = new CustomDGRayCastCar(maxTiresCount, carCordenatSytem, carBody->getNewtonBody());
		SetSupportJoint(controller);

		carBody->setNodeUpdateNotify<RayCastVehicle>(&RayCastVehicle::setTireTransformCallback, this);
	}

	RayCastVehicle::~RayCastVehicle()
	{
		if (m_debugInfo.size() > 0)
		{
			for (std::vector<DebugInfo>::iterator it(m_debugInfo.begin()); it != m_debugInfo.end(); it++)
			{
				DebugInfo& info = (*it);

				if (info.m_node->getParent())
				{
					info.m_node->getParent()->removeChild(info.m_node);
				}
				info.m_node->detachAllObjects();

				//			delete info.m_node;
				delete info.m_visualDebug;
				// this->m_sceneManager->destroyManualObject(info.m_visualDebug);

				info.m_node = nullptr;
				info.m_visualDebug = nullptr;
			}
			m_debugInfo.clear();
		}
	}


	void RayCastVehicle::AddSingleSuspensionTire(void *userData, const Ogre::Vector3& localPosition,
		Ogre::Real mass, Ogre::Real radius, Ogre::Real width, Ogre::Real friction,
		Ogre::Real susLength, Ogre::Real susSpring, Ogre::Real susShock, bool useConvexCast)
	{
		CustomDGRayCastCar* veh = (CustomDGRayCastCar*)m_joint;

		dVector location(localPosition.x, localPosition.y, localPosition.z, 0.0f);
		veh->AddSingleSuspensionTire(userData, location, mass, radius, width, friction, susLength, susSpring, susShock, useConvexCast ? 1 : 0);
	}

	void RayCastVehicle::setSteeringTorque(Ogre::Radian angle, Ogre::Real torque)
	{
		CustomDGRayCastCar* veh = (CustomDGRayCastCar*)m_joint;

		veh->SetTireSteerAngleForce(0, static_cast<float>(angle.valueRadians()), 1.0f);
		veh->SetTireSteerAngleForce(3, static_cast<float>(angle.valueRadians()), 1.0f);
		veh->SetTireTorque(1, torque);
		veh->SetTireTorque(2, torque);
	}

	void RayCastVehicle::setTireBrake(int index, Ogre::Real torque)
	{
		CustomDGRayCastCar* veh = (CustomDGRayCastCar*)m_joint;
		veh->SetTireBrake(index, torque);
	}

	void RayCastVehicle::setTireTorque(int index, Ogre::Real torque)
	{
		CustomDGRayCastCar* veh = (CustomDGRayCastCar*)m_joint;
		veh->SetTireTorque(index, torque);
	}

	void RayCastVehicle::setTireSteerAngleForce(int index, Ogre::Radian angle, Ogre::Real turnForce)
	{
		CustomDGRayCastCar* veh = (CustomDGRayCastCar*)m_joint;
		veh->SetTireSteerAngleForce(index, static_cast<float>(angle.valueRadians()), turnForce);
	}

	int RayCastVehicle::GetTiresCount() const
	{
		CustomDGRayCastCar* veh = (CustomDGRayCastCar*)m_joint;
		return veh->GetTiresCount();
	}

	void RayCastVehicle::setTireRollingResistance(int index, Ogre::Real rollingResitanceCoefficient)
	{
		CustomDGRayCastCar* veh = (CustomDGRayCastCar*)m_joint;
		veh->SetTireRollingResistance(index, rollingResitanceCoefficient);
	}

	//! show joint visual debugging data
	/*!
		implement its own version of visual debugging
	*/
	void RayCastVehicle::showDebugData(Ogre::SceneManager* sceneManager, Ogre::SceneNode* debugRootNode)
	{
		int tireCount;
		Body* carBody = getBody0();
		CustomDGRayCastCar* veh = (CustomDGRayCastCar*)m_joint;

		Ogre::Vector3 pos;
		Ogre::Quaternion orient;

		carBody->getVisualPositionOrientation(pos, orient);
		dMatrix carMatrix(dQuaternion(orient.w, orient.x, orient.y, orient.z), dVector(pos.x, pos.y, pos.z, 1.0f));
		//		dMatrix carMatrixInv (carMatrix.Inverse());
		//		dMatrix rootMatrixInv (veh->m_tireOffsetMatrix * carMatrix);
		//		rootMatrixInv = rootMatrixInv.Inverse();
		//		dMatrix rootMatrixInv (carMatrix);


		tireCount = GetTiresCount();
		for (int i = 0; i < tireCount; i++)
		{
			DebugInfo* infoEntry = nullptr;
			CustomDGRayCastCar::Tire& tire = veh->GetTire(i);
			for (std::vector<DebugInfo>::iterator it(m_debugInfo.begin()); it != m_debugInfo.end(); it++)
			{
				DebugInfo& info = (*it);
				if ((void*)&tire == info.m_tirePointer)
				{
					infoEntry = &info;
					break;
				}
			}

			if (!infoEntry)
			{

				DebugInfo newInfo;
				std::ostringstream oss;
				Ogre::Vector3 pos;
				Ogre::Quaternion orient;

				oss << "__OgreNewt__Debugger__Lines__" << &tire << "__";
				newInfo.m_visualDebug = sceneManager->createManualObject();
				// newInfo.m_visualDebug = new Ogre::ManualObject(0, &sceneManager->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), sceneManager);
				newInfo.m_visualDebug->setName(oss.str());
				newInfo.m_node = debugRootNode->createChildSceneNode();
				newInfo.m_tirePointer = &tire;

				OgreNewt::Debugger& debug(carBody->getWorld()->getDebugger());

				OgreNewt::Collision tireCollision(tire.m_shape, carBody->getWorld());
				debug.buildDebugObjectFromCollision(newInfo.m_visualDebug, Ogre::ColourValue(0, 0, 1, 1), tireCollision);

				newInfo.m_node->attachObject(newInfo.m_visualDebug);

				// append this debug info to the array
				m_debugInfo.push_back(newInfo);
				infoEntry = &m_debugInfo[m_debugInfo.size() - 1];
			}

			assert(infoEntry);
			// calculate the tire local matrix
			dMatrix tireBaseMatrix(veh->CalculateSuspensionMatrix(i, tire.m_posit) * veh->GetChassisMatrixLocal() * carMatrix);

			Converters::MatrixToQuatPos(&tireBaseMatrix[0][0], orient, pos);

			infoEntry->m_node->setPosition(pos);
			infoEntry->m_node->setOrientation(orient);
			if (!infoEntry->m_node->getParent())
			{
				debugRootNode->addChild(infoEntry->m_node);
			}
		}
	}


	void RayCastVehicle::setTireTransformCallback(OgreNewt::Body* carBody)
	{
		int tireCount;
		CustomDGRayCastCar* veh = (CustomDGRayCastCar*)m_joint;

		assert(getBody0() == carBody);

		//		carBody->getVisualPositionOrientation (pos, orient);
		//		dMatrix carMatrix (dQuaternion (orient.w, orient.x, orient.y, orient.z), dVector (pos.x, pos.y, pos.z, 1.0f));
		//		dMatrix carMatrixInv (carMatrix.Inverse());
		//		dMatrix rootMatrixInv (veh->m_tireOffsetMatrix * carMatrix);
		//		rootMatrixInv = rootMatrixInv.Inverse();
		//		dMatrix rootMatrixInv (carMatrix);
		tireCount = GetTiresCount();

		//iterate over all tires calling the transform callback to set the visual tire matrix
		for (int i = 0; i < tireCount; i++)
		{
			// calculate the tire local matrix
			Ogre::Vector3 pos;
			Ogre::Quaternion orient;
			CustomDGRayCastCar::Tire& tire = veh->GetTire(i);

			dMatrix matrix(veh->CalculateTireMatrix(i));
			Ogre::Matrix4 tireMatrix;
			OgreNewt::Converters::MatrixToMatrix4(&matrix[0][0], tireMatrix);
			setTireTransform(tire.m_userData, tireMatrix);
		}
	}


#if 0
	// destructor
	Vehicle::Tire::~Tire()
	{
		// remove the tire from the vehicle.
		NewtonVehicleRemoveTire(m_vehicle->getNewtonVehicle(), m_tireid);

	}


	void Vehicle::Tire::updateNode()
	{
		if (!m_node)
			return;

		Ogre::Quaternion orient;
		Ogre::Vector3 pos;

		getPositionOrientation(orient, pos);
		m_node->setOrientation(orient);
		m_node->setPosition(pos);
	}

	void Vehicle::Tire::getPositionOrientation(Ogre::Quaternion& orient, Ogre::Vector3& pos)
	{
		float matrix[16];

		NewtonVehicleGetTireMatrix(m_vehicle->getNewtonVehicle(), m_tireid, matrix);
		OgreNewt::Converters::MatrixToQuatPos(matrix, orient, pos);
	}



	///////////////////////////////////////////////////////////////////
	// VEHICLE FUNCTIONS




	void Vehicle::init(OgreNewt::Body* chassis, const Ogre::Vector3& updir)
	{
		// setup the basic vehicle.
		m_chassis = chassis;

		m_vehicle = NewtonConstraintCreateVehicle(chassis->getWorld()->getNewtonWorld(), &updir.x, chassis->getNewtonBody());

		// set the user data
		NewtonJointSetUserData(m_vehicle, this);
		NewtonJointSetDestructor(m_vehicle, newtonDestructor);

		//set the tire callback.
		NewtonVehicleSetTireCallback(m_vehicle, newtonCallback);

		//now call the user setup function
		setup();

	}

	void Vehicle::destroy()
	{
		// don't let newton call the destructor.
		NewtonJointSetDestructor(m_vehicle, nullptr);

		// destroy the chassis.
		if (m_chassis)
			delete m_chassis;

		// joint is now destroyed.
		m_vehicle = nullptr;
	}

	// get first tire.
	const OgreNewt::Vehicle::Tire* Vehicle::getFirstTire() const
	{
		Vehicle::Tire* tire = nullptr;
		void* id = 0;

		id = NewtonVehicleGetFirstTireID(m_vehicle);

		if (id)
			tire = (Vehicle::Tire*)NewtonVehicleGetTireUserData(m_vehicle, id);

		return tire;
	}

	// get next tire.
	const OgreNewt::Vehicle::Tire* Vehicle::getNextTire(Vehicle::Tire* current_tire) const
	{
		Vehicle::Tire* tire = nullptr;
		void* id = 0;

		id = NewtonVehicleGetNextTireID(m_vehicle, (void*)current_tire->getNewtonID());

		if (id)
			tire = (Vehicle::Tire*)NewtonVehicleGetTireUserData(m_vehicle, id);

		return tire;
	}


	// Newton callback.
	void _CDECL Vehicle::newtonCallback(const NewtonJoint* me)
	{
		OgreNewt::Vehicle* vehicle;

		vehicle = (Vehicle*)NewtonJointGetUserData(me);

		vehicle->userCallback();
	}

	void _CDECL Vehicle::newtonDestructor(const NewtonJoint* me)
	{
		Vehicle* vehicle;

		vehicle = (Vehicle*)NewtonJointGetUserData(me);

		NewtonJointSetDestructor(me, nullptr);
		NewtonJointSetUserData(me, nullptr);

		delete vehicle;
	}
#endif

}   // end NAMESPACE OgreNewt



