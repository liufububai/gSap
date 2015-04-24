/**
*
*	@author Takahiro HARADA
*
*/
#ifndef PARTICLE_RIGID_BODY
#define PARTICLE_RIGID_BODY

#ifdef _WIN32
#ifdef PARTICLERIGIDBODY_EXPORTS
#define DLL_API_PARTICLERIGIDBODY __declspec(dllexport)
#else
#define DLL_API_PARTICLERIGIDBODY __declspec(dllimport)
#endif
#pragma comment(lib,"C:/CUDA/lib/cudart.lib")
#else
#define DLL_API_PARTICLERIGIDBODY
#endif

#include <assert.h>
#include <GL/glew.h>
#if defined(__APPLE__) || defined(MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <cuda.h>
#include <cuda_runtime.h>
#include <vector_types.h>
#include <vector_functions.h>

#include <cutil.h>
#pragma  comment(lib,"glew32.lib")

#include "Matrix3x3.h"


#define MAX_NUM_PARTICLES 10
#define MAX_PARTICLE_SHAPE_TYPE 4


struct Quaternion;


class
DLL_API_PARTICLERIGIDBODY
ParticleRigidBody
{
	public:

		struct 
		DLL_API_PARTICLERIGIDBODY
		RigidBodyData
		{
			int m_nBodies;
			float4* m_pos;
			Quaternion* m_quat;
			float4* m_linVel;
			float4* m_angVel;
			int* m_shapeIdx;
			int* m_pBufferStartOffset;
			int* m_numParticles;
			float* m_invMass;
			Matrix3x3* m_invInertia;
			int m_numTotalParticles;
		};

		//	dynamically updated
		struct
		DLL_API_PARTICLERIGIDBODY
		ParticleData
		{
			float4* m_pos; // w : radius
			float4* m_vel;
			float4* m_force;
			float* m_mass;
		};


		ParticleRigidBody(int maxNumBodies, int maxNumParticles, int maxPairs);
		~ParticleRigidBody();

		void setRbData(RigidBodyData& rbData);
		void setParticleData(float4* pos, float* mass, int numElements);
		void setParticleShapeInvInertia(Matrix3x3* invInertia, int numShapes);
		void setParams(float spMultip, float dpMultip) { m_springMultip = spMultip; m_dampingMultip = dpMultip; }
		void setMaxVelocity(float minRadius, float dt ) { m_maxLinVelocity = minRadius/dt; }

		void updateParticleData();
		//	broadphase
		void prepare();
		void comupteFoceOnParticles(float dt);
		void updateVelocity(float dt);
		void integrate(float dt);

		//	getter
		//	bp
		int getNumPairs() { return m_numPairs; }
		void setNumPairs(int numPairs) { assert(numPairs < m_maxPairs); m_numPairs = numPairs; } 
		int2* getPairs() { return m_pairs; }
		int* getPairEntries() { return m_pairEntries; }
		int getNumPairEntries() { return m_numPairEntries; }
		void setNumPairEntries(int numPairEntries) { m_numPairEntries = numPairEntries; }

		//	rb
		float4* getRbPos() { return m_rbData.m_pos; }
		Quaternion* getRbQuat() { return m_rbData.m_quat; }
		int* getRbShapeIdx() { return m_rbData.m_shapeIdx; }
		int* getRbNumParticles(){ return m_rbData.m_numParticles; }

		//	particles
		int getNumParticles() { return m_rbData.m_numTotalParticles; }
		float4* getParticlePos() { return m_pData.m_pos; }
		float4* getParticleVel() { return m_pData.m_vel; }
		float4* getParticleForce() { return m_pData.m_force; }

		int getNumBodies() { return m_rbData.m_nBodies; }
	

		//	others
		static void setGridBlockSize(dim3 &dimBlock,dim3 &dimGrid,int blockSizeX,int n)
		{
			dimBlock.x=blockSizeX;
			dimBlock.y = dimBlock.z = 1;
			dimGrid.x= (n/dimBlock.x) + (!(n%dimBlock.x)?0:1);
			dimGrid.y = dimGrid.z = 1;

			if(dimGrid.x==0)dimGrid.x=1;
		}
		static void setGridBlockSize(dim3 &dimBlock, dim3 &dimGrid,int n)
		{
			setGridBlockSize(dimBlock,dimGrid,64,n);
		}


		__device__ __host__
		static int getParticleRefIdx(int shapeIdx, int idx) { return MAX_NUM_PARTICLES*shapeIdx+idx; }

	public:
		RigidBodyData m_rbData;
		ParticleData m_pData;
		//	reference
		float4* m_particlePositions;	//	w: radius. static data. particle referencePosition
		float* m_particleMass;
		Matrix3x3* m_particleRefInvInertia;

		int2* m_pairs;
		int m_numPairs;

		int* m_pairEntries;	// for each sphere, where to start
		int m_numPairEntries;

		int m_maxNumBodies;
		int m_maxNumParticles;
		int m_maxPairs;

		float m_springMultip;
		float m_dampingMultip;
		float m_maxLinVelocity;
		GLuint pbo_position; //Fuchang Liu
		GLuint pbo_quater; //Fuchang Liu
		unsigned int texSize; //Fuchang Liu
};

#endif

