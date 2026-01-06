/*
OgreNewt Library 4.0

Ogre implementation of Newton Game Dynamics SDK 4.0
*/

#ifndef _INCLUDE_OGRENEWT_VEHICLENOTIFY
#define _INCLUDE_OGRENEWT_VEHICLENOTIFY

#include "OgreNewt_BodyNotify.h"

namespace OgreNewt
{
    class Vehicle;

    class _OgreNewtExport VehicleNotify : public BodyNotify
    {
    public:
        explicit VehicleNotify(Vehicle* vehicle);

        void OnApplyExternalForce(ndInt32 threadIndex, ndFloat32 timestep) override;

        void OnTransform(ndFloat32 timestep, const ndMatrix& matrix) override;

    private:
        Vehicle* m_vehicle;
    };

    class ComplexVehicle;

    class _OgreNewtExport ComplexVehicleNotify : public BodyNotify
    {
    public:
        explicit ComplexVehicleNotify(ComplexVehicle* vehicle);

        void OnApplyExternalForce(ndInt32 threadIndex, ndFloat32 timestep) override;
    private:
        ComplexVehicle* m_vehicle;
    };
}

#endif
