#ifndef DEMO_PARTICLES_H
#define DEMO_PARTICLES_H

#include "ParticleRigidBody.h"
#include "Sap.h"
#include "quaternion.h"
#include <cutil_inline.h>
#include <cuda_gl_interop.h>
#include <algorithm>
//for rendering Youngeun
#include "shaderRigidbody.h"
#include "FrameBufferObject.h"
#include "vertex.h"
#include <fstream>
#include <time.h> // Fu-chang Liu
//#include "shader.h"

//const char *vertexShader;
extern GLuint shadowmapid;
extern GLuint floorTex;
extern int useTextures;

#ifndef GL_ARB_timer_query
#define GL_TIME_ELAPSED 0x88BF
#define GL_TIMESTAMP 0x8E28
#endif 

#define P_MAX_NUM_BODIES 17000
#define P_MAX_NUM_PARTICLES P_MAX_NUM_BODIES*10
#define P_MAX_COLLISION_PAIR	P_MAX_NUM_BODIES*10


const int numParticlesPerObj = 6;

#define WITH_BINDLESS

#ifdef WITH_BINDLESS
GLuint64EXT Vbo_addr[3];
GLuint      Vbo_size[3];
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
		void renderShadow();

		inline
		void cpuBroadphase();

		void getWorldExtent(float4& maxE, float4& minE);

		void snapShot(void);

		void _initGL();

		char* textFileRead(const char *filename);

		void setshader(const char* vertfile, const char* fragfile);

		void setshaderFloor(const char* vertfile, const char* fragfile);

	public:
		ParticleRigidBody* m_system;
		float4* m_pPos;
		bool m_debugDisplay;
		int m_counter;
		float m_pRadius;
		Sap* m_sap;
		std::ofstream fout;
		bool m_firstTime;	
		//GLuint torus_display_list;//Displaylist
		GLuint torus_display_list1;
		GLuint torus_display_list2;
		GLuint floor_display_list;
		GLuint texfloor;
		FBO posRB;
		FBO quatRB;
		int currentBuffer;
		GLuint texture;
		GLuint texture1, texture2;
		ShaderRigidBody shader;
		int fbo, rigidBodySize;
		GLuint vertexVBO, indexVBO;
		GLuint program;
		GLuint shaderprogram;
		GLuint floorprogram;
		GLuint texLoc;
		
	private:
		//initialization
		//void initRigidBody();
		void memoryAllocationRB();
		void initPositionRB();
		void initQuaternionRB();
		void initOthers();
	
		inline
		void injectPositionRB(float4* pos, bool isGPU);
		inline
		void injectQuaternionRB(Quaternion* quat, bool isGPU);

		void drawQuad(float size, float dpeth);
		void setOrthoMatrix();
};

DemoParticles::DemoParticles():fout   ("2.txt")
{
	m_system = 0;
	m_pPos = 0;
	m_debugDisplay = false;
	m_counter = 0;
	m_sap = 0;
	fbo = 256; //Youngeun
	rigidBodySize = 64;
	program = 0;
}

DemoParticles::~DemoParticles()
{
	delete m_system;
	delete m_pPos;
	delete m_sap;
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

void DemoParticles::init()
{
	//shader.init();
	memoryAllocationRB();
	initPositionRB();
	initQuaternionRB();
	//initRigidBody();
	initOthers();

	const int numBodies = 1024*4;//1024*4;//32*8*4;//1024;//16*4;
	const int nRow = 16;//8;
	if( m_system ) delete m_system;
	ParticleRigidBody::RigidBodyData rbData;
	{
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
	float spacing = 3.6f;
	rbData.m_numTotalParticles = numBodies*numParticlesPerObj;
	/*int i = 0;
	int nRow = 24;
	for(i=0; i<1152*10; i++)
	{
		rbData.m_pos[i].x = ((i%nRow)-nRow/2)*spacing;
		rbData.m_pos[i].z = (((i/nRow)%nRow)-nRow/2)*spacing;
		rbData.m_pos[i].y = ((i)/(nRow*nRow)+1)*spacing;
		rbData.m_quat[i] = make_float4(0,0,0,1);
		rbData.m_quat[i] = make_float4( getRand(-1,1),getRand(-1,1),getRand(-1,1),getRand(-1,1));
		rbData.m_linVel[i] = make_float4(0,0,0,0);
		rbData.m_angVel[i] = make_float4(0,0,0,0);
		rbData.m_angVel[i] = make_float4( getRand(-5,5),getRand(-5,5),getRand(-5,5),getRand(-5,5));

		//rbData.m_shapeIdx[i] = 0;
		rbData.m_pBufferStartOffset[i] = numParticlesPerObj*i;
		rbData.m_numParticles[i] = numParticlesPerObj;
		rbData.m_invMass[i] = 1.f/rbMass;
		rbData.m_invInertia[i] = Matrix3x3(make_float4(1,0,0,0), make_float4(0,1,0,0), make_float4(0,0,1,0));

		//rbData.m_pBufferStartOffset[i] = numParticles;
		if( i%2==0 )
		{
			//rbData.m_numParticles[i] = numParticlesPerObj;
			//numParticles += numParticlesPerObj;
			rbData.m_shapeIdx[i] = 0;
		}
		else
		{
			//rbData.m_numParticles[i] = numParticlesPerObj1;
			//numParticles += numParticlesPerObj1;
			rbData.m_shapeIdx[i] = 1;
		}
	}
	nRow = 24;*/
	for(int i=0; i<numBodies; i++)
	{
		rbData.m_pos[i].x = ((i%nRow)-nRow/2)*spacing;
		rbData.m_pos[i].z = (((i/nRow)%nRow)-nRow/2)*spacing;
		rbData.m_pos[i].y = ((i)/(nRow*nRow)+1)*spacing;// + ((i)/(nRow*nRow))*((i)/(nRow*nRow))/2.0;
		rbData.m_quat[i] = make_float4(0,0,0,1);
		rbData.m_quat[i] = make_float4( getRand(-1,1),getRand(-1,1),getRand(-1,1),getRand(-1,1));
		rbData.m_linVel[i] = make_float4(0,0,0,0);
		rbData.m_angVel[i] = make_float4(0,0,0,0);
		rbData.m_angVel[i] = make_float4( getRand(-5,5),getRand(-5,5),getRand(-5,5),getRand(-5,5));

		//rbData.m_shapeIdx[i] = 0;
		rbData.m_pBufferStartOffset[i] = numParticlesPerObj*i;
		rbData.m_numParticles[i] = numParticlesPerObj;
		rbData.m_invMass[i] = 1.f/rbMass;
		rbData.m_invInertia[i] = Matrix3x3(make_float4(1,0,0,0), make_float4(0,1,0,0), make_float4(0,0,1,0));

		//rbData.m_pBufferStartOffset[i] = numParticles;
		if( i%2==0 )
		{
			//rbData.m_numParticles[i] = numParticlesPerObj;
			//numParticles += numParticlesPerObj;
			rbData.m_shapeIdx[i] = 0;
		}
		else
		{
			//rbData.m_numParticles[i] = numParticlesPerObj1;
			//numParticles += numParticlesPerObj1;
			rbData.m_shapeIdx[i] = 1;
		}
		//rbData.m_invMass[i] = 1.f/rbMass;
		//rbData.m_invInertia[i] = Matrix3x3(make_float4(1,0,0,0), make_float4(0,1,0,0), make_float4(0,0,1,0));
		//rbData.m_numTotalParticles = numParticles;
	}

	m_system->setRbData( rbData );
	injectPositionRB(rbData.m_pos, false);	//Youngeun
	injectQuaternionRB(rbData.m_quat, false); //Youngeun
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

	m_pRadius = 0.25f;
	float smallRadius = m_pRadius;
	float pSpacing = m_pRadius*2*1.001f;
	float* pMass;
	{
		m_pPos = new float4[MAX_NUM_PARTICLES*2];
		pMass = new float[MAX_NUM_PARTICLES*2];
		//m_pPos = new float4[MAX_NUM_PARTICLES];
		//pMass = new float[MAX_NUM_PARTICLES];
	}

	if( numParticlesPerObj == 8 )
	{
		for(int i=0; i<2; i++) for(int j=0;j<2;j++)for(int k=0;k<2;k++)
		{
			int idx = i+j*2+k*2*2;
			m_pPos[idx] = make_float4(i-0.5f,j-0.5f,k-0.5f,0)*pSpacing;
			m_pPos[idx].w = m_pRadius; // radius
			pMass[idx] = rbMass/numParticlesPerObj;
		}
	}
	else if( numParticlesPerObj == 9 )
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
	else
	{
		/*for(int i=0; i<numParticlesPerObj; i++)
		{
			m_pPos[i] = make_float4(0.f);
			m_pPos[i].y = i*pSpacing - (numParticlesPerObj-1)*pSpacing/2;
			m_pPos[i].w = m_pRadius; // radius
			pMass[i] = rbMass/numParticlesPerObj;
		}*/
		m_pPos[0] = make_float4(-1,0,0,0)*pSpacing;
		m_pPos[0].w = m_pRadius;
		pMass[0] = rbMass/numParticlesPerObj;

		m_pPos[1] = make_float4(-1.f/2,sqrtf(3)/2,0,0)*pSpacing;
		m_pPos[1].w = m_pRadius;
		pMass[1] = rbMass/numParticlesPerObj;

		m_pPos[2] = make_float4(1.f/2,sqrtf(3)/2,0,0)*pSpacing;
		m_pPos[2].w = m_pRadius;
		pMass[2] = rbMass/numParticlesPerObj;

		m_pPos[3] = make_float4(1,0,0,0)*pSpacing;
		m_pPos[3].w = m_pRadius;
		pMass[3] = rbMass/numParticlesPerObj;

		m_pPos[4] = make_float4(1.f/2,-sqrtf(3)/2,0,0)*pSpacing;
		m_pPos[4].w = m_pRadius;
		pMass[4] = rbMass/numParticlesPerObj;

		m_pPos[5] = make_float4(-1.f/2,-sqrtf(3)/2,0,0)*pSpacing;
		m_pPos[5].w = m_pRadius;
		pMass[5] = rbMass/numParticlesPerObj;

		m_pPos[MAX_NUM_PARTICLES+0] = make_float4(-1,0,0,0)*pSpacing*1.2;
		m_pPos[MAX_NUM_PARTICLES+0].w = m_pRadius*1.2;
		pMass[MAX_NUM_PARTICLES+0] = rbMass/numParticlesPerObj;

		m_pPos[MAX_NUM_PARTICLES+1] = make_float4(-1.f/2,sqrtf(3)/2,0,0)*pSpacing*1.2;
		m_pPos[MAX_NUM_PARTICLES+1].w = m_pRadius*1.2;
		pMass[MAX_NUM_PARTICLES+1] = rbMass/numParticlesPerObj;

		m_pPos[MAX_NUM_PARTICLES+2] = make_float4(1.f/2,sqrtf(3)/2,0,0)*pSpacing*1.2;
		m_pPos[MAX_NUM_PARTICLES+2].w = m_pRadius*1.2;
		pMass[MAX_NUM_PARTICLES+2] = rbMass/numParticlesPerObj;

		m_pPos[MAX_NUM_PARTICLES+3] = make_float4(1,0,0,0)*pSpacing*1.2;
		m_pPos[MAX_NUM_PARTICLES+3].w = m_pRadius*1.2;
		pMass[MAX_NUM_PARTICLES+3] = rbMass/numParticlesPerObj;

		m_pPos[MAX_NUM_PARTICLES+4] = make_float4(1.f/2,-sqrtf(3)/2,0,0)*pSpacing*1.2;
		m_pPos[MAX_NUM_PARTICLES+4].w = m_pRadius*1.2;
		pMass[MAX_NUM_PARTICLES+4] = rbMass/numParticlesPerObj;

		m_pPos[MAX_NUM_PARTICLES+5] = make_float4(-1.f/2,-sqrtf(3)/2,0,0)*pSpacing*1.2;
		m_pPos[MAX_NUM_PARTICLES+5].w = m_pRadius*1.2;
		pMass[MAX_NUM_PARTICLES+5] = rbMass/numParticlesPerObj;
	}


	float inertiaElem = 1/12.f*pow(2*m_pRadius,2)*2*rbMass*0.2f;
	//m_system->setParticleData(m_pPos, pMass, numParticlesPerObj);
	m_system->setParticleData(m_pPos, pMass, MAX_NUM_PARTICLES*2);
	Matrix3x3 invInertia(make_float4(inertiaElem,0,0,0), make_float4(0,inertiaElem,0,0), make_float4(0,0,inertiaElem,0));
	invInertia.invert();
	Matrix3x3 invInertia1(make_float4(2*inertiaElem,0,0,0), make_float4(0,2*inertiaElem,0,0), make_float4(0,0,2*inertiaElem,0));
	invInertia1.invert();
	Matrix3x3 ii[2] = {invInertia, invInertia1};
	//m_system->setParticleShapeInvInertia(&invInertia, 1);
	m_system->setParticleShapeInvInertia(ii, 2);

	delete pMass;

	//initSAP(numBodies*numParticlesPerObj);
	m_sap = new Sap(numBodies*numParticlesPerObj);
	m_firstTime = true;
	float dt = 0.013f*2;
	m_system->setParams(2.1f, 0.8f);//1.4,0.8
	m_system->setMaxVelocity( smallRadius, 0.01f);//0.013 
	m_system->updateParticleData();
	m_system->comupteFoceOnParticles( dt); 
//	torus_display_list = glGenLists(1);
//	glNewList(torus_display_list,GL_COMPILE);
//	glutSolidTorus(0.255, 0.725, 10, 15);
//	glEndList(); 
}

void DemoParticles::memoryAllocationRB()
{
	posRB.init(rigidBodySize,rigidBodySize);
	posRB.createTexture(0,GL_RGBA32F_ARB, GL_TEXTURE_2D,GL_RGBA,GL_FLOAT,GL_NEAREST,false);
	posRB.createTexture(1,GL_RGBA32F_ARB, GL_TEXTURE_2D,GL_RGBA,GL_FLOAT,GL_NEAREST,false);
	
	quatRB.init(rigidBodySize,rigidBodySize);
	quatRB.createTexture(0,GL_RGBA32F_ARB, GL_TEXTURE_2D,GL_RGBA,GL_FLOAT,GL_NEAREST,false);
	quatRB.createTexture(1,GL_RGBA32F_ARB, GL_TEXTURE_2D,GL_RGBA,GL_FLOAT,GL_NEAREST,false);
}

void DemoParticles::initPositionRB()
{
	float *tmp=new float[rigidBodySize*rigidBodySize*4];
	for(int i=0;i<rigidBodySize*rigidBodySize;i++){
		tmp[i*4+0]=-10;
		tmp[i*4+1]=-10;
		tmp[i*4+2]=-10;
		tmp[i*4+3]=-10;
	}

	glGenTextures(1,&texture1);
	glBindTexture(GL_TEXTURE_2D,texture1);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F_ARB,rigidBodySize,rigidBodySize,0,GL_RGBA,GL_FLOAT,tmp);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);//_TO_EDGE_EXT);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);//_TO_EDGE_EXT);
	glBindTexture(GL_TEXTURE_2D,0);

	glGenTextures(1,&texture);
	glBindTexture(GL_TEXTURE_2D,texture);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F_ARB,rigidBodySize,rigidBodySize,0,GL_RGBA,GL_FLOAT,tmp);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);//_TO_EDGE_EXT);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);//_TO_EDGE_EXT);
	

	posRB.activate();
	posRB.draw(0);
	setOrthoMatrix();
	glClearColor(1,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,texture1);
	drawQuad(1,0);
	glDisable(GL_TEXTURE_2D);

	posRB.draw(1);
	setOrthoMatrix();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,texture1);
	drawQuad(1,0);
	glDisable(GL_TEXTURE_2D);

	posRB.inactivate();

	delete [] tmp;
}

void DemoParticles::initQuaternionRB()
{
	float *tmp=new float[rigidBodySize*rigidBodySize*4];

	for(int i=0;i<rigidBodySize*rigidBodySize;i++){
		tmp[i*4+0]=-10;
		tmp[i*4+1]=-10;
		tmp[i*4+2]=-10;
		tmp[i*4+3]=-10;
	}


	glGenTextures(1,&texture2);
	glBindTexture(GL_TEXTURE_2D,texture2);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F_ARB,rigidBodySize,rigidBodySize,0,GL_RGBA,GL_FLOAT,tmp);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);//_TO_EDGE_EXT);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);//_TO_EDGE_EXT);
	glBindTexture(GL_TEXTURE_2D,0);

	glBindTexture(GL_TEXTURE_2D,texture);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F_ARB,rigidBodySize,rigidBodySize,0,GL_RGBA,GL_FLOAT,tmp);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);//_TO_EDGE_EXT);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);//_TO_EDGE_EXT);
	

	quatRB.activate();
	quatRB.draw(0);
	setOrthoMatrix();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,texture2);
	drawQuad(1,0);
	glDisable(GL_TEXTURE_2D);

	quatRB.draw(1);
	setOrthoMatrix();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,texture2);
	drawQuad(1,0);
	glDisable(GL_TEXTURE_2D);

	quatRB.inactivate();


	delete [] tmp;
}

inline
void DemoParticles::injectPositionRB(float4* pos, bool isGPU)
{
	/*float bits[4];
	float d = 1/(float)rigidBodySize*2;
	int x,y;

	const int nBodies = m_system->getNumBodies();
	posRB.activate();
	posRB.draw(currentBuffer);
	setOrthoMatrix();

	glRasterPos2f(-1, -1);
	glDrawPixels(rigidBodySize, rigidBodySize, GL_RGBA, GL_FLOAT, pos);
	posRB.inactivate();*/

	const int nBodies = m_system->getNumBodies();

	if(!isGPU)
	{
		//Fuchang Liu
		//GLuint texid = 0;
		//glGenTextures(1, &texid);
		//glBufferData(GL_PIXEL_UNPACK_BUFFER, 4*m_system->texSize*m_system->texSize*sizeof(float), 0, GL_DYNAMIC_DRAW);  //Fuchang Liu
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER, m_system->pbo_position); //Fuchang Liu
		glBufferDataARB(GL_PIXEL_UNPACK_BUFFER, 4*m_system->texSize*m_system->texSize*sizeof(float), (float*)pos, GL_STREAM_DRAW_ARB);  //Fuchang Liu
		void* PBOpointer = glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY); // obtain pointer to pbo data
		memcpy(PBOpointer, (float*)pos, 4*m_system->texSize*m_system->texSize*sizeof(float)); // copy data from system memory to pbo
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB); 
		//glEnable(GL_TEXTURE_2D);
		//glGenTextures(1,&texture);
		glBindTexture(GL_TEXTURE_2D, texture1);

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_system->texSize, m_system->texSize, GL_RGBA, GL_FLOAT, 0);
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, m_system->texSize, m_system->texSize, 0, GL_RGBA, GL_FLOAT, 0);
		//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
		//glBindTexture(GL_TEXTURE_2D, 0);

		//GLuint fbo;
		//glGenFramebuffersEXT(1,&fbo);
		//glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fbo);
		//glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, texture, 0);
		//glBindTexture(GL_TEXTURE_2D, 0);
		//glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0); 
    }
	else
	{
		float* d_Pos;
		//glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_system->pbo_position); //Fuchang Liu
		cutilSafeCall(cudaGLMapBufferObject((void **)&d_Pos, m_system->pbo_position));
		//unsigned int timer=0;
		//float t;
		//cutCreateTimer(&timer);
		//getTimeAndResetTimer( timer );
		cutilSafeCall( cudaMemcpy(d_Pos, m_system->getRbPos() , sizeof(float4)*(nBodies), cudaMemcpyDeviceToDevice));
		//t = getTimeAndResetTimer( timer );
		//printf("CopyDevice:: %5.3f\n", t);//Fu-chang Liu
		cutilSafeCall(cudaGLUnmapBufferObject(m_system->pbo_position));

		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, m_system->pbo_position); //Fuchang Liu
		//glEnable(GL_TEXTURE_2D);
		//glGenTextures(1,&texture);
		glBindTexture(GL_TEXTURE_2D, texture1);

		//float* h_pos = new float[4*nBodies];
		//cutilSafeCall(cudaMemcpy(h_pos, m_system->getRbPos(), sizeof(float4)*(nBodies), cudaMemcpyDeviceToHost));
		/*for(uint i=0;i<nBodies;i++) // for validation
		 {
			 fout<<"i :"<<i<<'\n'
				<<h_pos[i*4]<<'\n'
				 <<h_pos[i*4+1]<<'\n'
				 <<h_pos[i*4+2]<<'\n'
				 <<h_pos[i*4+3]<<"\n\n";
		 }
		fout<<"----------------------------------------------------------------------------------------------------------------"<<"\n";*/
		//glBufferData(GL_PIXEL_UNPACK_BUFFER, 4*m_system->texSize*m_system->texSize*sizeof(float), h_pos, GL_STREAM_DRAW);  //Fuchang Liu

		//glBufferData(GL_PIXEL_UNPACK_BUFFER, 4*m_system->texSize*m_system->texSize*sizeof(float), 0, GL_DYNAMIC_DRAW);  //Fuchang Liu
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_system->texSize, m_system->texSize, GL_RGBA, GL_FLOAT, 0);
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, m_system->texSize, m_system->texSize, 0, GL_RGBA, GL_FLOAT, 0);
		//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
		//glBindTexture(GL_TEXTURE_2D, 0);
	}
}

inline
void DemoParticles::injectQuaternionRB(Quaternion *quat, bool isGPU)
{
	/*float bits[4];
	float d = 1/(float)rigidBodySize*2;
	int x,y;

	const int nBodies = m_system->getNumBodies();
	quatRB.activate();
	quatRB.draw(currentBuffer);
	setOrthoMatrix();

	glRasterPos2f(-1, -1);
	glDrawPixels(rigidBodySize, rigidBodySize, GL_RGBA, GL_FLOAT, quat);

	quatRB.inactivate();*/

	const int nBodies = m_system->getNumBodies();

	if(!isGPU)
	{
		//Fuchang Liu
		//GLuint texid = 1;
		//glGenTextures(1, &texid);
		//glBufferData(GL_PIXEL_UNPACK_BUFFER, 4*m_system->texSize*m_system->texSize*sizeof(float), 0, GL_DYNAMIC_DRAW);  //Fuchang Liu
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER, m_system->pbo_quater); //Fuchang Liu
		glBufferDataARB(GL_PIXEL_UNPACK_BUFFER, 4*m_system->texSize*m_system->texSize*sizeof(float), (float*)quat, GL_STREAM_DRAW_ARB);  //Fuchang Liu
		void* PBOpointer = glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY); // obtain pointer to pbo data
		memcpy(PBOpointer, (float*)quat, 4*m_system->texSize*m_system->texSize*sizeof(float)); // copy data from system memory to pbo
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB); 
		//glEnable(GL_TEXTURE_2D);
		//glGenTextures(1,&texture);
		glBindTexture(GL_TEXTURE_2D, texture2);

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_system->texSize, m_system->texSize, GL_RGBA, GL_FLOAT, 0);
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, m_system->texSize, m_system->texSize, 0, GL_RGBA, GL_FLOAT, 0);
		//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
		//glBindTexture(GL_TEXTURE_2D, 0);
		//GLuint fbo;
		//glGenFramebuffersEXT(1,&fbo);
		//glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fbo);
		//glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, texture, 0);
		//glBindTexture(GL_TEXTURE_2D, 0);
		//glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
    }
	else
	{
		float* d_Quater;
		//glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_system->pbo_quater); //Fuchang Liu
		cutilSafeCall(cudaGLMapBufferObject((void **)&d_Quater, m_system->pbo_quater));
		//unsigned int timer=0;
		//float t;
		//cutCreateTimer(&timer);
		//getTimeAndResetTimer( timer );
		cutilSafeCall( cudaMemcpy(d_Quater, m_system->getRbQuat() , sizeof(float4)*(nBodies), cudaMemcpyDeviceToDevice));
		//t = getTimeAndResetTimer( timer );
		//printf("CopyDevice:: %5.3f\n", t);//Fu-chang Liu
		cutilSafeCall(cudaGLUnmapBufferObject(m_system->pbo_quater));

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_system->pbo_quater); //Fuchang Liu
		//glEnable(GL_TEXTURE_2D);
		//glGenTextures(1,&texture);
		glBindTexture(GL_TEXTURE_2D, texture2);

		//float* h_quater = new float[4*nBodies];
		//cutilSafeCall(cudaMemcpy(h_quater, m_system->getRbQuat(), sizeof(float4)*(nBodies), cudaMemcpyDeviceToHost));
		//glBufferData(GL_PIXEL_UNPACK_BUFFER, 4*m_system->texSize*m_system->texSize*sizeof(float), h_pos, GL_STREAM_DRAW);  //Fuchang Liu

		//glBufferData(GL_PIXEL_UNPACK_BUFFER, 4*m_system->texSize*m_system->texSize*sizeof(float), 0, GL_DYNAMIC_DRAW);  //Fuchang Liu
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_system->texSize, m_system->texSize, GL_RGBA, GL_FLOAT, 0);
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, m_system->texSize, m_system->texSize, 0, GL_RGBA, GL_FLOAT, 0);
		//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
		//glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void DemoParticles::initOthers()
{
	currentBuffer=0;

	int id0, id1,id2,id3, num;
	num = 1;//1024*16;
	unsigned int *face = new unsigned int[150*num*4];

	Vertex* v = new Vertex[150*num];

	int4* quads = new int4[150*num];
	float4* vtx = new float4[150*num];
	float v0[3],v1[3];
	float twopi = 2 * 3.14159;
	//createBox( m_pRadius*2, m_pRadius*2, m_pRadius*2, tris, vtx );
	createTorus(0.5, 0.25, quads, vtx);

	for(int i=0; i<150*num; i++)
	{
		face[i*4+0] = quads[i].x;
		face[i*4+1] = quads[i].y;
		face[i*4+2] = quads[i].z;
		face[i*4+3] = quads[i].w;
	}

	//float c = 1/sqrt(3.0f);
	for(int i=0; i<150*num; i++)
	{
		v[i].position[0] = vtx[i].x;
		v[i].position[1] = vtx[i].y;
		v[i].position[2] = vtx[i].z;

		//id0 = face[i*4+0];
		//id1 = face[i*4+1];
		//id2 = face[i*4+3];
		//id3 = face[i][3];

		float x = 0.5 * cos(i*twopi/15);
	    float y = 0.5 * sin(i*twopi/15);

		/*v0[0] = v[id1].position[0] - v[id0].position[0];
		v1[0] = v[id2].position[0] - v[id0].position[0];
		v0[1] = v[id1].position[1] - v[id0].position[1];
		v1[1] = v[id2].position[1] - v[id0].position[1];
		v0[2] = v[id1].position[2] - v[id0].position[2];
		v1[2] = v[id2].position[2] - v[id0].position[2];*/

		v[i].normal[2] = v[i].position[2];//v0[0]*v1[1] - v0[1]*v1[0];
		v[i].normal[0] = v[i].position[0] - x;//v0[1]*v1[2] - v0[2]*v1[1];
		v[i].normal[1] = v[i].position[1] - y;//v0[2]*v1[0] - v0[0]*v1[2];
	}

	delete []quads;
	delete []vtx;

	glGenBuffersARB(1, &vertexVBO);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vertexVBO);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,  150*num*sizeof(Vertex), &v[0].position[0], GL_STREAM_DRAW_ARB);

	glGenBuffersARB(1, &indexVBO);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, indexVBO);
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,  150*4*num*sizeof(int), &face[0], GL_STREAM_DRAW_ARB);

	//glGetBufferParameterui64vNV( GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_GPU_ADDRESS_NV, &Vbo_addr[i] );
    //glMakeBufferResidentNV     ( GL_ELEMENT_ARRAY_BUFFER, GL_READ_ONLY );

	delete []face;
	delete []v;

/*
	// the small torus
	torus_display_list1 = glGenLists(1);
	glNewList(torus_display_list1,GL_COMPILE);
	glutSolidTorus(0.25, 0.5, 10, 12);
	glEndList(); 

	// the big torus
	torus_display_list2 = glGenLists(1);
	glNewList(torus_display_list2,GL_COMPILE);
	glutSolidTorus(0.3, 0.6, 10, 12);//(0.510, 1.510, 10, 15);
	glEndList(); 
*/

	// the floor
	float x, y;
    float d = 1.0;
    float s = 120.0f;
	floor_display_list = glGenLists(1);
	glNewList(floor_display_list,GL_COMPILE);
	glColor3f(1.0, 1.0, 1.0);
			/*glBegin(GL_QUADS);
			glNormal3f(0,1,0);
			glVertex3f(-s,0,-s);
			glVertex3f(s,0,-s);
			glVertex3f(s,0,s);
			glVertex3f(-s,0,s);			
			glEnd();*/

		  /* Draw ground. */
		  /*if (useTextures) {
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D,floorTex);
		  }*/
		  //glColor3f(1,1,0);
		  glNormal3f(0,1,0);
		  glPushMatrix();
		  glTranslatef(0.0, 1.0, 0.0);
		  //glRotatef(rotate_x, 1.0, 0.0, 0.0);
		  //glRotatef(rotate_y, 0.0, 1.0, 0.0);

		  /* Tesselate floor so lighting looks reasonable. */
		  for (x=-s; x<s; x+=d) {
			glBegin(GL_QUAD_STRIP);
			for (y=-s; y<s; y+=d) {
			  glTexCoord2f((x+s)/s/2, (y+s)/s/2);
			  glVertex3f(x, -1, -y);
			  glTexCoord2f((x+1+s)/s/2, (y+s)/s/2);
			  glVertex3f(x+1, -1, -y);
			}
			glEnd();
		  }
		  glPopMatrix();

		  /*if (useTextures) {
			glBindTexture(GL_TEXTURE_2D,0);
			glDisable(GL_TEXTURE_2D);
		  }*/
	glEndList(); 

	_initGL();
}

void DemoParticles::setOrthoMatrix(){
	glClearColor(0.0, 0.0, 0.0, 1.0);	//Youngeun background color
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.,1.,-1.,1.,-100,100);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}
void DemoParticles::drawQuad(float size,float depth){
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);
	glVertex3f(-size,-size,depth);
	glTexCoord2f(0,1);
	glVertex3f(-size,size,depth);
	glTexCoord2f(1,1);
	glVertex3f(size,size,depth);
	glTexCoord2f(1,0);
	glVertex3f(size,-size,depth);
	glEnd();
}

void DemoParticles::step()
{
	float4 maxE, minE;
	if (m_firstTime)
	{
		getWorldExtent( maxE, minE );
		printf("WorldExtent==\n");
		printf("%3.2f, %3.2f, %3.2f\n",maxE.x, maxE.y, maxE.z);
		printf("%3.2f, %3.2f, %3.2f\n\n",minE.x, minE.y, minE.z);
	}
	int nIter = 1;
	float dt = 0.025;//0.013f*2/nIter;
    float t_Sim = 0.0;
	unsigned int timer=0;
	float t[6];
	cutCreateTimer(&timer);
	for(int i=0; i<nIter; i++)
	{
		getTimeAndResetTimer( timer );
		m_system->integrate( dt );
		t[0] = getTimeAndResetTimer( timer );
		m_system->updateParticleData();
		t[1] = getTimeAndResetTimer( timer );

		if(0)
		{
			cpuBroadphase();
		}
		else
		{
			//float spacing = 1.8f;
			int nPairs = 0;
			int nPEntries = 0;
			float interval;
			if (m_firstTime)
			{
				interval = max(maxE.x-minE.x, maxE.y-minE.y);
				interval = interval > maxE.z-minE.z ? interval : maxE.z-minE.z;
				m_firstTime = false;
			}
			else
				interval = 0;
			m_sap->collision(interval, m_system->getParticlePos(), m_system->getPairs(), m_system->getPairEntries(), &nPairs, &nPEntries);
			//m_sap->collisionWithSub(interval, m_system->getParticlePos(), m_system->getPairs(), m_system->getPairEntries(), &nPairs, &nPEntries);
			//printf("nPairs%-5d\n", nPairs);
			//printf("nPEntries%-5d\n", nPEntries);
			m_system->setNumPairs( nPairs );
			m_system->setNumPairEntries( nPEntries );
		}
		t[2] = getTimeAndResetTimer( timer );

		m_system->prepare();
		t[3] = getTimeAndResetTimer( timer );
		//m_system->comupteFoceOnParticles( dt );
		m_system->comupteFoceOnParticles( dt); 
		t[4] = getTimeAndResetTimer( timer );
		m_system->updateVelocity( dt );
		t[5] = getTimeAndResetTimer( timer );
		t_Sim += t[0]+t[1]+t[3]+t[4]+t[5];
	}
    t_Sim /=nIter;
	printf("%d: %3.2f, %3.2f, %3.2f, %3.2f, %3.2f, %3.2f\n", m_counter, t[0], t[1], t[2], t[3], t[4], t[5]);
    //fout<<"simulation time::   \t"<< t_Sim <<"\n";

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

char* DemoParticles::textFileRead(const GLchar *filename) 
{
	FILE *fp;
	char *content = NULL;

	int count=0;

	if (filename != NULL) {
		fp = fopen(filename,"rt");

		if (fp != NULL) {
      
      fseek(fp, 0, SEEK_END);
      count = ftell(fp);
      rewind(fp);

			if (count > 0) {
				content = new char[sizeof(char) * (count+1)];
				count = fread(content,sizeof(char),count,fp);
				content[count] = '\0';
			}
			fclose(fp);
		}
	}
	return content;
}

void DemoParticles::_initGL()
{
	char *vs = NULL;
	const char *use_vs = NULL;
	program = glCreateProgram();
	vs = textFileRead("shadowmap.vert");
	use_vs = vs;

	GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    //GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vShader, 1, &use_vs, NULL);
    //glShaderSource(vShader, 1, &vertexShader, 0);
    //glShaderSource(fragmentShader, 1, &fsource, 0);
    
	delete []vs;
	//free(vs);

    glCompileShader(vShader);
    //glCompileShader(fragmentShader);


    glAttachShader(program, vShader);
    //glAttachShader(program, fragmentShader);

    glLinkProgram(program);

    // check if program linked
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
        char temp[256];
        glGetProgramInfoLog(program, 256, 0, temp);
        printf("Failed to link program:\n%s\n", temp);
        glDeleteProgram(program);
        program = 0;
    }

	setshader("shadowmap1.vert","shadowmap1.frag");
	setshaderFloor("shadowmap2.vert","shadowmap2.frag");
    //return program;
}

void DemoParticles::setshader(const GLchar* vertfile, const GLchar* fragfile)
{
	char *vs = NULL,*fs = NULL;
    const char *use_vs = NULL, *use_fs = NULL;

    //glewInit();	
    shaderprogram = glCreateProgram();

    GLuint vertshader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragshader = glCreateShader(GL_FRAGMENT_SHADER);

	vs = textFileRead(vertfile);
	fs = textFileRead(fragfile);

	use_vs = vs;
	use_fs = fs;

	glShaderSource(vertshader, 1, &use_vs, NULL);
	glShaderSource(fragshader, 1, &use_fs, NULL);

	delete []vs;
	delete []fs;
	//free(vs);
	//free(fs);

	glCompileShader(vertshader);
	glCompileShader(fragshader);


	glAttachShader(shaderprogram, vertshader);
	glAttachShader(shaderprogram, fragshader);

	glLinkProgram(shaderprogram);

}

void DemoParticles::setshaderFloor(const GLchar* vertfile, const GLchar* fragfile)
{
	char *vs = NULL,*fs = NULL;
    const char *use_vs = NULL, *use_fs = NULL;

    //glewInit();	
    floorprogram = glCreateProgram();

    GLuint vertshader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragshader = glCreateShader(GL_FRAGMENT_SHADER);

	vs = textFileRead(vertfile);
	fs = textFileRead(fragfile);

	use_vs = vs;
	use_fs = fs;

	glShaderSource(vertshader, 1, &use_vs, NULL);
	glShaderSource(fragshader, 1, &use_fs, NULL);

	delete []vs;
	delete []fs;
	//free(vs);
	//free(fs);

	glCompileShader(vertshader);
	glCompileShader(fragshader);


	glAttachShader(floorprogram, vertshader);
	glAttachShader(floorprogram, fragshader);

	glLinkProgram(floorprogram);

}

void DemoParticles::renderShadow()
{
	//setshader("shadowmap1.vert","shadowmap1.frag");
	const int nBodies = m_system->getNumBodies();
	glPushAttrib(GL_TEXTURE_BIT);
	glUseProgram(shaderprogram);
	texLoc = glGetUniformLocationARB(shaderprogram, "posTex");
	if(texLoc == -1) printf("shadowmap cannot be found in torus shader!!!");
	glUniform1iARB(texLoc, 0);
	texLoc = glGetUniformLocationARB(shaderprogram, "quatTex");
	if(texLoc == -1) printf("shadowmap cannot be found in torus shader!!!");
	glUniform1iARB(texLoc, 1);
	texLoc = glGetUniformLocationARB(shaderprogram, "shadowmap");
	if(texLoc == -1) printf("shadowmap cannot be found in torus shader!!!");
	glUniform1iARB(texLoc, 2);

	//glEnable(GL_TEXTURE_2D); 
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture1);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture2);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowmapid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_NORMAL_ARRAY );
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,indexVBO);
	glBindBufferARB(GL_ARRAY_BUFFER, vertexVBO);
	glVertexPointer(3,GL_FLOAT,sizeof(Vertex),0);
	glNormalPointer(GL_FLOAT,sizeof(Vertex),(void*)(sizeof(Vertex)/2));

	glDrawElementsInstancedEXT(GL_QUADS, 150*4, GL_UNSIGNED_INT, 0, nBodies);
	//glDrawArraysInstancedEXT(GL_QUADS, 0, 150*4, nBodies);

	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_NORMAL_ARRAY );
	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	//glDisable(GL_TEXTURE_2D);
	glPopAttrib();

	glPushAttrib(GL_TEXTURE_BIT);
	glUseProgram(floorprogram);
	texLoc = glGetUniformLocationARB(floorprogram, "colorMap");
	if(texLoc == -1) printf("colorMap cannot be found in floor shader!!!\n");
	glUniform1iARB(texLoc, 0);

	texLoc = glGetUniformLocationARB(floorprogram, "shadowmap");
	if(texLoc == -1) printf("shadowmap cannot be found in floor shader!!!\n");
	glUniform1iARB(texLoc, 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, floorTex);
	//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, GL_BLEND);

	//glEnable(GL_TEXTURE_2D); 
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, shadowmapid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, GL_BLEND);

	glCallList(floor_display_list);
	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	//glDisable(GL_TEXTURE_2D);
	glPopAttrib();
}

void DemoParticles::render()
{
	
	const int nBodies = m_system->getNumBodies();
	clock_t StartTime, EndTime;//Fu-chang Liu
	/*float4* pos = new float4[nBodies];
	TransferUtils::copyDeviceToHost( pos, m_system->getRbPos(), sizeof(float4)*nBodies);
	Quaternion* quat = new Quaternion[nBodies];
	TransferUtils::copyDeviceToHost( quat, m_system->getRbQuat(), sizeof(Quaternion)*nBodies);*/

	//unsigned int timer=0;
	float t;
	//cutCreateTimer(&timer);
	//getTimeAndResetTimer( timer );
	injectPositionRB(m_system->getRbPos(), true);
	//glBindTexture(GL_TEXTURE_2D, 0);
	injectQuaternionRB(m_system->getRbQuat(), true);
	//glBindTexture(GL_TEXTURE_2D, 0);
	//t = getTimeAndResetTimer( timer );
	//printf("inject:: %5.3f\n", t);//Fu-chang Liu

	glPushAttrib(GL_TEXTURE_BIT);
	/*glActiveTexture(GL_TEXTURE0);
	posRB.bind(currentBuffer);
	glActiveTexture(GL_TEXTURE1);
	quatRB.bind(currentBuffer);*/
	glUseProgram(program);
	texLoc = glGetUniformLocationARB(program, "posTex");
	glUniform1iARB(texLoc, 0);
	texLoc = glGetUniformLocationARB(program, "quatTex");
	glUniform1iARB(texLoc, 1);

	//glEnable(GL_TEXTURE_2D); 
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture1);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture2);
	//glUseProgram(program);
	//shader.renderRigidBodyVP();

// for box simualtion
/*
	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_NORMAL_ARRAY );
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,indexVBO);
	glBindBufferARB(GL_ARRAY_BUFFER, vertexVBO);
	glVertexPointer(3,GL_FLOAT,sizeof(Vertex),0);
	glNormalPointer(GL_FLOAT,sizeof(Vertex),(void*)(sizeof(Vertex)/2));

	for(int i=0;i<nBodies;i++){
		glTexCoord2f(i,0);
		glDrawElements(GL_TRIANGLES,12*3,GL_UNSIGNED_INT,0);
	}

	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_NORMAL_ARRAY );
*/

	//glFlush();
	//glFinish();
    //float sTime=(GLfloat)glutGet(GLUT_ELAPSED_TIME);
	//const int nIter = 50;	
	//for(int i=0; i<nIter; i++)
	//{
	//for torus
	/*for(int i=0;i<nBodies;i++){
		if(i%2 == 0)
		{
			glTexCoord2f(i,0);
			glCallList(torus_display_list1);
		}
		else
		{
			glTexCoord2f(i,0);
			glCallList(torus_display_list2);
		}
	}*/
	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_NORMAL_ARRAY );
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,indexVBO);
	glBindBufferARB(GL_ARRAY_BUFFER, vertexVBO);
	glVertexPointer(3,GL_FLOAT,sizeof(Vertex),0);
	glNormalPointer(GL_FLOAT,sizeof(Vertex),(void*)(sizeof(Vertex)/2));

	//glFinish();
    //float sTime=(GLfloat)glutGet(GLUT_ELAPSED_TIME);
	//for(int i=0;i<nBodies;i++)
	{
		//glTexCoord2f(i,0);
		//glDrawElements(GL_QUADS, 150*4*1024*16, GL_UNSIGNED_INT, 0);
		GLuint m_iTimeQuery;
		GLuint timeElapsed = 0;
		// Create a query object.
        glGenQueries(1, &m_iTimeQuery);
		// Query current timestamp 1

		GLint available = 0;
		// See how much time the rendering of object i took in nanoseconds.
		glBeginQuery(GL_TIME_ELAPSED, m_iTimeQuery);
		glDrawElementsInstancedEXT(GL_QUADS, 150*4, GL_UNSIGNED_INT, 0, nBodies);
		glEndQuery(GL_TIME_ELAPSED);
		while (!available) {
            //DoSomeWork(); // <== This way we do some extra work while waiting
            glGetQueryObjectiv(m_iTimeQuery, GL_QUERY_RESULT_AVAILABLE, &available);
        }
        glGetQueryObjectuiv(m_iTimeQuery, GL_QUERY_RESULT, &timeElapsed);
		//printf("glDrawE:: %d\n", timeElapsed);//Fu-chang Liu*/
	}

	//glDisableClientState( GL_VERTEX_ARRAY );
	//glDisableClientState( GL_NORMAL_ARRAY );
	//glFlush();
	//}
	//glFinish();
	//float eTime=(GLfloat)glutGet(GLUT_ELAPSED_TIME);
	//t = eTime - sTime;//Fu-chang Liu
	//t = t/50.0;
    //printf("glDrawE:: %5.3f\n", t);//Fu-chang Liu
	//fout<<"DrawElements time::   \t"<< t<<"\n";

	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_NORMAL_ARRAY );
	glUseProgram(0);
	//shader.disableVP();

	//glCallList(floor_display_list);

	glBindTexture(GL_TEXTURE_2D, 0);

	glPopAttrib();

	//glCallList(floor_display_list);

	/*currentBuffer = (currentBuffer+1)%2;

	injectPositionRB(pos);
	injectQuaternionRB(quat);


	delete pos;
	delete quat;*/
/*
	if( numParticlesPerObj == 9 )
	{
		int3 tris[12];
		float4 vtx[8];
		createBox( m_pRadius*2, m_pRadius*2, m_pRadius*2, tris, vtx );

		for(int i=0; i<nBodies; i++)
		{
			float4 localVtx[8];
			for(int j=0; j<8; j++)
			{
				VecUtils::rotateVector(quat[i], vtx[j],localVtx[j]);
				localVtx[j].x += pos[i].x;
				localVtx[j].y += pos[i].y;
				localVtx[j].z += pos[i].z;
			}
			float c = 0.5f;
			glColor3f(c,c,c);
			if( m_debugDisplay )
			{
				glBegin(GL_LINES);
				for(int j=0; j<12; j++)
				{
					float4 x = localVtx[tris[j].x];
					float4 y = localVtx[tris[j].y];
					float4 z = localVtx[tris[j].z];
					glVertex3f(x.x, x.y, x.z); glVertex3f(y.x, y.y, y.z);
					glVertex3f(y.x, y.y, y.z); glVertex3f(z.x, z.y, z.z);
					glVertex3f(z.x, z.y, z.z); glVertex3f(x.x, x.y, x.z);
				}
				glEnd();
				glColor3f(1,1,1);
				for(int j=0; j<8; j++)
				{
//					glDraw3DStrings(localVtx[j].y, localVtx[j]);
				}
			}
			else
			{
				glEnable(GL_NORMALIZE);
				glEnable(GL_LIGHTING);
				glEnable(GL_LIGHT0);

				int tIdx = i%s_tableSize;
				float cs = 0.6f;
				glColor3f(colorTable[tIdx][0]+cs, colorTable[tIdx][1]+cs, colorTable[tIdx][2]+cs);

				float scale, ax, ay, az, angle;
				scale = sqrt(quat[i].x*quat[i].x+quat[i].y*quat[i].y+quat[i].z*quat[i].z);
				ax = quat[i].x/scale;
				ay = quat[i].y/scale;
				az = quat[i].z/scale;
				angle = 2.0f*acos(quat[i].w);
				glPushMatrix();
				glRotatef(angle, ax,ay,az);
				glTranslatef(pos[i].x, pos[i].y, pos[i].z);
				//glRotatef(angle, ax,ay,az);
				glCallList(torus_display_list);
				glPopMatrix();
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

				glDisable(GL_LIGHTING);
				glDisable(GL_LIGHT0);
			}
		}
		if( !m_debugDisplay )
		{
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
	}*/

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

	//delete pos;
	//delete quat;
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
	
	/*int index0, index1;
	if(numPairs > 0)
	{
		//std::ofstream fout("2.txt");
		for(uint i=0;i<numPairs;i++) // for validation
		{
			index0 = pairs[i].x;
			index1 = pairs[i].y;
			fout<<"("<<index0<<"\t"<<index1<<")\n"
				  <<particlePos[index0].x<<"\t"<<particlePos[index0].y<<"\t"<<particlePos[index0].z<<"\t"<<particlePos[index0].w<<"\n"
				  <<particlePos[index1].x<<"\t"<<particlePos[index1].y<<"\t"<<particlePos[index1].z<<"\t"<<particlePos[index1].w<<"\n"
				  <<particlePos[521].x<<"\t"<<particlePos[521].y<<"\t"<<particlePos[521].z<<"\t"<<particlePos[521].w<<"\n";
		}
		fout<<"overlap pairs "<<numPairs<<"\n";
		fout<<"-------------------------------------------------------------\n";
		for(uint i=0;i<numPairEntries;i++) // for validation
			fout<<pairEntries[i]<<"\n";
		fout<<"pairsEntries "<<numPairEntries<"\n";
	}*/

	//	check pairEntries
	TransferUtils::copyHostToDevice( m_system->getPairs(), pairs, sizeof(int2)*numPairs );
	m_system->setNumPairs( numPairs );
	TransferUtils::copyHostToDevice( m_system->getPairEntries(), pairEntries, sizeof(int)*numPairEntries);
	m_system->setNumPairEntries( numPairEntries );

	delete particlePos;
	delete pairs;
	delete pairEntries;
}

void DemoParticles::getWorldExtent(float4& maxE, float4& minE)
{
		float4* particlePos = new float4[m_system->getNumParticles()];
		TransferUtils::copyDeviceToHost( particlePos, m_system->getParticlePos(), sizeof(float4)*m_system->getNumParticles() );

		maxE = make_float4(-10000.f);
		minE = make_float4(10000.f);
		for(int i=0; i<m_system->getNumParticles(); i++)
		{
			const float4& iPos = particlePos[i];
			float4 iMax = iPos + make_float4(iPos.w);
			maxE.x = max( maxE.x, iMax.x );
			maxE.y = max( maxE.y, iMax.y );
			maxE.z = max( maxE.z, iMax.z );

			float4 iMin = iPos - make_float4(iPos.w);
			minE.x = min( minE.x, iMin.x );
			minE.y = min( minE.y, iMin.y );
			minE.z = min( minE.z, iMin.z );
		}

		delete particlePos;
}
    
void DemoParticles::snapShot(void)
{
  FILE *fp;

  // GENERATE A UNIQUE FILENAME
  char FileName[20];
  static int SnapShotNum=10000;
  int UniqueFound=0;  
  do { sprintf(FileName,"snap/snap%d.ppm",SnapShotNum);
       if (fp=fopen(FileName,"r")) fclose(fp); else UniqueFound=1;
       SnapShotNum++;
     } while(!UniqueFound);

  GLint OldReadBuffer;
  glGetIntegerv(GL_READ_BUFFER,&OldReadBuffer);
  glReadBuffer(GL_FRONT);

  GLint OldPackAlignment;
  glGetIntegerv(GL_PACK_ALIGNMENT,&OldPackAlignment); 
  glPixelStorei(GL_PACK_ALIGNMENT,1);

  int WW = glutGet((GLenum)GLUT_WINDOW_WIDTH);
  int WH = glutGet((GLenum)GLUT_WINDOW_HEIGHT);

  int NumPixels = WW*WH;
    
  printf("%d x %d: %s\n",WW,WH,FileName);

  GLubyte* Pixels = new GLubyte[NumPixels*3];
  glReadPixels(0,0,WW,WH,GL_RGB,GL_UNSIGNED_BYTE,Pixels);

  fp = fopen(FileName, "wb");
  fprintf(fp, "P6\n%d %d\n255\n", WW, WH);
  for (int i=((WH-1)*WW)*3; i>=0; i-=(WW*3))
    fwrite(&(Pixels[i]),1,WW*3,fp);
  fclose(fp);
  delete[] Pixels;

  glPixelStorei(GL_PACK_ALIGNMENT,OldPackAlignment);
  glReadBuffer(OldReadBuffer);
}

#endif

