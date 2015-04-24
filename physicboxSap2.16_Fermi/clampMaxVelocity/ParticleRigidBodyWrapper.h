/**
*
*	@author Takahiro HARADA
*
*/
#ifndef PRB_WRAPPER_H
#define PRB_WRAPPER_H

#include "../include/ParticleRigidBody.h"

extern "C"
{
	namespace PRBWrap
	{
		void integrate(dim3 dimGrid,dim3 dimBlock, 
			float4* pos, Quaternion* quat, 
			float4* linVel, float4* angVel, 
			float dt, int size);

		void upadteParticleData(dim3 dimGrid,dim3 dimBlock, 
			float4* pos, Quaternion* quat, 
			float4* linVel, float4* angVel, 
			int* shapeIdxBuf, int* pBufStartOffsetBuf, int* numParticles, 
			float4* particleRefPosBuf, float* particleRefMassBuf,
			float4* particlePosOut, float4* particleVelOut, float* particleMassOut, int size);

		void clearForceAndBoundaryCondition(dim3 dimGrid,dim3 dimBlock, 
			float4* pPos, float4* pVel, float* pMass, 
			float4* pForceOut, 
			float spMultip, float dpMultip, int numParticles, float dt);

		void updateVelocity(dim3 dimGrid,dim3 dimBlock, 
			float4* linVel, float4* angVel, 
			Quaternion* quat, float* invMass, Matrix3x3* invInertia, 
			int* shapeIdxBuf, int* pBufStartOffsetBuf, int* numParticles, 
			float4* particleRefPosBuf, 
			float4* particleForceBuf, int numBodies, float dt,
			float maxLinVel, bool capVel);

		void prepare(dim3 dimGrid,dim3 dimBlock, 
			Quaternion* quat, int* shapeIdxBuf,
			Matrix3x3* shapeInvInertia, 
			Matrix3x3* rbInvInertiaOut, int size);

		void computeFoceOnParticles(dim3 dimGrid,dim3 dimBlock, 
			float4* pPos, float4* pVel, 
			int2* pairs, int* pairEntries, int numPairEntries, 
			float* particleMass,
			float spMultip, float dpMultip, 
			float4* pForceOut, float dt);


	}
}

#endif

