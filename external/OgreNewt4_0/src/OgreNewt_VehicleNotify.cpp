#include "OgreNewt_Stdafx.h"
#include "OgreNewt_VehicleNotify.h"
#include "OgreNewt_Vehicle.h"
#include "OgreNewt_ComplexVehicle.h"

namespace OgreNewt
{
    VehicleNotify::VehicleNotify(Vehicle* vehicle)
        : BodyNotify(vehicle), 
        m_vehicle(vehicle)
    {

    }

    void VehicleNotify::OnApplyExternalForce(ndInt32 threadIndex, ndFloat32 timestep)
    {
        BodyNotify::OnApplyExternalForce(threadIndex, timestep);

        // This is ND4's "pre-update" hook during the physics step.
        // Do NOT do any render/scene node touching here - only physics + raycasts + forces.
        if (m_vehicle)
            m_vehicle->physicsPreUpdate((Ogre::Real)timestep, (int)threadIndex);
    }

    void VehicleNotify::OnTransform(ndFloat32 timestep, const ndMatrix& matrix)
    {
        BodyNotify::OnTransform(timestep, matrix);
        // This is ND4's "post-solve transform" hook.
        // Update the chassis cached transform and update the TIRE PROXY transforms here
        // (same place as ND3 OnUpdateTransform).
        if (m_vehicle)
            m_vehicle->physicsOnTransform(matrix);
    }

    ///////////////////////////////////////////////////////

    ComplexVehicleNotify::ComplexVehicleNotify(ComplexVehicle* vehicle)
        : BodyNotify(vehicle),
        m_vehicle(vehicle)
    {

    }

    void ComplexVehicleNotify::OnApplyExternalForce(ndInt32 threadIndex, ndFloat32 timestep)
    {
        // keep default behavior (gravity/callbacks you already have)
        BodyNotify::OnApplyExternalForce(threadIndex, timestep);

        // IMPORTANT: this is now called at the real ND4 physics substep dt.
        // if (m_vehicle)
           //  m_vehicle->update((Ogre::Real)timestep, (int)threadIndex);
    }
}
