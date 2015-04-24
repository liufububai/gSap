/**
*
*	@author Takahiro HARADA
*
*/
#include "ParticleRigidBody.h"
#include "quaternion.h"
#include "Transform.h"
#include "transferUtils.h"

#include "ParticleRigidBodyWrapper.h"

#define VEL_MAX 1e+10

ParticleRigidBody::ParticleRigidBody(int maxNumBodies, int maxNumParticles, int maxPairs)
: m_maxNumBodies( maxNumBodies ),
m_maxNumParticles( maxNumParticles ),
m_maxPairs( maxPairs ),
m_numPairs( 0 ),
m_numPairEntries( 0 ),
m_maxLinVelocity( VEL_MAX  )
{
	cudaMalloc((void**)&m_rbData.m_pos, sizeof(float4)*m_maxNumBodies);
	cudaMalloc((void**)&m_rbData.m_quat, sizeof(float4)*m_maxNumBodies);
	cudaMalloc((void**)&m_rbData.m_linVel, sizeof(float4)*m_maxNumBodies);
	cudaMalloc((void**)&m_rbData.m_angVel, sizeof(float4)*m_maxNumBodies);
	cudaMalloc((void**)&m_rbData.m_shapeIdx, sizeof(int)*m_maxNumBodies);
	cudaMalloc((void**)&m_rbData.m_pBufferStartOffset, sizeof(int)*m_maxNumBodies);
	cudaMalloc((void**)&m_rbData.m_numParticles, sizeof(int)*m_maxNumBodies);
	cudaMalloc((void**)&m_rbData.m_invMass, sizeof(float)*m_maxNumBodies);
	cudaMalloc((void**)&m_rbData.m_invInertia, sizeof(Matrix3x3)*m_maxNumBodies);
	m_rbData.m_numTotalParticles = 0;

	cudaMalloc((void**)&m_pData.m_pos, sizeof(float4)*m_maxNumParticles);
	cudaMalloc((void**)&m_pData.m_vel, sizeof(float4)*m_maxNumParticles);
	cudaMalloc((void**)&m_pData.m_force, sizeof(float4)*m_maxNumParticles);	
	cudaMalloc((void**)&m_pData.m_mass, sizeof(float4)*m_maxNumParticles);

	cudaMalloc((void**)&m_particlePositions, sizeof(float4)*MAX_NUM_PARTICLES*MAX_PARTICLE_SHAPE_TYPE);
	cudaMalloc((void**)&m_particleMass, sizeof(float4)*MAX_NUM_PARTICLES*MAX_PARTICLE_SHAPE_TYPE);
	cudaMalloc((void**)&m_particleRefInvInertia, sizeof(Matrix3x3)*MAX_PARTICLE_SHAPE_TYPE);
	cudaMalloc((void**)&m_pairs, sizeof(int2)*m_maxPairs);
	cudaMalloc((void**)&m_pairEntries, sizeof(int)*m_maxNumBodies);

	m_springMultip = 1.f;
	m_dampingMultip = 0.5f;
}

ParticleRigidBody::~ParticleRigidBody()
{
	cudaFree(m_rbData.m_pos);
	cudaFree(m_rbData.m_quat);
	cudaFree(m_rbData.m_linVel);
	cudaFree(m_rbData.m_angVel);
	cudaFree(m_rbData.m_shapeIdx);
	cudaFree(m_rbData.m_pBufferStartOffset);
	cudaFree(m_rbData.m_numParticles);
	cudaFree(m_rbData.m_invMass);
	cudaFree(m_rbData.m_invInertia);

	cudaFree(m_pData.m_pos);
	cudaFree(m_pData.m_vel);
	cudaFree(m_pData.m_force);
	cudaFree(m_pData.m_mass);

	cudaFree(m_particlePositions);
	cudaFree(m_particleMass);
	cudaFree( m_particleRefInvInertia );
	cudaFree(m_pairs);
	cudaFree(m_pairEntries);
}

void ParticleRigidBody::setRbData(RigidBodyData& rbData)
{
	const int nBodies = m_rbData.m_nBodies = rbData.m_nBodies;
	m_rbData.m_numTotalParticles = rbData.m_numTotalParticles;
	for(int i=0; i<nBodies; i++)
	{
		rbData.m_quat[i].normalizeQ();
		assert(rbData.m_numParticles[i] < MAX_NUM_PARTICLES);
	}
	TransferUtils::copyHostToDevice(m_rbData.m_pos, rbData.m_pos, sizeof(float4)*nBodies);
	TransferUtils::copyHostToDevice(m_rbData.m_quat, rbData.m_quat, sizeof(float4)*nBodies);
	TransferUtils::copyHostToDevice(m_rbData.m_linVel, rbData.m_linVel, sizeof(float4)*nBodies);
	TransferUtils::copyHostToDevice(m_rbData.m_angVel, rbData.m_angVel, sizeof(float4)*nBodies);
	TransferUtils::copyHostToDevice(m_rbData.m_shapeIdx, rbData.m_shapeIdx, sizeof(int)*nBodies);
	TransferUtils::copyHostToDevice(m_rbData.m_pBufferStartOffset, rbData.m_pBufferStartOffset, sizeof(int)*nBodies);
	TransferUtils::copyHostToDevice(m_rbData.m_numParticles, rbData.m_numParticles, sizeof(int)*nBodies);
	TransferUtils::copyHostToDevice(m_rbData.m_invMass, rbData.m_invMass, sizeof(float)*nBodies);
	TransferUtils::copyHostToDevice(m_rbData.m_invInertia, rbData.m_invInertia, sizeof(Matrix3x3)*nBodies);
}

void ParticleRigidBody::setParticleData(float4* pos, float* mass, int numElements)
{
	TransferUtils::copyHostToDevice(m_particlePositions, pos, sizeof(float4)*numElements);
	TransferUtils::copyHostToDevice(m_particleMass, mass, sizeof(float4)*numElements);
}

void ParticleRigidBody::setParticleShapeInvInertia(Matrix3x3* invInertia, int numShapes)
{
	TransferUtils::copyHostToDevice( m_particleRefInvInertia, invInertia, sizeof(Matrix3x3)*numShapes );
}

void ParticleRigidBody::updateParticleData()
{
	int size = m_rbData.m_nBodies;

	dim3 dimBlock, dimGrid;
	setGridBlockSize( dimBlock, dimGrid, size );

	PRBWrap::upadteParticleData(dimGrid, dimBlock, 
		m_rbData.m_pos, m_rbData.m_quat, 
		m_rbData.m_linVel, m_rbData.m_angVel, 
		m_rbData.m_shapeIdx, m_rbData.m_pBufferStartOffset, m_rbData.m_numParticles, 
		m_particlePositions, m_particleMass, 
		m_pData.m_pos, m_pData.m_vel, m_pData.m_mass, size);
}

void ParticleRigidBody::prepare()
{
	int size = m_rbData.m_nBodies;
	dim3 dimBlock, dimGrid;
	setGridBlockSize( dimBlock, dimGrid, size );

	PRBWrap::prepare( dimGrid, dimBlock, m_rbData.m_quat, m_rbData.m_shapeIdx, m_particleRefInvInertia, 
		m_rbData.m_invInertia, size );
}

void ParticleRigidBody::comupteFoceOnParticles(float dt)
{
	//	clear force and calculate boundary force
	int size = m_rbData.m_numTotalParticles;
	dim3 dimBlock, dimGrid;
	setGridBlockSize( dimBlock, dimGrid, size );
	//	collide
	PRBWrap::clearForceAndBoundaryCondition( dimGrid, dimBlock,
		m_pData.m_pos, m_pData.m_vel, m_pData.m_mass, m_pData.m_force, m_springMultip, m_dampingMultip, 
		m_rbData.m_numTotalParticles, dt );


	size = m_numPairEntries;
	setGridBlockSize( dimBlock, dimGrid, size );

	PRBWrap::computeFoceOnParticles(dimGrid,dimBlock,
		m_pData.m_pos, m_pData.m_vel, m_pairs, m_pairEntries, size, 
		m_pData.m_mass, m_springMultip, m_dampingMultip, 
		m_pData.m_force, dt);
}

void ParticleRigidBody::updateVelocity(float dt)
{
	int size = m_rbData.m_nBodies;

	dim3 dimBlock, dimGrid;
	setGridBlockSize( dimBlock, dimGrid, size );

	PRBWrap::updateVelocity(dimGrid, dimBlock,
		m_rbData.m_linVel, m_rbData.m_angVel, m_rbData.m_quat, m_rbData.m_invMass, m_rbData.m_invInertia, 
		m_rbData.m_shapeIdx, m_rbData.m_pBufferStartOffset, m_rbData.m_numParticles,
		m_particlePositions, m_pData.m_force, size, dt, m_maxLinVelocity, m_maxLinVelocity != VEL_MAX);
}

void ParticleRigidBody::integrate(float dt)
{
	int size = m_rbData.m_nBodies;

	dim3 dimBlock, dimGrid;
	setGridBlockSize( dimBlock, dimGrid, size );

	PRBWrap::integrate( dimGrid, dimBlock,
		m_rbData.m_pos, m_rbData.m_quat, m_rbData.m_linVel, m_rbData.m_angVel, dt, size);
}



