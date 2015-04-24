/**
*
*	@author Takahiro HARADA
*
*/
#include "../include/ParticleRigidBody.h"
#include "../include/quaternion.h"
#include "../include/Transform.h"

__device__
const float e = 0.4f;

__device__
void calcSpringCoeff(float iMassInv, float jMassInv, float dt2Inv, float dtInv, 
					 float spMultip, float dpMultip, 
					 float& sCoeff, float& dCoeff)
{
	float m = iMassInv + jMassInv;
	m = 1/m;
	sCoeff = m*dt2Inv*spMultip;
	dCoeff = m*dtInv*(1.f-dpMultip);
}

__device__
float calcDampCoeff(float iMass, float jMass, float dtInv)
{
	return 0;
}

__global__
void prepareK(Quaternion* quat, int* shapeIdxBuf,
			  Matrix3x3* shapeInvInertia, 
			  Matrix3x3* rbInvInertiaOut, int size)
{
	int idx=blockIdx.x*blockDim.x+threadIdx.x;

	if( idx >= size ) return;
	Quaternion iQuat = quat[idx];
	Matrix3x3 invInertiaInit = shapeInvInertia[ shapeIdxBuf[idx] ];
	Matrix3x3 rMatrix;
	iQuat.getRotationMatrix( rMatrix );
	Matrix3x3 rMatrixT;
	rMatrixT.setTranspose( rMatrix );
	rbInvInertiaOut[idx] = (rMatrix*invInertiaInit)*rMatrixT;
}

__global__
void integrateK(float4* pos, Quaternion* quat, 
			float4* linVel, float4* angVel, 
			float dt, int size)
{
	int idx=blockIdx.x*blockDim.x+threadIdx.x;
	if( idx >= size ) return;

	linVel[idx] += make_float4(0.0f, -5.0f, 0.0f, 0.0f)*dt;
	Quaternion nextQuat = quat[idx];
	Quaternion angVelQ(angVel[idx]); angVelQ.w = 0.f;
	nextQuat += (angVelQ*nextQuat)*(dt*0.5f);
	nextQuat.normalizeQ();

	quat[idx] = nextQuat;
	pos[idx].x += linVel[idx].x*dt;
	pos[idx].y += linVel[idx].y*dt;
	pos[idx].z += linVel[idx].z*dt;
	if((idx/6)%2 == 0 && pos[idx].y <= 0.25) // Fuchang Liu Aug. 18 2010
			pos[idx].y = 0.25;
	else if ((idx/6)%2 == 1 && pos[idx].y <= 0.3)
		    pos[idx].y = 0.3;
}

__global__
void updateParticleDataK(float4* pos, Quaternion* quat, 
						 float4* linVel, float4* angVel, 
						 int* shapeIdxBuf, int* pBufStartOffsetBuf, int* numParticles, 
						 float4* particleRefPosBuf, float* particleRefMassBuf, 
						 float4* particlePosOut, float4* particleVelOut, float* particleMassOut, int size)
{
	int rbIdx=blockIdx.x*blockDim.x+threadIdx.x;
	if( rbIdx >= size ) return;

	int pStartOffset = pBufStartOffsetBuf[rbIdx];
	float4 rbPos = pos[rbIdx];
	float4 rbVel = linVel[rbIdx];
	Quaternion rbQuat = quat[rbIdx];
	float4 rbAngVel = angVel[rbIdx];
	for(int ip=0; ip<numParticles[rbIdx]; ip++)
	{
		int shapeIdx = shapeIdxBuf[rbIdx];
		float4 refPos = particleRefPosBuf[ ParticleRigidBody::getParticleRefIdx( shapeIdx, ip ) ];
		float4 currentPos;
		rbQuat.rotateVector( refPos, currentPos );

		particleVelOut[pStartOffset+ip] = rbVel + cross(rbAngVel, currentPos);

		currentPos += rbPos;
		particlePosOut[pStartOffset+ip] = make_float4(currentPos.x ,currentPos.y, currentPos.z, refPos.w);
		particleMassOut[pStartOffset+ip] = particleRefMassBuf[ParticleRigidBody::getParticleRefIdx( shapeIdx, ip )];
/*
	float4 centerVel	=	tex2D(velTex,rbTexCrd);
	float4 angularVel	=	tex2D(wVelTex,rbTexCrd);
	pVel.xyz	=	centerVel.xyz + cross(angularVel.xyz,rp.xyz);
*/
	}
}

__global__
void clearForceAndBoundaryConditionK(float4* pPos, float4* pVel, float* pMass, 
							   float4* pForceOut, 
							   float spMultip, float dpMultip, 
							   int numParticles, float dt)
{
	int idx=blockIdx.x*blockDim.x+threadIdx.x;
	if( idx >= numParticles ) return;
	float dt2Inv = 1.f/(dt*dt);
	float dtInv = 1.f/dt;

	float4 iPos = pPos[idx];
	float4 iVel = pVel[idx];
	float iMass = pMass[idx];
	float4 force = make_float4(0.f);

	{
		float dist = iPos.y;
		float4 r_ij = make_float4(0,1,0,0);
		float4 v_ij = -iVel;

		if( dist < iPos.w )
		{
			float springCoeff, dampingCoeff;
			calcSpringCoeff(1.f/iMass, 0.5f/iMass, dt2Inv, dtInv, spMultip, dpMultip, 
				springCoeff, dampingCoeff);

			float4 sForce = -springCoeff*(dist-iPos.w)*r_ij*0.1;// Fuchang Liu Aug. 18 2010
			float4 dForce = dampingCoeff*v_ij;
			force = sForce+dForce;
		}
	}

	pForceOut[idx] = force;
}

__global__
void comupteFoceOnParticlesK(float4* pPos, float4* pVel, 
							 int2* pairs, int* pairEntries, int numPairEntries, 
							 float* particleMass,
							 float spMultip, float dpMultip, 
							 float4* pForceOut, float dt)
{
	int idx=blockIdx.x*blockDim.x+threadIdx.x;
	if( idx >= numPairEntries ) return;

	int pairIdx = pairEntries[idx];
	int2 iPair = pairs[pairIdx];
	int iParticleIdx = iPair.x;
	float4 iPos = pPos[iParticleIdx];
	float4 iVel = pVel[iParticleIdx];
	float iMass = particleMass[iParticleIdx];
	float4 sForce = make_float4(0.f);
	float4 dForce = make_float4(0.f);
	float dt2Inv = 1.f/(dt*dt);
	float dtInv = 1.f/dt;

	do
	{
		int jParticleIdx = iPair.y;

		float4 jPos = pPos[jParticleIdx];
		float4 jVel = pVel[jParticleIdx];
		float jMass = particleMass[jParticleIdx];

		float4 r_ij = jPos - iPos; r_ij.w = 0.f;
		float4 v_ij = jVel - iVel;
		float dist = length(r_ij);
		float origDist = iPos.w + jPos.w;
		float springCoeff, dampingCoeff;
//		float m = iMass*jMass/(iMass+jMass);
		calcSpringCoeff(1.f/iMass, 1.f/jMass, dt2Inv, dtInv, spMultip, dpMultip, 
			springCoeff, dampingCoeff);

		float penetration = origDist-dist;

		if( penetration > 0.f )
		{
			/*if(dist > 0.000001 && dist < 0.01)    
			{
				sForce -= springCoeff*penetration*r_ij*100;
				dForce += dampingCoeff*v_ij;
			}
			else if(dist >= 0.01 && dist < 0.1)    
			{
				sForce -= springCoeff*penetration*r_ij*10;
				dForce += dampingCoeff*v_ij;
			}
			else*/
			{
				sForce -= springCoeff*penetration*r_ij/dist;
				dForce += dampingCoeff*v_ij;
			}
		}

		if( pairIdx++ == numPairEntries ) break;
		iPair = pairs[pairIdx];
	}
	while( iPair.x == iParticleIdx );

//	force.xyz += springCoeff*springForce + dampingCoeff*dampingForce
	pForceOut[iParticleIdx] += sForce + dForce;
}

//	float springCoeff=mass/(dt*dt)/2;
//	float dampingCoeff=mass/dt/2*(1-e);

template<bool CAP_VEL>
__global__
void updateVelocityK(float4* linVel, float4* angVel, 
					 Quaternion* quat, float* invMass, Matrix3x3* invInertia, 
					 int* shapeIdxBuf, int* pBufStartOffsetBuf, int* numParticles, 
					 float4* particleRefPosBuf, 
					 float4* particleForceBuf, int numBodies, float dt,
					 float maxLinVel)
{
	int rbIdx=blockIdx.x*blockDim.x+threadIdx.x;
	if( rbIdx >= numBodies ) return;

	Quaternion rbQuat = quat[rbIdx];
	int pStartOffset = pBufStartOffsetBuf[rbIdx];
	Matrix3x3 rbInvInertia = invInertia[rbIdx];

	float4 linForce = make_float4(0.f);
	float4 angForce = make_float4(0.f);

	for(int ip=0; ip<numParticles[rbIdx]; ip++)
	{
		int pIdx = pStartOffset+ip;

		int shapeIdx = shapeIdxBuf[rbIdx];
		float4 refPos = particleRefPosBuf[ ParticleRigidBody::getParticleRefIdx( shapeIdx, ip ) ];
		float4 currentPos;
		rbQuat.rotateVector( refPos, currentPos );

		float4 iForce = particleForceBuf[pIdx];
		linForce += iForce;
		angForce += cross(currentPos, iForce);
	}
	linVel[rbIdx] += linForce*invMass[rbIdx]*dt;

	if(CAP_VEL)
	{
		float4 lv = linVel[rbIdx]; lv.w = 0;
		float lvLength = length(lv);
		if( lvLength > maxLinVel )
		{
			lv = normalize(lv);
			linVel[rbIdx] = maxLinVel*lv;
		}
	}

	//	todo. cap velocity here

	float4 dw = rbInvInertia*angForce;
	angVel[rbIdx] += angForce*dt;
}



