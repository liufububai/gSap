/**
*
*	@author Takahiro HARADA
*
*/
#ifndef _TK_MATRIX3_
#define _TK_MATRIX3_

#ifdef _WIN32
#ifdef RIGIDBODYSYSTEM_EXPORTS
#define DLL_API_RIGIDBODYSYSTEM __declspec(dllexport)
#else
#define DLL_API_RIGIDBODYSYSTEM __declspec(dllimport)
#endif
#else
#define DLL_API_RIGIDBODYSYSTEM
#endif

#include <assert.h>
#include "../src/cutil_math.h"

#ifdef __CUDACC__
#define CUASSERT(x)
#else
#define CUASSERT(x) assert(x);
#endif


inline
__device__
float dot3(const float4& a, const float4& b)
{
	return dot(make_float3(a), make_float3(b));
}

class
DLL_API_RIGIDBODYSYSTEM
__builtin_align__(16)
Matrix3x3
{

public:
	__device__
	void setZero()
	{
		m_row[0] = make_float4(0,0,0,0);
		m_row[1] = make_float4(0,0,0,0);
		m_row[2] = make_float4(0,0,0,0);
	}
	__device__
	void setIdentity()
	{
		setZero();
		m_row[0].x = 1.0f;
		m_row[1].y = 1.0f;
		m_row[2].z = 1.0f;
	}
//	void setRow(const float& r0,const float& r1,const float& r2,const float& r3);
	__device__
	void setDiagMatrix(const float4& diag)
	{
		setZero();
		m_row[0].x= diag.x;
		m_row[1].y = diag.y;
		m_row[2].z = diag.z;
	}
	__device__
	void transpose()
	{
		Matrix3x3 oldMatrix(*this);
		setTranspose( oldMatrix );
	}
	__device__
	void setTranspose(const Matrix3x3& m)
	{
		//for(int i=0; i<3; i++)
		//{
		//	for(int j=0; j<3; j++)
		//	{
		//		m_row[i][j] = m[j][i];
		//	}
		//}
		m_row[0] = make_float4(m[0].x, m[1].x, m[2].x,0);
		m_row[1] = make_float4(m[0].y, m[1].y, m[2].y,0);
		m_row[2] = make_float4(m[0].z, m[1].z, m[2].z,0);
	}
	

	__device__
	Matrix3x3(){}
	__device__
	Matrix3x3(const Matrix3x3& a)
	{
		m_row[0] = a[0];
		m_row[1] = a[1];
		m_row[2] = a[2];
	}
	__device__
	Matrix3x3(const float4& r0,const float4& r1,const float4& r2)
	{
		m_row[0] = r0;
		m_row[1] = r1;
		m_row[2] = r2;
	}
	__device__
	Matrix3x3& operator = (float m)
	{
		m_row[0] = make_float4(m,m,m,0);
		m_row[1] = make_float4(m,m,m,0);
		m_row[2] = make_float4(m,m,m,0);
		return *this;
	}
	__device__
	Matrix3x3& operator +=(const Matrix3x3& a)
	{
		m_row[0] = m_row[0]+a[0];
		m_row[1] = m_row[1]+a[1];
		m_row[2] = m_row[2]+a[2];
		return *this;
	}
	__device__
	Matrix3x3& operator -=(const Matrix3x3& a)
	{
		m_row[0] = m_row[0]-a[0];
		m_row[1] = m_row[1]-a[1];
		m_row[2] = m_row[2]-a[2];
		return *this;
	}
	__device__
	Matrix3x3& operator *=(const Matrix3x3& a)
	{
//		Matrix3x3 transA;
//		transA.setTranspose(a);

		Matrix3x3 old(*this);
		
		*this = old*a;

		return *this;
	}
	__device__
	Matrix3x3& operator *=(const float& a)
	{
		//Matrix3x3& m = *this;
		//m[0].x *= a;
		//m[0].y *= a;
		//m[0].z *= a;
		//m[1].x *= a;
		//m[1].y *= a;
		//m[1].z *= a;
		//m[2].x *= a;
		//m[2].y *= a;
		//m[2].z *= a;
		m_row[0] *= a;
		m_row[1] *= a;
		m_row[2] *= a;
		return *this;
	}
	__device__
	friend Matrix3x3 operator +(const Matrix3x3& a,const Matrix3x3& b)
	{
//		Matrix3x3 m(a);
//		m += b;
//		return m;
		return Matrix3x3(a[0]+b[0], a[1]+b[1], a[2]+b[2]);
	}
	__device__
	friend Matrix3x3 operator -(const Matrix3x3& a,const Matrix3x3& b)
	{
//		Matrix3x3 m(a);
//		a-=b;
//		return m;
		return Matrix3x3(a[0]-b[0], a[1]-b[1], a[2]-b[2]);
	}
	inline
	__device__
	friend Matrix3x3 operator *(const Matrix3x3& a,const Matrix3x3& b)
	{
		//Matrix3x3 transB;
		//transB.setTranspose(b);

		//Matrix3x3 ans;
		//for(int i=0; i<3; i++)
		//{
		//	for(int j=0; j<3; j++)
		//	{
		//		ans[i][j] = dot3(a[i],transB[j]);
		//	}
		//}
		//return ans;

		Matrix3x3 transB;
		transB.setTranspose(b);

		Matrix3x3 ans;
//		for(int i=0; i<3; i++)
		{
			int i=0;
			ans[i].x = dot3(a[i],transB[0]);
			ans[i].y = dot3(a[i],transB[1]);
			ans[i].z = dot3(a[i],transB[2]);
		}
		{
			int i=1;
			ans[i].x = dot3(a[i],transB[0]);
			ans[i].y = dot3(a[i],transB[1]);
			ans[i].z = dot3(a[i],transB[2]);
		}
		{
			int i=2;
			ans[i].x = dot3(a[i],transB[0]);
			ans[i].y = dot3(a[i],transB[1]);
			ans[i].z = dot3(a[i],transB[2]);
		}
		return ans;
	}
	__device__
	friend float4 operator *(const Matrix3x3& a,const float4& b)
	{
		float4 ans;
		//ans[0] = (a[0].dot3(b))[0];
		//ans[1] = (a[1].dot3(b))[0];
		//ans[2] = (a[2].dot3(b))[0];
		ans.x = dot3(a[0], b);
		ans.y = dot3(a[1], b);
		ans.z = dot3(a[2], b);
		return ans;
	}

	inline
		__device__
	friend float4 operator *(const float4& a,const Matrix3x3& b)
	{
		float4 ans;
		ans.x = a.x*b[0].x + a.y*b[1].x + a.z*b[2].x;
		ans.y = a.x*b[0].y + a.y*b[1].y + a.z*b[2].y;
		ans.z = a.x*b[0].z + a.y*b[1].z + a.z*b[2].z;
		return ans;
	}

	__device__
	friend Matrix3x3 operator *(const Matrix3x3& a,const float& b)
	{
		return Matrix3x3(a[0]*b, a[1]*b, a[2]*b);
	}
	__device__
	friend Matrix3x3 operator /(const Matrix3x3& a,const float& b)
	{
		return Matrix3x3(a[0]/b, a[1]/b, a[2]/b);
	}
	__device__
	Matrix3x3& operator = (const Matrix3x3& m)
	{
		m_row[0] = m[0];
		m_row[1] = m[1];
		m_row[2] = m[2];
		return *this;
	}


/*
	friend std::ostream& operator <<(std::ostream& os, const Matrix3x3& m)
	{
		os << m[0][0] << "," << m[0][1] << "," << m[0][2] << endl
		<< m[1][0] << "," << m[1][1] << "," << m[1][2] << endl
		<< m[2][0] << "," << m[2][1] << "," << m[2][2] ;
		return os;
	}
*/
	__device__
	const float4& operator [](int i) const
	{
		CUASSERT((0 <= i)&&(i <= 2));
		return m_row[i];
	}
	__device__
	float4& operator [](int i)
	{
		CUASSERT((0 <= i)&&(i <= 2));
		return m_row[i];
	}

	__device__
	void invert()
	{
		Matrix3x3& m = *this;

		float det = m[0].x*m[1].y*m[2].z+m[1].x*m[2].y*m[0].z+m[2].x*m[0].y*m[1].z
		-m[0].x*m[2].y*m[1].z-m[2].x*m[1].y*m[0].z-m[1].x*m[0].y*m[2].z;

		Matrix3x3 ans;
		ans[0].x = m[1].y*m[2].z - m[1].z*m[2].y;
		ans[0].y = m[0].z*m[2].y - m[0].y*m[2].z;
		ans[0].z = m[0].y*m[1].z - m[0].z*m[1].y;

		ans[1].x = m[1].z*m[2].x - m[1].x*m[2].z;
		ans[1].y = m[0].x*m[2].z - m[0].z*m[2].x;
		ans[1].z = m[0].z*m[1].x - m[0].x*m[1].z;

		ans[2].x = m[1].x*m[2].y - m[1].y*m[2].x;
		ans[2].y = m[0].y*m[2].x - m[0].x*m[2].y;
		ans[2].z = m[0].x*m[1].y - m[0].y*m[1].x;

		m = ans * (1.0f/det);
	}
/*
	void setTensorProduct(const float4& a,const float4& b)
	{
		Matrix3x3& m = *this;
		for(int i=0; i<3; i++)
			for(int j=0; j<3; j++)
			{
				m[i][j] = a[i] * b[j];
			}
	}
	Matrix3x3 mulKroneckerDelta()
	{
		Matrix3x3& m = *this;
		Matrix3x3 a;
		float4 diag(m[2][2],m[1][1],m[0][0]);
		a.setDiagMatrix(diag);
		return a;
	}
*/

public:
	float4 m_row[3];
};



#endif

