/**
*	@author Takahiro HARADA
*/
#ifdef _WIN32
#pragma comment( lib, "cg.lib" )
#pragma comment( lib, "cggl.lib" )
#pragma comment( lib, "glew32.lib" )
#pragma comment(lib,"cutil32.lib")
#endif

#include <stdlib.h>
#include <GL/glew.h>
#include <GL/wglew.h>
#include <GL/glut.h>

#include <Cg/cg.h>
#include <Cg/cgGL.h>

#include <stdio.h>
#include <iostream>
//#include <fstream>
#include <time.h> // Fu-chang Liu

using namespace std;
#ifdef _WIN32
#include "openGLExtension.h"
#endif

#include "demo.h"
#include "demoParticles.h"
#include "Matrix16.h"
#include <wingdi.h>

#define DEMO_NAME DemoParticles

int fCounter = 0;
// mouse controls
int mouse_old_x, mouse_old_y;
int mouse_buttons = 0;
float rotate_x = 0.0+10.0, rotate_y = 0.0-30.0;
float translate_x = 0.0f;
float translate_y = 0.0f;
float translate_z = 0.0f+30.0;
int wWidth = 1920, wHeight = 1080;
float rot=0;
float rotY=0;
bool autoMove = true;//false;
int useTextures = 1;
int useMipmaps = 1;
int linearFiltering = 1;
double L[4] = { 100, 100, 100, 1 };   /* Light location. */
double Pg[4];                 /* Plane at the actual ground plane. */
/* Three vertices on the ground plane. */
double Sg[3] = { 0, 0, 0 };
double Tg[3] = { 0, 0, 1 };
double Rg[3] = { 1, 0, 0 };
enum {
  X, Y, Z, W  /* Used to make code involving coordinates more readable. */
};
DEMO_NAME* s_demo;
GLuint floorTex = -1;
//std::ofstream fout ("1.txt");
GLuint depthFBO;
GLuint shadowmapid;
GLuint colorTextureId;

CMatrix16 Light_ProjectionMatrix;
CMatrix16 Light_ViewMatrix;
CMatrix16 Camera_ProjectionMatrix;
CMatrix16 Camera_ViewMatrix;
CMatrix16 Camera_ViewInverse;
float dx = 0.02;

/* Nice floor texture tiling pattern. */
static char *circles[] = {
  ".xxxxxxxxxxxxx..",
  ".x.....x..x..x..",
  ".xxxxxxxxxxxxx..",
  ".x.....x.....x..",
  ".xxxxxxxxxxxxx..",
  ".x.....x..x..x..",
  ".xxxxxxxxxxxxx..",
  ".x.....x..x..x..",
  ".xxxxxxxxxxxxx..",
  ".x..x..x..x..x..",
  ".xxxxxxxxxxxxx..",
  ".x.....x..x..x..",
  ".xxxxxxxxxxxxx..",
  "................",
  "................",
  "................",
};

unsigned char *LoadBitmapFile(char *filename, BITMAPINFOHEADER *bitmapInfoHeader)
{
	FILE *filePtr = NULL; //our file pointer
	BITMAPFILEHEADER bitmapFileHeader; //our bitmap file header
	unsigned char *bitmapImage = NULL;  //store image data
	int imageIdx=0;  //image index counter
	unsigned char tempRGB;  //our swap variable

	//open filename in read binary mode
	filePtr = fopen(filename,"rb");
	if (filePtr == NULL)
	return NULL;

	//read the bitmap file header
	fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER),1,filePtr);

	//verify that this is a bmp file by check bitmap id
	if (bitmapFileHeader.bfType !=0x4D42)
	{
	fclose(filePtr);
	return NULL;
	}

	//read the bitmap info header
	fread(bitmapInfoHeader, sizeof(BITMAPINFOHEADER),1,filePtr);

	//move file point to the begging of bitmap data
	fseek(filePtr, bitmapFileHeader.bfOffBits, SEEK_SET);

	//allocate enough memory for the bitmap image data
	bitmapImage = (unsigned char*)malloc(bitmapInfoHeader->biSizeImage);
    printf("image width %d\n", bitmapInfoHeader->biWidth);
	printf("image height %d\n", bitmapInfoHeader->biHeight);
	printf("image size %d\n", bitmapInfoHeader->biSizeImage);
	//verify memory allocation
	if (!bitmapImage)
	{
	free(bitmapImage);
	fclose(filePtr);
	return NULL;
    }

	//read in the bitmap image data
	fread(bitmapImage,bitmapInfoHeader->biSizeImage,1,filePtr);

	//make sure bitmap image data was read
	if (bitmapImage == NULL)
	{
	fclose(filePtr);
	return NULL;
	}

	//swap the r and b values to get RGB (bitmap is BGR)
	/*for (imageIdx = 0;imageIdx < bitmapInfoHeader->biSizeImage;imageIdx+=3)
	{
	tempRGB = bitmapImage[imageIdx];
	bitmapImage[imageIdx] = bitmapImage[imageIdx + 2];
	bitmapImage[imageIdx + 2] = tempRGB;
	printf("Blue color %u\n", bitmapImage[imageIdx]);
	printf("Green color %u\n", bitmapImage[imageIdx+1]);
	printf("Red color %u\n\n", bitmapImage[imageIdx+2]);
	}*/

	//close file and return bitmap iamge data
	fclose(filePtr);
	return bitmapImage;
}

void raw_texture_load(char *filename, int width, int height)
 {
    //GLuint texture;
    // Generate name for and bind texture.
    //glGenTextures(1, &floorTex);

	BITMAPINFOHEADER bitmapInfoHeader;
    unsigned char *Data;
    Data = LoadBitmapFile(filename, &bitmapInfoHeader);

	  glBindTexture(GL_TEXTURE_2D,floorTex);
	  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	  
	  if (useMipmaps) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		  GL_LINEAR_MIPMAP_LINEAR);
		gluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height,
		  GL_RGB, GL_UNSIGNED_BYTE, Data);
	  } else {
		if (linearFiltering) {
		  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		} else {
		  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
		glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0,
		  GL_RGB, GL_UNSIGNED_BYTE, Data);
	  }

	  //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, GL_BLEND);

	  glBindTexture(GL_TEXTURE_2D,0);

	free(Data);

 
     //return texture;
 }

static void
makeFloorTexture(void)
{
  GLubyte floorTexture[16][16][3];
  GLubyte *loc;
  int s, t;

  /* Setup RGB image for the texture. */
  loc = (GLubyte*) floorTexture;
  for (t = 0; t < 16; t++) {
    for (s = 0; s < 16; s++) {
      if (circles[t][s] == 'x') {
	/* Nice green. */
        loc[0] = 0x1f;
        loc[1] = 0x8f;
        loc[2] = 0x1f;
      } else {
	/* Light gray. */
        loc[0] = 0xaa;
        loc[1] = 0xaa;
        loc[2] = 0xaa;
      }
      loc += 3;
    }
  }

  glBindTexture(GL_TEXTURE_2D,floorTex);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  
  if (useMipmaps) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
      GL_LINEAR_MIPMAP_LINEAR);
    gluBuild2DMipmaps(GL_TEXTURE_2D, 3, 16, 16,
      GL_RGB, GL_UNSIGNED_BYTE, floorTexture);
  } else {
    if (linearFiltering) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    } else {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, 3, 16, 16, 0,
      GL_RGB, GL_UNSIGNED_BYTE, floorTexture);
  }
  glBindTexture(GL_TEXTURE_2D,0);
}
void setModelViewMatrix()
{
	glLoadIdentity();
	glTranslatef(translate_x, translate_y, translate_z);
	glRotatef(rotate_x, 1.0, 0.0, 0.0);
	glRotatef(rotate_y, 0.0, 1.0, 0.0);
}
void setPerspectiveProjMatrix()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(65.0, (double)wWidth / (double)wHeight, 0.1, 100.0);
	glMatrixMode(GL_MODELVIEW);
}
void setOrthoProjMatrix()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1,1,-1,1,-100,100);
	glMatrixMode(GL_MODELVIEW);
}
void
drawFloor(void)
{
  float x, y;
  float d = 1.0;
  float s = 60.0f;
  /* Draw ground. */
  if (useTextures) {
    glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,floorTex);
  }
  glColor3f(1,1,0);
  glNormal3f(0,1,0);
  glPushMatrix();
  glTranslatef(0.0, 1.0, 0.0);
  //glRotatef(rotate_x, 1.0, 0.0, 0.0);
  //glRotatef(rotate_y, 0.0, 1.0, 0.0);

  /* Tesselate floor so lighting looks reasonable. */
  for (x=-s; x<s; x+=d) {
    glBegin(GL_QUAD_STRIP);
    for (y=-s; y<s; y+=d) {
      glTexCoord2f(x+1, y);
      glVertex3f(x+1, -1, y);
      glTexCoord2f(x, y);
      glVertex3f(x, -1, y);
    }
    glEnd();
  }
  glPopMatrix();

  if (useTextures) {
	glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_TEXTURE_2D);
  }
}
void drawObjects()
{	
	glPushMatrix();
	//glTranslatef(translate_x, translate_y, translate_z);
	//glRotatef(rotate_x, 1.0, 0.0, 0.0);
	//glRotatef(rotate_y, 0.0, 1.0, 0.0);
	s_demo->render();
	//glutSolidTorus(0.25, 0.5, 10, 12);
	glPopMatrix();

	/*float s = 5.0f;
	glBegin(GL_QUADS);
	{
		glNormal3f(0,1,0);
		glVertex3f(-s,0,-s);
		glVertex3f(s,0,-s);
		glVertex3f(s,0,s);
		glVertex3f(-s,0,s);
	}
	glEnd();*/
}
void RenderObjects()
{	
	glPushMatrix();
	//glTranslatef(translate_x, translate_y, translate_z);
	//glRotatef(rotate_x, 1.0, 0.0, 0.0);
	//glRotatef(rotate_y, 0.0, 1.0, 0.0);
	s_demo->renderShadow();
	//glutSolidTorus(0.25, 0.5, 10, 12);
	glPopMatrix();

	/*float s = 5.0f;
	glBegin(GL_QUADS);
	{
		glNormal3f(0,1,0);
		glVertex3f(-s,0,-s);
		glVertex3f(s,0,-s);
		glVertex3f(s,0,s);
		glVertex3f(-s,0,s);
	}
	glEnd();*/
}
/* Find the plane equation given 3 points. */
void
findPlane(double plane[4],
  double v0[3], double v1[3], double v2[3])
{
  double vec0[3], vec1[3];

  /* Need 2 vectors to find cross product. */
  vec0[0] = v1[0] - v0[0];
  vec0[1] = v1[1] - v0[1];
  vec0[2] = v1[2] - v0[2];

  vec1[0] = v2[0] - v0[0];
  vec1[1] = v2[1] - v0[1];
  vec1[2] = v2[2] - v0[2];

  /* find cross product to get A, B, and C of plane equation */
  plane[0] = vec0[1] * vec1[2] - vec0[2] * vec1[1];
  plane[1] = -(vec0[0] * vec1[2] - vec0[2] * vec1[0]);

  plane[2] = vec0[0] * vec1[1] - vec0[1] * vec1[0];

  plane[3] = -(plane[0] * v0[0] + plane[1] * v0[1] + plane[2] * v0[2]);
}
void
ambientLight(int enable)
{
  GLfloat on[4] = { 0.92, 0.92, 0.92, 1.0 };
  GLfloat off[4] = { 0.0, 0.0, 0.0, 1.0 };

  if (enable) {
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, on);
  } else {
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, off);
  }
}
/* Create a matrix that will project the desired shadow. */
void
shadowMatrix(GLfloat shadowMat[4][4],
  GLdouble groundplane[4],
  GLfloat lightpos[4])
{
  GLfloat dot;

  /* Find dot product between light position vector and ground plane normal. */
  dot = groundplane[X] * lightpos[X] +
    groundplane[Y] * lightpos[Y] +
    groundplane[Z] * lightpos[Z] +
    groundplane[W] * lightpos[W];

  shadowMat[0][0] = dot - lightpos[X] * groundplane[X];
  shadowMat[1][0] = 0.f - lightpos[X] * groundplane[Y];
  shadowMat[2][0] = 0.f - lightpos[X] * groundplane[Z];
  shadowMat[3][0] = 0.f - lightpos[X] * groundplane[W];

  shadowMat[X][1] = 0.f - lightpos[Y] * groundplane[X];
  shadowMat[1][1] = dot - lightpos[Y] * groundplane[Y];
  shadowMat[2][1] = 0.f - lightpos[Y] * groundplane[Z];
  shadowMat[3][1] = 0.f - lightpos[Y] * groundplane[W];

  shadowMat[X][2] = 0.f - lightpos[Z] * groundplane[X];
  shadowMat[1][2] = 0.f - lightpos[Z] * groundplane[Y];
  shadowMat[2][2] = dot - lightpos[Z] * groundplane[Z];
  shadowMat[3][2] = 0.f - lightpos[Z] * groundplane[W];

  shadowMat[X][3] = 0.f - lightpos[W] * groundplane[X];
  shadowMat[1][3] = 0.f - lightpos[W] * groundplane[Y];
  shadowMat[2][3] = 0.f - lightpos[W] * groundplane[Z];
  shadowMat[3][3] = dot - lightpos[W] * groundplane[W];
}
void setlight()
{
	//GLdouble P[5][3];
	//GLdouble matrix[4][4];
	//GLfloat shadowMat[4][4];
	GLfloat Lf[4];
	GLfloat Lc[4];

	/* Use a white light. */
	Lc[0] = 1.0;
	Lc[1] = 1.0;
	Lc[2] = 1.0;
	Lc[3] = 1.0;
	glLightfv(GL_LIGHT0, GL_DIFFUSE, &Lc[0]);
	glLightfv(GL_LIGHT0, GL_SPECULAR, &Lc[0]);

	//glColorMask(1,1,1,1);
	//glDepthMask(1);
	//glStencilMask(~0u);
	/* GL_LESS means that depth test "ties" go to the FIRST tieing fragment.
	This is OpenGL's default depth test, but when depth testing with
	shadow volumes, we later use GL_EQUAL to match depth values.  However,
	GL_EQUAL means that depth test "ties" go to the LAST tieing fragment.
	Therefore, we use GL_LEQUAL initially so we can match the later
	"depth test tie goes to the last fragment" passes. */
	//glDepthFunc(GL_LEQUAL);
	//glDisable(GL_STENCIL_TEST);

	glEnable(GL_LIGHT0);

	ambientLight(1);

	Lf[0] = L[0];
	Lf[1] = L[1];
	Lf[2] = L[2];
	Lf[3] = L[3];
	glLightfv(GL_LIGHT0, GL_POSITION, &Lf[0]);
    
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	//shadowMatrix(shadowMat, Pg, Lf);

	//glMatrixMode( GL_MODELVIEW );
    //glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(Light_ProjectionMatrix.mt);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(Light_ViewMatrix.mt);
    //glTranslatef(translate_x, translate_y, translate_z);
    //glRotatef(rotate_x, 1.0, 0.0, 0.0);
    //glRotatef(rotate_y, 0.0, 1.0, 0.0);

	glDisable(GL_LIGHTING);
	glPushMatrix();
	//glTranslatef(translate_x, translate_y, translate_z);
    //glRotatef(rotate_x, 1.0, 0.0, 0.0);
    //glRotatef(rotate_y, 0.0, 1.0, 0.0);
	/* Draw light source position as a Red dot. */
	glColor3f(1,0,0);
	glBegin(GL_POINTS);
	glVertex3dv(L);
	glEnd();
	glPopMatrix();
	glEnable(GL_LIGHTING);

	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
}
void GenShadowMap()
{
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); 	    
    glShadeModel(GL_SMOOTH);

	glPolygonOffset(0.5f, 10.0f);
	glEnable(GL_POLYGON_OFFSET_FILL);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, depthFBO);
	glDrawBuffer(GL_DEPTH_ATTACHMENT_EXT);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawObjects();//////////////////////////1
	//drawFloor();////////////////////////////1
	
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);


    glShadeModel(GL_SMOOTH);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); 
	glDisable(GL_POLYGON_OFFSET_FILL);
}
void CastShadowMap()
{	
    //glEnable(GL_TEXTURE_2D); 
	//glBindTexture(GL_TEXTURE_2D, shadowmapid);

	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	////glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);

    glMatrixMode(GL_TEXTURE);	
	//glPushMatrix();
	glPushAttrib(GL_TEXTURE_BIT);
	glActiveTexture(GL_TEXTURE7);
	glLoadIdentity();	
	glLoadMatrixf(Light_ProjectionMatrix.mt);
	glMultMatrixf(Light_ViewMatrix.mt);
	glMultMatrixf(Camera_ViewInverse.mt);
	//glPopMatrix();

    glMatrixMode(GL_MODELVIEW);	
    RenderObjects();
	//glBindTexture(GL_TEXTURE_2D,0);	
    //glDisable(GL_TEXTURE_2D);
	glPopAttrib();
}
void SetFrameBufferObject(int fbowidth, int fboheight)
{

    glGenFramebuffersEXT(1, &depthFBO);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, depthFBO);

   	glGenRenderbuffersEXT(1, &colorTextureId);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, colorTextureId);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_RGBA, fbowidth, fboheight);
    

    glGenTextures(1, &shadowmapid);
    glBindTexture(GL_TEXTURE_2D, shadowmapid);
  	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, fbowidth, fboheight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);						
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, colorTextureId);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, shadowmapid, 0);

	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if(status != GL_FRAMEBUFFER_COMPLETE_EXT)
		printf( "FBO Initialization Failed.\n" );


	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}
void clearFrameBufferObject()
{
	glDeleteFramebuffersEXT(1, &depthFBO);
    glDeleteTextures(1, &shadowmapid);
    glDeleteTextures(1, &colorTextureId);	
}
void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	setlight();
	GenShadowMap();
	//drawObjects();//////////////////////////1
	//drawFloor();////////////////////////////1

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(Camera_ProjectionMatrix.mt);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(Camera_ViewMatrix.mt);

    glTranslatef(translate_x, translate_y, translate_z);
    glRotatef(rotate_x, 1.0, 0.0, 0.0);
    glRotatef(rotate_y, 0.0, 1.0, 0.0);
	if( autoMove ){
	rotate_y -= 0.1;
    translate_z-=0.025;
	fCounter++;
	}
	if(fCounter > 120 && fCounter <= 190)
	{
		dx *=1.06;
		//translate_z-=0.20;
		rotate_x += dx;
		//rotate_y -= 0.05;
	}
	else if(fCounter > 300)
	{
		autoMove = false;
		//rotate_x += 0.05;
		//translate_z-=0.30;
	}
	////glGetFloatv(GL_MODELVIEW_MATRIX, Light_ViewMatrix.mt);
	glGetFloatv(GL_MODELVIEW_MATRIX, Camera_ViewInverse.mt);
	Camera_ViewInverse = Camera_ViewInverse.GetmvMatrixInverse();
	//drawObjects();//////////////////////////2
	CastShadowMap();
	//drawFloor();////////////////////////////2
	glutSwapBuffers();
}
void idle(void)
{
	if( autoMove )
	{
		s_demo->step();
		s_demo->readback();
		//s_demo->snapShot();
	}

	glutPostRedisplay();
}
void resize(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	//glOrtho(-200,200,-200,200,-200,200);
	gluPerspective(65.0, (double)w / (double)h, 0.001, 1000.0);
	//gluPerspective(60.0, (GLdouble)w/h, 1.0, 20.0);
	glGetFloatv(GL_PROJECTION_MATRIX, Camera_ProjectionMatrix.mt);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	gluLookAt(.0, .0, 100.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	glGetFloatv(GL_MODELVIEW_MATRIX, Camera_ViewMatrix.mt);
	//glGetFloatv(GL_MODELVIEW_MATRIX, Camera_ViewInverse.mt);
	////Camera_ViewInverse = Camera_ViewInverse.GetmvMatrixInverse();
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluPerspective(45.0f, (double)w / (double)h, 0.001, 1000.0);
	glGetFloatv(GL_PROJECTION_MATRIX, Light_ProjectionMatrix.mt);
	glPopMatrix();
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	gluLookAt(	L[0], L[1], L[2],
				0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f);
	glGetFloatv(GL_MODELVIEW_MATRIX, Light_ViewMatrix.mt);
	glPopMatrix();

	clearFrameBufferObject();
	SetFrameBufferObject(w, h);// shadow Sept.24th
	wWidth=w;wHeight=h;
}
void init(void)
{
	checkGLExtensions();
	glShadeModel(GL_SMOOTH);// Enable Smooth Shading, shadow Sept.24th
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);// Black Background, shadow Sept.24th
	glClearDepth(1.0f);	// Depth Buffer Setup, shadow Sept.24th
	glEnable(GL_DEPTH_TEST);// Enables Depth Testing, shadow Sept.24th
	glDepthFunc(GL_LEQUAL);	// shadow Sept.24th	
	glEnable(GL_TEXTURE_2D);// shadow Sept.24th
	SetFrameBufferObject(wWidth, wHeight);// shadow Sept.24th

	glGenTextures(1,&floorTex);
	raw_texture_load("background.bmp", 1000, 800);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	//glOrtho(-200,200,-200,200,-200,200);
	gluPerspective(65.0, (double)wWidth / (double)wHeight, 0.001, 1000.0);
	//gluPerspective(60.0, (GLdouble)w/h, 1.0, 20.0);
	glGetFloatv(GL_PROJECTION_MATRIX, Camera_ProjectionMatrix.mt);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	gluLookAt(.0, .0, 100.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	glGetFloatv(GL_MODELVIEW_MATRIX, Camera_ViewMatrix.mt);
	//glGetFloatv(GL_MODELVIEW_MATRIX, Camera_ViewInverse.mt);
	////Camera_ViewInverse = Camera_ViewInverse.GetmvMatrixInverse();
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluPerspective(45.0f, (double)wWidth / (double)wHeight, 0.001, 1000.0);
	glGetFloatv(GL_PROJECTION_MATRIX, Light_ProjectionMatrix.mt);
	glPopMatrix();
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	gluLookAt(	L[0], L[1], L[2],
				0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f);
	glGetFloatv(GL_MODELVIEW_MATRIX, Light_ViewMatrix.mt);
	glPopMatrix();

	glPointSize(16.0);
    //findPlane(Pg, Sg, Tg, Rg);

	glutSetWindowTitle("Demo: Rigid Body System");

	/*if (useTextures) 
	{
		makeFloorTexture();
    }*/
	s_demo = new DEMO_NAME();
	s_demo->init();
}
void mouse(int button, int state, int x, int y)
{	
	if (state == GLUT_DOWN) {
		mouse_buttons |= 1<<button;
	} else if (state == GLUT_UP) {
		mouse_buttons = 0;
	}
	if (glutGetModifiers() & GLUT_ACTIVE_SHIFT
		 && state == GLUT_DOWN){
		mouse_buttons |= 2 << 2;
	}
	
	mouse_old_x = x;
	mouse_old_y = y;
	glutPostRedisplay();
}
void motion(int x, int y)
{
	float dx, dy;
	dx = x - mouse_old_x;
	dy = y - mouse_old_y;
	
	if(mouse_buttons & (2 << 2) && mouse_buttons & 1){
		
	}else if (mouse_buttons & 1) {
		rotate_x += dy * 0.2;
		rotate_y += dx * 0.2;
	} else if (mouse_buttons & (2<<2)) {
		translate_z += dy*0.01f;
	} else if (mouse_buttons & 4){
		translate_x += dx * 0.005;
		translate_y -= dy * 0.005;
	}
	
	
	mouse_old_x = x;
	mouse_old_y = y;
}
void keyboard(unsigned char key, int x, int y)
{
	const char str = key;
	float dz = 0.5f;
	
	switch (key) {
		case's':
			translate_z+=dz;
			break;
		case'S':
			translate_z-=dz;
			break;

		case 'q':
		case 'Q':
		case '\033':
			glDeleteTextures(1, &floorTex);
			clearFrameBufferObject();
			delete s_demo;
			exit(0);
		case 'a':
			autoMove = !autoMove;
			break;
		case 'd':
			s_demo->m_debugDisplay = !s_demo->m_debugDisplay;
			break;
		case ' ':
			s_demo->step();
			s_demo->readback();
			break;
		default:
			break;
	}
	glutPostRedisplay();
}
int main(int argc, char *argv[])
{
	glutInitWindowSize(wWidth, wHeight);
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);
	glutCreateWindow(argv[0]);

	glutDisplayFunc(display);
	glutReshapeFunc(resize);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutKeyboardFunc(keyboard);
	glutIdleFunc(idle);
	init();
	glutMainLoop();
	
	return 0;
}