/**
*
*	@author Takahiro HARADA
*
*/
#ifndef F4_QUATERNION_H
#define F4_QUATERNION_H

#include <vector_types.h>
#include <vector_functions.h>
#include "mathConstants.h"
#include "Matrix3x3.h"
#include "../src/cutil_math.h"


inline
__device__
float4 cross(float4 a, float4 b)
{
	float3 ans = cross(make_float3(a), make_float3(b));
	return make_float4(ans, 0);
}


struct Quaternion : public float4
{
	public:
		inline
		__device__
		Quaternion(){};
		inline
		__device__
		Quaternion(const float4& a);
		inline
		__device__
		void setAngularVelocity(const float4& w, float dt);
		inline
		__device__
		void setCross3(const Quaternion& a, const Quaternion& b);
		inline
		__device__
		void getRotationMatrix(Matrix3x3& out) const;
		inline
		__device__
		void invert();
		inline
		__device__
		void normalizeQ();
		inline
		__device__
		friend Quaternion operator * (const Quaternion& a, const Quaternion& b);
		inline
		__device__
		void rotateVector(const float4& in, float4& out) const;
};

__device__
Quaternion::Quaternion(const float4& a)
{
	*this = *(Quaternion*)&a;
}

__device__
void Quaternion::setAngularVelocity(const float4& w, float dt)
{
	float4& ans = *this;
	
	float4 wdt = w*dt; wdt.w = 0.f;
	float lengthf = length(wdt);// wdt.length3()[0];
	float halfTheta = lengthf*0.5f;
	
	ans = w; ans.w = 0.f;
	ans = normalize(ans);
	ans *= sin( halfTheta );
	ans.w = cos( halfTheta );

	if( lengthf < EPSILON )
		ans = make_float4(0,0,0,1);
}

__device__
void Quaternion::setCross3(const Quaternion& a, const Quaternion& b)
{
	float4& ans = *this;
	const float4& aV = (const float4&)a;
	const float4& bV = (const float4&)b;
	ans = cross(aV, bV);//(aV.cross3(bV));
}

__device__
void Quaternion::getRotationMatrix(Matrix3x3& out) const
{
	const float4& quat = *this;
	float4 quat2 = make_float4(quat.x*quat.x, quat.y*quat.y, quat.z*quat.z, 0.f);
	out.setZero();

	out[0].x=1-2*quat2.y-2*quat2.z;
	out[0].y=2*quat.x*quat.y-2*quat.w*quat.z;
	out[0].z=2*quat.x*quat.z+2*quat.w*quat.y;
	out[1].x=2*quat.x*quat.y+2*quat.w*quat.z;
	out[1].y=1-2*quat2.x-2*quat2.z;
	out[1].z=2*quat.y*quat.z-2*quat.w*quat.x;
	out[2].x=2*quat.x*quat.z-2*quat.w*quat.y;
	out[2].y=2*quat.y*quat.z+2*quat.w*quat.x;
	out[2].z=1-2*quat2.x-2*quat2.y;
}

__device__
void Quaternion::invert()
{
	float4& ans = *this;
	ans *= -1;
	ans.w *= -1;
}

__device__
void Quaternion::normalizeQ()
{
	float4& ans = *this;
	ans /= length(ans);
/*
	float4 l = length(ans);//ans.length4();
	l.y = l.x;
	l.z = l.x;
	l.w = l.x;
	ans /= l;
*/
}

__device__
Quaternion operator * (const Quaternion& a, const Quaternion& b)
{
	Quaternion ans;
//	ans.setCross3(a, b);
	ans = cross(a, b);
	ans += a.w * b + b.w * a;
	ans.w = a.w * b.w - dot(make_float3(a), make_float3(b));//dot3(a, b);
	return ans;
}

__device__
void Quaternion::rotateVector(const float4& in, float4& out) const
{
	const Quaternion& q = *this;
	Quaternion qInv = q; qInv.invert();
	
	out = q * (Quaternion&)in * qInv;
}



#endif

