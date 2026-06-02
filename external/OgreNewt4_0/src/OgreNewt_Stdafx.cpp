#include "OgreNewt_Stdafx.h"

#include "ndVector.h"

// These fix unresolved externals for static ndVector constants.
ndVector ndVector::m_xMask(ndInt32(-1), ndInt32(0), ndInt32(0), ndInt32(0));
ndVector ndVector::m_yMask(ndInt32(0), ndInt32(-1), ndInt32(0), ndInt32(0));
ndVector ndVector::m_zMask(ndInt32(0), ndInt32(0), ndInt32(-1), ndInt32(0));
ndVector ndVector::m_wMask(ndInt32(0), ndInt32(0), ndInt32(0), ndInt32(-1));
ndVector ndVector::m_xyzwMask(ndInt32(-1), ndInt32(-1), ndInt32(-1), ndInt32(-1));
ndVector ndVector::m_triplexMask(ndInt32(-1), ndInt32(-1), ndInt32(-1), ndInt32(0));
ndVector ndVector::m_signMask(ndVector(ndInt32(-1), ndInt32(-1), ndInt32(-1), ndInt32(-1)).ShiftRightLogical(1));

ndVector ndVector::m_zero(ndFloat32(0.0f));
ndVector ndVector::m_one(ndFloat32(1.0f));
ndVector ndVector::m_two(ndFloat32(2.0f));
ndVector ndVector::m_half(ndFloat32(0.5f));
ndVector ndVector::m_three(ndFloat32(3.0f));
ndVector ndVector::m_negOne(ndFloat32(-1.0f));
ndVector ndVector::m_epsilon(ndFloat32(1.0e-20f));
ndVector ndVector::m_wOne(ndFloat32(0.0f), ndFloat32(0.0f), ndFloat32(0.0f), ndFloat32(1.0f));
