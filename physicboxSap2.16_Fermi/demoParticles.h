#ifndef DEMO_PARTICLES_H
#define DEMO_PARTICLES_H

#include "ParticleRigidBody.h"
#include "quaternion.h"

#include <algorithm>

#define P_MAX_NUM_BODIES 5000
#define P_MAX_NUM_PARTICLES P_MAX_NUM_BODIES*10
#define P_MAX_COLLISION_PAIR	P_MAX_NUM_BODIES*10

#define USE_FLAT_BOX

const int numParticlesPerObj = 9;
#ifdef USE_FLAT_BOX
const int numParticlesPerObj1 = 4;
#else
const int numParticlesPerObj1 = 6;	//	torus
#endif


inline
void glDraw3DStrings(std::string str, const float4& pos)
{
	using namespace std;
	glRasterPos3f(pos.x, pos.y, pos.z);

		for(int i=0; i<(int)str.size(); i++)
			glutBitmapCharacter( GLUT_BITMAP_HELVETICA_12, str.c_str()[i] );
}

inline
void glDraw3DStrings(float value, const float4& pos)
{
	using namespace std;
	char valueChar[128];
	sprintf(valueChar, "%3.2f", value);
	std::string str = valueChar;

	glDraw3DStrings(str, pos);
}


class DemoParticles
{
	public:

		inline
		DemoParticles();

		inline
		~DemoParticles();

		inline
		void init();
		inline
		void step();
		inline
		void readback();
		inline
		void render();

		inline
		void cpuBroadphase();

	public:
		ParticleRigidBody* m_system;
		float4* m_pPos;
		bool m_debugDisplay;
		int m_counter;
		float m_pRadius;
};

DemoParticles::DemoParticles()
{
	m_system = 0;
	m_pPos = 0;
	m_debugDisplay = false;
	m_counter = 0;
}

DemoParticles::~DemoParticles()
{
	delete m_system;
	delete m_pPos;
}

void DemoParticles::init()
{
	const int numBodies = 16*4;//1024;//16*4;
	const int nRow = 4;
//	const int numBodies = 2048;//1024;//16*4;
//	const int nRow = 7;
	if( m_system ) delete m_system;
	ParticleRigidBody::RigidBodyData rbData;
	{	// allocation
		m_system = new ParticleRigidBody(P_MAX_NUM_BODIES, P_MAX_NUM_PARTICLES, P_MAX_COLLISION_PAIR);
		rbData.m_nBodies = numBodies;
		rbData.m_pos = new float4[numBodies];
		rbData.m_quat = new Quaternion[numBodies];
		rbData.m_linVel = new float4[numBodies];
		rbData.m_angVel = new float4[numBodies];
		rbData.m_shapeIdx = new int[numBodies];
		rbData.m_pBufferStartOffset = new int[numBodies];
		rbData.m_numParticles = new int[numBodies];
		rbData.m_invMass = new float[numBodies];
		rbData.m_invInertia = new Matrix3x3[numBodies];
	}
	float rbMass = 10.0f;
	float spacing = 1.8f;
	int numParticles = 0;
	//	initialize rigid body data (center of mass, quaternion, angular vel etc)
	for(int i=0; i<numBodies; i++)
	{
		rbData.m_pos[i].x = ((i%nRow)-nRow/2)*spacing;
		rbData.m_pos[i].z = (((i/nRow)%nRow)-nRow/2)*spacing;
		rbData.m_pos[i].y = ((i)/(nRow*nRow)+1)*spacing;
		rbData.m_quat[i] = make_float4(0,0,0,1);
		rbData.m_quat[i] = make_float4( getRand(-1,1),getRand(-1,1),getRand(-1,1),getRand(-1,1));
		rbData.m_linVel[i] = make_float4(0,0,0,0);
		rbData.m_angVel[i] = make_float4(0,0,0,0);
		rbData.m_angVel[i] = make_float4( getRand(-5,5),getRand(-5,5),getRand(-5,5),getRand(-5,5));

		rbData.m_pBufferStartOffset[i] = numParticles; // this is where particle data of this body starts in ParticleData
		if( i%2==0 )
//		if(0)
		{
			rbData.m_numParticles[i] = numParticlesPerObj; // number of particles per object
			numParticles += numParticlesPerObj;
			rbData.m_shapeIdx[i] = 0; // shape index
		}
		else
		{
			rbData.m_numParticles[i] = numParticlesPerObj1;
			numParticles += numParticlesPerObj1;
			rbData.m_shapeIdx[i] = 1;
		}
		rbData.m_invMass[i] = 1.f/rbMass;
		rbData.m_invInertia[i] = Matrix3x3(make_float4(1,0,0,0), make_float4(0,1,0,0), make_float4(0,0,1,0));
	}
	rbData.m_numTotalParticles = numParticles;

	//	set the data
	m_system->setRbData( rbData );

	{
		delete rbData.m_pos;
		delete rbData.m_quat;
		delete rbData.m_linVel;
		delete rbData.m_angVel;
		delete rbData.m_shapeIdx;
		delete rbData.m_pBufferStartOffset;
		delete rbData.m_numParticles;
		delete rbData.m_invMass;
		delete rbData.m_invInertia;
	}

	//	have to provide particle distribution for shapes
	m_pRadius = 0.25f;
	float pSpacing = m_pRadius*2*1.001f;
	float* pMass;
	{
		m_pPos = new float4[MAX_NUM_PARTICLES*2];
		pMass = new float[MAX_NUM_PARTICLES*2];
	}

	float smallRadius;
//	if( numParticlesPerObj == 9 )
	//	this is for a cube (9 particles)
	{
		float bigRadius = m_pRadius*2.f;
		smallRadius = (sqrtf(3)-1)/(sqrtf(3)+1)*bigRadius;
		float bigVolume = 4/3.f*PI*pow(bigRadius, 3.f);
		float smallVolume = 4/3.f*PI*pow(smallRadius, 3.f);
		float totalVolume = bigVolume+8*smallVolume;
		float bigMass = rbMass*bigVolume/totalVolume;
		float smallMass = rbMass*smallVolume/totalVolume;
		for(int i=0; i<2; i++) for(int j=0;j<2;j++)for(int k=0;k<2;k++)
		{
			int idx = i+j*2+k*2*2;
			float4 n = normalize(make_float4(i-0.5f,j-0.5f,k-0.5f,0));
			m_pPos[idx] = n*(bigRadius + smallRadius*1.01f);
			m_pPos[idx].w = smallRadius; // radius
			pMass[idx] = smallMass;
		}
		m_pPos[8] = make_float4(0, 0, 0, 0);
		m_pPos[8].w = bigRadius;
		pMass[8] = bigMass;
	}
	{
#ifdef USE_FLAT_BOX
		//	flat box (4 particles)
		for(int i=0; i<2; i++) for(int j=0;j<2;j++)
		{
			int idx = i+j*2;
			m_pPos[idx+MAX_NUM_PARTICLES] = make_float4(i-0.5f,j-0.5f,0.f,0)*pSpacing;
			m_pPos[idx+MAX_NUM_PARTICLES].w = m_pRadius; // radius
			pMass[idx+MAX_NUM_PARTICLES] = rbMass/numParticlesPerObj1;
		}
#else
		//	torus (6 particles)
		{
			m_pPos[MAX_NUM_PARTICLES+0] = make_float4(-1,0,0,0)*pSpacing;
			m_pPos[MAX_NUM_PARTICLES+0].w = m_pRadius;
			pMass[MAX_NUM_PARTICLES+0] = rbMass/numParticlesPerObj1;

			m_pPos[MAX_NUM_PARTICLES+1] = make_float4(-1.f/2,sqrtf(3)/2,0,0)*pSpacing;
			m_pPos[MAX_NUM_PARTICLES+1].w = m_pRadius;
			pMass[MAX_NUM_PARTICLES+1] = rbMass/numParticlesPerObj1;

			m_pPos[MAX_NUM_PARTICLES+2] = make_float4(1.f/2,sqrtf(3)/2,0,0)*pSpacing;
			m_pPos[MAX_NUM_PARTICLES+2].w = m_pRadius;
			pMass[MAX_NUM_PARTICLES+2] = rbMass/numParticlesPerObj1;

			m_pPos[MAX_NUM_PARTICLES+3] = make_float4(1,0,0,0)*pSpacing;
			m_pPos[MAX_NUM_PARTICLES+3].w = m_pRadius;
			pMass[MAX_NUM_PARTICLES+3] = rbMass/numParticlesPerObj1;

			m_pPos[MAX_NUM_PARTICLES+4] = make_float4(1.f/2,-sqrtf(3)/2,0,0)*pSpacing;
			m_pPos[MAX_NUM_PARTICLES+4].w = m_pRadius;
			pMass[MAX_NUM_PARTICLES+4] = rbMass/numParticlesPerObj1;

			m_pPos[MAX_NUM_PARTICLES+5] = make_float4(-1.f/2,-sqrtf(3)/2,0,0)*pSpacing;
			m_pPos[MAX_NUM_PARTICLES+5].w = m_pRadius;
			pMass[MAX_NUM_PARTICLES+5] = rbMass/numParticlesPerObj1;
		}
#endif
	}

	float inertiaElem = 1/12.f*pow(2*m_pRadius,2)*2*rbMass*0.2f;
	m_system->setParticleData(m_pPos, pMass, MAX_NUM_PARTICLES*2);
	Matrix3x3 invInertia(make_float4(inertiaElem,0,0,0), make_float4(0,inertiaElem,0,0), make_float4(0,0,inertiaElem,0));
	invInertia.invert();
	Matrix3x3 ii[2] = {invInertia, invInertia};
	//	also have to set invInertia( for 2 shapes )
	m_system->setParticleShapeInvInertia(ii, 2);

	delete pMass;

	float dt = 0.013f;
	//	for the second param, smaller value is more unstable than bigger value
	m_system->setParams(1.4f, 0.5f);
	m_system->setMaxVelocity( smallRadius, 0.013f/2 );
	//	calculate particle position for rendering
	m_system->updateParticleData();
	m_system->comupteFoceOnParticles( dt );
}

inline
float getTimeAndResetTimer(unsigned int timer)
{
	float t;
	cudaThreadSynchronize();
	cutStopTimer(timer);
	t=cutGetTimerValue(timer);
	cutResetTimer(timer);
	cutStartTimer(timer);
	return t;
}

void DemoParticles::step()
{
	int nIter = 2;
	float dt = 0.013f/nIter;

	unsigned int timer=0;
	float t[6] = {0,0,0,0,0,0};
	cutCreateTimer(&timer);
	for(int i=0; i<nIter; i++)
	{
	getTimeAndResetTimer( timer );
	m_system->integrate( dt );
	t[0] = getTimeAndResetTimer( timer );
	m_system->updateParticleData();
	t[1] = getTimeAndResetTimer( timer );

	cpuBroadphase();
	t[2] = getTimeAndResetTimer( timer );

	m_system->prepare();
	t[3] = getTimeAndResetTimer( timer );
	m_system->comupteFoceOnParticles( dt );
	t[4] = getTimeAndResetTimer( timer );
	m_system->updateVelocity( dt );
	t[5] = getTimeAndResetTimer( timer );
	}

	printf("%d: %3.2f, %3.2f, %3.2f, %3.2f, %3.2f, %3.2f\n", m_counter, t[0], t[1], t[2], t[3], t[4], t[5]);

	cutDeleteTimer(timer);

	m_counter++;
	{
		char txt[128];
		sprintf(txt,"Demo: Rigid Body System: %d", m_counter);
		glutSetWindowTitle(txt);
	}
}

void DemoParticles::readback()
{

}

void DemoParticles::render()
{
	const int nBodies = m_system->getNumBodies();
	float4* pos = new float4[nBodies];
	TransferUtils::copyDeviceToHost( pos, m_system->getRbPos(), sizeof(float4)*nBodies);
	Quaternion* quat = new Quaternion[nBodies];
	TransferUtils::copyDeviceToHost( quat, m_system->getRbQuat(), sizeof(Quaternion)*nBodies);


	if( !m_debugDisplay )
	{
		if( numParticlesPerObj == 9 )
		{
			int3 tris[12];
			float4 vtx[2][8];
			createBox( m_pRadius*2, m_pRadius*2, m_pRadius*2, tris, vtx[0] );
			createBox( m_pRadius*2, m_pRadius*2, m_pRadius, tris, vtx[1] );

			int* numParticlesPerObjCpy;
			TransferUtils::allocateAndCpyD2H( (void*&)numParticlesPerObjCpy, m_system->getRbNumParticles(), sizeof(int)*m_system->getNumParticles());
			glEnable(GL_NORMALIZE);
			glEnable(GL_LIGHTING);
			glEnable(GL_LIGHT0);

			for(int i=0; i<nBodies; i++)
			{
				float4 localVtx[8];
				int shapeIdx = (numParticlesPerObjCpy[i] == 9)? 0:1;
				for(int j=0; j<8; j++)
				{
					quat[i].rotateVector(vtx[shapeIdx][j],localVtx[j]);
					localVtx[j] += pos[i];
				}
				int tIdx = i%s_tableSize;
				float cs = 0.6f;
				glColor3f(colorTable[tIdx][0]+cs, colorTable[tIdx][1]+cs, colorTable[tIdx][2]+cs);
				glBegin(GL_TRIANGLES);
				for(int j=0; j<12; j++)
				{
					float4 x = localVtx[tris[j].x];
					float4 y = localVtx[tris[j].y];
					float4 z = localVtx[tris[j].z];
					float4 xy = y-x;
					float4 xz = z-x;
					float4 n =make_float4( cross(make_float3(xy), make_float3(xz)), 0);
					n = normalize(n);
					glNormal3f(n.x, n.y, n.z);
					glVertex3f(x.x, x.y, x.z);
					glVertex3f(y.x, y.y, y.z);
					glVertex3f(z.x, z.y, z.z);
				}
				glEnd();
				glColor3f(1,1,1);
			}
			glDisable(GL_LIGHTING);
			glDisable(GL_LIGHT0);
			delete numParticlesPerObjCpy;
		}
		{	//	floor
			glEnable(GL_NORMALIZE);
			glEnable(GL_LIGHTING);
			glEnable(GL_LIGHT0);

			float s = 60.f;
			glBegin(GL_QUADS);
			glNormal3f(0,1,0);
			glVertex3f(-s,0,-s);
			glVertex3f(s,0,-s);
			glVertex3f(s,0,s);
			glVertex3f(-s,0,s);			
			glEnd();
			
			glDisable(GL_LIGHTING);
			glDisable(GL_LIGHT0);
		}
	}

	if( m_debugDisplay )
	{
		int* shapeIdx = new int[nBodies];
		TransferUtils::copyDeviceToHost( shapeIdx, m_system->getRbShapeIdx(), sizeof(int)*nBodies);
		int* numParticles = new int[nBodies];
		TransferUtils::copyDeviceToHost( numParticles, m_system->getRbNumParticles(), sizeof(int)*nBodies);

		float4* particlePos = new float4[m_system->getNumParticles()];
		TransferUtils::copyDeviceToHost( particlePos, m_system->getParticlePos(), sizeof(float4)*m_system->getNumParticles() );
		float4* particleVel = new float4[m_system->getNumParticles()];
		TransferUtils::copyDeviceToHost( particleVel, m_system->getParticleVel(), sizeof(float4)*m_system->getNumParticles() );

		{	//	gpu ver of particle rendering
			int* numParticlesPerObjCpy;
			TransferUtils::allocateAndCpyD2H( (void*&)numParticlesPerObjCpy, m_system->getRbNumParticles(), sizeof(int)*m_system->getNumParticles());
			for(int ir=0; ir<2; ir++)
			{
				int pIdx = 0;
				for(int ib=0; ib<nBodies; ib++)
				{
					glColor3f(0,1,0);
					glPointSize(4);
					glLineWidth(2.f);
					if( ir==0 )
						glBegin(GL_LINE_LOOP);
					else
						glBegin(GL_POINTS);
					for(int ip=0;ip<numParticlesPerObjCpy[ib]; ip++)
					{
						int idx = pIdx;
						float4 p = particlePos[idx];
						glVertex3f( p.x, p.y, p.z );
						pIdx++;
					}
					glEnd();
					glLineWidth(1.f);
				}
			}
			delete numParticlesPerObjCpy;
		}

		{	//	broadphase pair
			int numPairs = m_system->getNumPairs();
			int2* pairs = new int2[numPairs];
			TransferUtils::copyDeviceToHost( pairs, m_system->getPairs(), sizeof(int2)*numPairs );

			glColor3f(1,0,0);
			glBegin(GL_LINES);
			for(int i=0; i<numPairs; i++)
			{
				int aIdx = pairs[i].x;
				int bIdx = pairs[i].y;
				float4 p = particlePos[aIdx];
				glVertex3f( p.x, p.y, p.z );
				float4 p1 = particlePos[bIdx];
				glVertex3f( p1.x, p1.y, p1.z );
				float4 rel = p-p1; rel.w = 0.f;
				float dist = length(rel);
				int a=0;
				a++;
			}
			glEnd();

			delete pairs;
		}

		{	//	show force
			int numParticles = m_system->getNumParticles();
			float4* force = new float4[numParticles];
			TransferUtils::copyDeviceToHost( force, m_system->getParticleForce(), sizeof(float4)*numParticles);

			glColor3f(1,1,1);
			glBegin(GL_LINES);
			for(int i=0; i<numParticles; i++)
			{
				float4 p = particlePos[i];
				float4 f = force[i];
				glVertex3f(p.x, p.y, p.z);
				p+=f*0.013f*0.1f;
				glVertex3f(p.x, p.y, p.z);
			}
			glEnd();

			delete force;
		}
		delete shapeIdx;
		delete numParticles;
		delete particlePos;
		delete particleVel;
	}

	delete pos;
	delete quat;
}

bool operator<(const int2& left, const int2& right)
{
  return left.x < right.x ;
}

//	pairs have to be sorted
void DemoParticles::cpuBroadphase()
{
	int nParticles = m_system->getNumParticles();
	float4* particlePos = new float4[nParticles];
	TransferUtils::copyDeviceToHost( particlePos, m_system->getParticlePos(), sizeof(float4)*nParticles );
	int2* pairs = new int2[P_MAX_COLLISION_PAIR];
	int* pairEntries = new int[nParticles];

	int numPairs = 0;
	int numPairEntries = 0;
	for(int i=0; i<nParticles; i++)
	{
		float4 ip = particlePos[i];
		for(int j=i+1; j<nParticles; j++)
		{
			float4 jp = particlePos[j];
			float4 rel = ip-jp; rel.w = 0;
			float range = ip.w+jp.w;
			if( rel.x > range || rel.y > range || rel.z > range ) continue;

			float dist = length(rel);
			if( dist < range )
			{
				if( numPairs < P_MAX_COLLISION_PAIR )
					pairs[numPairs++] = make_int2(i,j);
				if( numPairs < P_MAX_COLLISION_PAIR )
					pairs[numPairs++] = make_int2(j,i);
			}
		}
	}

	std::sort( &pairs[0], &pairs[numPairs] );

	int prevIdx = -1;
	for(int i=0; i<numPairs; i++)
	{
		if( prevIdx != pairs[i].x )
		{
			prevIdx = pairs[i].x;
			pairEntries[numPairEntries++] = i;
		}
	}

	//	check pairEntries
	TransferUtils::copyHostToDevice( m_system->getPairs(), pairs, sizeof(int2)*numPairs );
	m_system->setNumPairs( numPairs );
	TransferUtils::copyHostToDevice( m_system->getPairEntries(), pairEntries, sizeof(int)*numPairEntries);
	m_system->setNumPairEntries( numPairEntries );

	delete particlePos;
	delete pairs;
	delete pairEntries;
}


#endif

