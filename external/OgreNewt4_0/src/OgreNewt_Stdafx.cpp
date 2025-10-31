#include "OgreNewt_Stdafx.h"

#include "ndVector.h"

// These fix unresolved externals for static ndVector constants.
ndVector ndVector::m_zero(0.0f);
ndVector ndVector::m_one(1.0f);
ndVector ndVector::m_wOne(0.0f, 0.0f, 0.0f, 1.0f);
ndVector ndVector::m_half(0.5f);
ndVector ndVector::m_two(2.0f);
ndVector ndVector::m_three(3.0f);
ndVector ndVector::m_negOne(-1.0f);


