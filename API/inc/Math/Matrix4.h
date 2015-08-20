#pragma once

#include "Vector4.h"
#include "Vector3.h"

namespace VectorMath
{
	/**
	 * 4x4 Transformation Matrix.
	 *
	 * Matrix operations are right to left.  
	 * The matrix is stored as float[16] and organized in row-major order.  
	 * Entry of row 'r' and column 'c' is stored at: i = c + (4 * r)
	 */
	class Matrix4
	{
	public:
		/**
		 * Constructor
		 */
		Matrix4()
			{
			*this = Matrix4::IDENTITY;
			}
		
		/**
		 * Constructs the matrix with specific values.
		 */
		Matrix4 (	float fM00, float fM01, float fM02, float fM03,
         					float fM10, float fM11, float fM12, float fM13,
         					float fM20, float fM21, float fM22, float fM23,
         					float fM30, float fM31, float fM32, float fM33)
		{
#define I(r, c) (c * 4 + r)
			mEntry[I(0, 0)] = fM00;
			mEntry[I(0, 1)] = fM01;
			mEntry[I(0, 2)] = fM02;
			mEntry[I(0, 3)] = fM03;
			mEntry[I(1, 0)] = fM10;
			mEntry[I(1, 1)] = fM11;
			mEntry[I(1, 2)] = fM12;
			mEntry[I(1, 3)] = fM13;
			mEntry[I(2, 0)] = fM20;
			mEntry[I(2, 1)] = fM21;
			mEntry[I(2, 2)] = fM22;
			mEntry[I(2, 3)] = fM23;
			mEntry[I(3, 0)] = fM30;
			mEntry[I(3, 1)] = fM31;
			mEntry[I(3, 2)] = fM32;
			mEntry[I(3, 3)] = fM33;
#undef I
		}

	public:

		/**
		 * Multiplies this matrix with another.
		 * Operation is from left to right, ie (*this) * rkM is the
		 * transform of (*this) followed by rkM
		 */
		Matrix4 operator* (const Matrix4& rkM) const
		{
#define I(r, c) (c * 4 + r)
			return Matrix4(
					mEntry[I(0, 0)] * rkM.mEntry[I(0, 0)] + mEntry[I(0, 1)] * rkM.mEntry[I(1, 0)] + mEntry[I(0, 2)] * rkM.mEntry[I(2, 0)] + mEntry[I(0, 3)] * rkM.mEntry[I(3, 0)],
					mEntry[I(0, 0)] * rkM.mEntry[I(0, 1)] + mEntry[I(0, 1)] * rkM.mEntry[I(1, 1)] + mEntry[I(0, 2)] * rkM.mEntry[I(2, 1)] + mEntry[I(0, 3)] * rkM.mEntry[I(3, 1)],
					mEntry[I(0, 0)] * rkM.mEntry[I(0, 2)] + mEntry[I(0, 1)] * rkM.mEntry[I(1, 2)] + mEntry[I(0, 2)] * rkM.mEntry[I(2, 2)] + mEntry[I(0, 3)] * rkM.mEntry[I(3, 2)],
					mEntry[I(0, 0)] * rkM.mEntry[I(0, 3)] + mEntry[I(0, 1)] * rkM.mEntry[I(1, 3)] + mEntry[I(0, 2)] * rkM.mEntry[I(2, 3)] + mEntry[I(0, 3)] * rkM.mEntry[I(3, 3)],
					mEntry[I(1, 0)] * rkM.mEntry[I(0, 0)] + mEntry[I(1, 1)] * rkM.mEntry[I(1, 0)] + mEntry[I(1, 2)] * rkM.mEntry[I(2, 0)] + mEntry[I(1, 3)] * rkM.mEntry[I(3, 0)],
					mEntry[I(1, 0)] * rkM.mEntry[I(0, 1)] + mEntry[I(1, 1)] * rkM.mEntry[I(1, 1)] + mEntry[I(1, 2)] * rkM.mEntry[I(2, 1)] + mEntry[I(1, 3)] * rkM.mEntry[I(3, 1)],
					mEntry[I(1, 0)] * rkM.mEntry[I(0, 2)] + mEntry[I(1, 1)] * rkM.mEntry[I(1, 2)] + mEntry[I(1, 2)] * rkM.mEntry[I(2, 2)] + mEntry[I(1, 3)] * rkM.mEntry[I(3, 2)],
					mEntry[I(1, 0)] * rkM.mEntry[I(0, 3)] + mEntry[I(1, 1)] * rkM.mEntry[I(1, 3)] + mEntry[I(1, 2)] * rkM.mEntry[I(2, 3)] + mEntry[I(1, 3)] * rkM.mEntry[I(3, 3)],
					mEntry[I(2, 0)] * rkM.mEntry[I(0, 0)] + mEntry[I(2, 1)] * rkM.mEntry[I(1, 0)] + mEntry[I(2, 2)] * rkM.mEntry[I(2, 0)] + mEntry[I(2, 3)] * rkM.mEntry[I(3, 0)],
					mEntry[I(2, 0)] * rkM.mEntry[I(0, 1)] + mEntry[I(2, 1)] * rkM.mEntry[I(1, 1)] + mEntry[I(2, 2)] * rkM.mEntry[I(2, 1)] + mEntry[I(2, 3)] * rkM.mEntry[I(3, 1)],
					mEntry[I(2, 0)] * rkM.mEntry[I(0, 2)] + mEntry[I(2, 1)] * rkM.mEntry[I(1, 2)] + mEntry[I(2, 2)] * rkM.mEntry[I(2, 2)] + mEntry[I(2, 3)] * rkM.mEntry[I(3, 2)],
					mEntry[I(2, 0)] * rkM.mEntry[I(0, 3)] + mEntry[I(2, 1)] * rkM.mEntry[I(1, 3)] + mEntry[I(2, 2)] * rkM.mEntry[I(2, 3)] + mEntry[I(2, 3)] * rkM.mEntry[I(3, 3)],
					mEntry[I(3, 0)] * rkM.mEntry[I(0, 0)] + mEntry[I(3, 1)] * rkM.mEntry[I(1, 0)] + mEntry[I(3, 2)] * rkM.mEntry[I(2, 0)] + mEntry[I(3, 3)] * rkM.mEntry[I(3, 0)],
					mEntry[I(3, 0)] * rkM.mEntry[I(0, 1)] + mEntry[I(3, 1)] * rkM.mEntry[I(1, 1)] + mEntry[I(3, 2)] * rkM.mEntry[I(2, 1)] + mEntry[I(3, 3)] * rkM.mEntry[I(3, 1)],
					mEntry[I(3, 0)] * rkM.mEntry[I(0, 2)] + mEntry[I(3, 1)] * rkM.mEntry[I(1, 2)] + mEntry[I(3, 2)] * rkM.mEntry[I(2, 2)] + mEntry[I(3, 3)] * rkM.mEntry[I(3, 2)],
					mEntry[I(3, 0)] * rkM.mEntry[I(0, 3)] + mEntry[I(3, 1)] * rkM.mEntry[I(1, 3)] + mEntry[I(3, 2)] * rkM.mEntry[I(2, 3)] + mEntry[I(3, 3)] * rkM.mEntry[I(3, 3)] );
#undef I
		}

		/**
		 * Returns the inverse of the matrix.
		 */
		Matrix4 Inverse() const;
		
		/**
		 * Returns the transpose of the matrix
		 */
		Matrix4 Transpose() const;

		/**
		 * Matrix times vector
		 */
		Vector4 operator*(const Vector4& rkV) const
		{
#define I(r, c) (c * 4 + r)
			float const v0=rkV.x;
			float const v1=rkV.y;
			float const v2=rkV.z;
			float const v3=rkV.w;
			return Vector4(
				mEntry[I(0,0)]*v0 + mEntry[I(0,1)]*v1 + mEntry[I(0,2)]*v2 + mEntry[I(0,3)]*v3,
				mEntry[I(1,0)]*v0 + mEntry[I(1,1)]*v1 + mEntry[I(1,2)]*v2 + mEntry[I(1,3)]*v3,
				mEntry[I(2,0)]*v0 + mEntry[I(2,1)]*v1 + mEntry[I(2,2)]*v2 + mEntry[I(2,3)]*v3,
				mEntry[I(3,0)]*v0 + mEntry[I(3,1)]*v1 + mEntry[I(3,2)]*v2 + mEntry[I(3,3)]*v3);
#undef I
		}

		Vector3 TransformPoint( Vector3 const & rkV) const
		{
#define I(r, c) (c * 4 + r)
			float const v0=rkV.x;
			float const v1=rkV.y;
			float const v2=rkV.z;
			return Vector3(
				mEntry[I(0,0)]*v0 + mEntry[I(0,1)]*v1 + mEntry[I(0,2)]*v2 + mEntry[I(0,3)],
				mEntry[I(1,0)]*v0 + mEntry[I(1,1)]*v1 + mEntry[I(1,2)]*v2 + mEntry[I(1,3)],
				mEntry[I(2,0)]*v0 + mEntry[I(2,1)]*v1 + mEntry[I(2,2)]*v2 + mEntry[I(2,3)]);
#undef I
		}

	public:
	
		static const Matrix4 ZERO;
		static const Matrix4 IDENTITY;

		/**
		 * Constructs a translation matrix
		 */
		static Matrix4 Translation(float x, float y, float z);

		/**
		 * Constructs a rotation matrix about X
		 */
		static Matrix4 RotationX(float fAngle);

		/**
		 * Constructs a rotation matrix about Y
		 */
		static Matrix4 RotationY(float fAngle);

		/**
		 * Constructs a rotation matrix about Z
		 */
		static Matrix4 RotationZ(float fAngle);

	public:

		float mEntry[16];
	};
}
