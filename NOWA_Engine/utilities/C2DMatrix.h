#ifndef C2DMATRIX_H
#define C2DMATRIX_H
//------------------------------------------------------------------------
//
//  Name:   C2DMatrix.h
//
//  Author: Mat Buckland 2002
//
//  Desc:   2D Matrix class 
//  Took from book 'Programming GameAI by Example' because this matrix is made to
//	position and orientation from world to local space and vice versa
//
//------------------------------------------------------------------------
#include <math.h>
#include <vector>
#include "OgreVector3.h"

class C2DMatrix
{
private:

	struct Matrix
	{

		Ogre::Real _11, _12, _13;
		Ogre::Real _21, _22, _23;
		Ogre::Real _31, _32, _33;

		Matrix()
		{
			_11 = 0.0; _12 = 0.0; _13 = 0.0;
			_21 = 0.0; _22 = 0.0; _23 = 0.0;
			_31 = 0.0; _32 = 0.0; _33 = 0.0;
		}

	};

	Matrix m_Matrix;

	//multiplies m_Matrix with mIn
	inline void  MatrixMultiply(Matrix &mIn);


public:

	C2DMatrix()
	{
		//initialize the matrix to an identity matrix
		Identity();
	}

	//create an identity matrix
	inline void Identity();

	//create a transformation matrix
	inline void Translate(Ogre::Real x, Ogre::Real z);

	//create a scale matrix
	inline void Scale(Ogre::Real xScale, Ogre::Real zScale);

	//create a rotation matrix
	inline void  Rotate(Ogre::Real rotation);

	//create a rotation matrix from a fwd and side 2D vector
	inline void  Rotate(const Ogre::Vector3 &fwd, const Ogre::Vector3 &side);

	//applys a transformation matrix to a std::vector of points
	inline void TransformVector2Ds(std::vector<Ogre::Vector3> &vPoints);

	//applys a transformation matrix to a point
	inline void TransformVector2Ds(Ogre::Vector3 &vPoint);

	//accessors to the matrix elements
	void _11(Ogre::Real val)
	{
		m_Matrix._11 = val;
	}
	void _12(Ogre::Real val)
	{
		m_Matrix._12 = val;
	}
	void _13(Ogre::Real val)
	{
		m_Matrix._13 = val;
	}

	void _21(Ogre::Real val)
	{
		m_Matrix._21 = val;
	}
	void _22(Ogre::Real val)
	{
		m_Matrix._22 = val;
	}
	void _23(Ogre::Real val)
	{
		m_Matrix._23 = val;
	}

	void _31(Ogre::Real val)
	{
		m_Matrix._31 = val;
	}
	void _32(Ogre::Real val)
	{
		m_Matrix._32 = val;
	}
	void _33(Ogre::Real val)
	{
		m_Matrix._33 = val;
	}

};



//multiply two matrices together
inline void C2DMatrix::MatrixMultiply(Matrix &mIn)
{
	C2DMatrix::Matrix mat_temp;

	//first row
	mat_temp._11 = (m_Matrix._11*mIn._11) + (m_Matrix._12*mIn._21) + (m_Matrix._13*mIn._31);
	mat_temp._12 = (m_Matrix._11*mIn._12) + (m_Matrix._12*mIn._22) + (m_Matrix._13*mIn._32);
	mat_temp._13 = (m_Matrix._11*mIn._13) + (m_Matrix._12*mIn._23) + (m_Matrix._13*mIn._33);

	//second
	mat_temp._21 = (m_Matrix._21*mIn._11) + (m_Matrix._22*mIn._21) + (m_Matrix._23*mIn._31);
	mat_temp._22 = (m_Matrix._21*mIn._12) + (m_Matrix._22*mIn._22) + (m_Matrix._23*mIn._32);
	mat_temp._23 = (m_Matrix._21*mIn._13) + (m_Matrix._22*mIn._23) + (m_Matrix._23*mIn._33);

	//third
	mat_temp._31 = (m_Matrix._31*mIn._11) + (m_Matrix._32*mIn._21) + (m_Matrix._33*mIn._31);
	mat_temp._32 = (m_Matrix._31*mIn._12) + (m_Matrix._32*mIn._22) + (m_Matrix._33*mIn._32);
	mat_temp._33 = (m_Matrix._31*mIn._13) + (m_Matrix._32*mIn._23) + (m_Matrix._33*mIn._33);

	m_Matrix = mat_temp;
}

//applies a 2D transformation matrix to a std::vector of Vector2Ds
inline void C2DMatrix::TransformVector2Ds(std::vector<Ogre::Vector3> &vPoint)
{
	for (unsigned int i = 0; i < vPoint.size(); ++i)
	{
		Ogre::Real tempX = (m_Matrix._11*vPoint[i].x) + (m_Matrix._21*vPoint[i].z) + (m_Matrix._31);

		Ogre::Real tempZ = (m_Matrix._12*vPoint[i].x) + (m_Matrix._22*vPoint[i].z) + (m_Matrix._32);

		vPoint[i].x = tempX;

		vPoint[i].z = tempZ;

	}
}

//applies a 2D transformation matrix to a single Vector2D
inline void C2DMatrix::TransformVector2Ds(Ogre::Vector3 &vPoint)
{

	Ogre::Real tempX = (m_Matrix._11*vPoint.x) + (m_Matrix._21*vPoint.z) + (m_Matrix._31);

	Ogre::Real tempZ = (m_Matrix._12*vPoint.x) + (m_Matrix._22*vPoint.z) + (m_Matrix._32);

	vPoint.x = tempX;

	vPoint.z = tempZ;
}



//create an identity matrix
inline void C2DMatrix::Identity()
{
	m_Matrix._11 = 1; m_Matrix._12 = 0; m_Matrix._13 = 0;

	m_Matrix._21 = 0; m_Matrix._22 = 1; m_Matrix._23 = 0;

	m_Matrix._31 = 0; m_Matrix._32 = 0; m_Matrix._33 = 1;

}

//create a transformation matrix
inline void C2DMatrix::Translate(Ogre::Real x, Ogre::Real z)
{
	Matrix mat;

	mat._11 = 1; mat._12 = 0; mat._13 = 0;

	mat._21 = 0; mat._22 = 1; mat._23 = 0;

	mat._31 = x;    mat._32 = z;    mat._33 = 1;

	//and multiply
	MatrixMultiply(mat);
}

//create a scale matrix
inline void C2DMatrix::Scale(Ogre::Real xScale, Ogre::Real zScale)
{
	C2DMatrix::Matrix mat;

	mat._11 = xScale; mat._12 = 0; mat._13 = 0;

	mat._21 = 0; mat._22 = zScale; mat._23 = 0;

	mat._31 = 0; mat._32 = 0; mat._33 = 1;

	//and multiply
	MatrixMultiply(mat);
}


//create a rotation matrix
inline void C2DMatrix::Rotate(Ogre::Real rot)
{
	C2DMatrix::Matrix mat;

	Ogre::Real Sin = sin(rot);
	Ogre::Real Cos = cos(rot);

	mat._11 = Cos;  mat._12 = Sin; mat._13 = 0;

	mat._21 = -Sin; mat._22 = Cos; mat._23 = 0;

	mat._31 = 0; mat._32 = 0; mat._33 = 1;

	//and multiply
	MatrixMultiply(mat);
}


//create a rotation matrix from a 2D vector
inline void C2DMatrix::Rotate(const Ogre::Vector3 &fwd, const Ogre::Vector3 &side)
{
	C2DMatrix::Matrix mat;

	mat._11 = fwd.x;  mat._12 = fwd.z; mat._13 = 0;

	mat._21 = side.x; mat._22 = side.z; mat._23 = 0;

	mat._31 = 0; mat._32 = 0; mat._33 = 1;

	//and multiply
	MatrixMultiply(mat);
}

#endif
