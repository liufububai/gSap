#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h> 
#include <malloc.h> //darwin includes stdlib.h rather than malloc.h
#include <math.h>
#include <time.h>
#include <unistd.h>
#include "obj.h"

//circle
 #define GL_PI 3.14159265 
 #define EDGES 30 

//isocowhatever
#define X .525731112119133606 
#define Z .850650808352039932


#ifdef __APPLE__
    #include <OpenGL/glu.h>
    #include <OpenGL/glext.h>
#else
    #include <GL/glu.h>
    #include <GL/glext.h>
    #include <GL/glx.h>
    #include <GL/glxext.h>
    #define glXGetProcAddress(x) (*glXGetProcAddressARB)((const GLubyte*)x)
#endif


#if !defined(__APPLE__) && !defined(_WIN32)

PFNGLCREATEPROGRAMOBJECTARBPROC     glCreateProgramObjectARB = NULL;
PFNGLCREATESHADEROBJECTARBPROC      glCreateShaderObjectARB = NULL;
PFNGLSHADERSOURCEARBPROC            glShaderSourceARB = NULL;
PFNGLCOMPILESHADERARBPROC           glCompileShaderARB = NULL;
PFNGLGETOBJECTPARAMETERIVARBPROC    glGetObjectParameterivARB = NULL;
PFNGLATTACHOBJECTARBPROC            glAttachObjectARB = NULL;
PFNGLGETINFOLOGARBPROC              glGetInfoLogARB = NULL;
PFNGLLINKPROGRAMARBPROC             glLinkProgramARB = NULL;
PFNGLUSEPROGRAMOBJECTARBPROC        glUseProgramObjectARB = NULL;
PFNGLGETUNIFORMLOCATIONARBPROC      glGetUniformLocationARB = NULL;
PFNGLUNIFORM3FARBPROC               glUniform3fARB = NULL;
PFNGLUNIFORM2FARBPROC               glUniform2fARB = NULL;
PFNGLUNIFORM4FVARBPROC              glUniform4fvARB = NULL;
PFNGLUNIFORM1FARBPROC               glUniform1fARB = NULL;
PFNGLUNIFORM1FVARBPROC              glUniform1fvARB = NULL;
#endif


#ifdef __APPLE__
void SetupGLSLProcs()
{
    //do nothing
}

#elif !defined(_WIN32)

void SetupGLSLProcs()
{
    glCreateProgramObjectARB = (PFNGLCREATEPROGRAMOBJECTARBPROC) 
	glXGetProcAddress("glCreateProgramObjectARB");
    glCreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC)
        glXGetProcAddress("glCreateShaderObjectARB");
    glShaderSourceARB = (PFNGLSHADERSOURCEARBPROC)
        glXGetProcAddress("glShaderSourceARB");
    glCompileShaderARB = (PFNGLCOMPILESHADERARBPROC)
        glXGetProcAddress("glCompileShaderARB");
    glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC)
        glXGetProcAddress("glGetObjectParameterivARB");
    glAttachObjectARB = (PFNGLATTACHOBJECTARBPROC)
        glXGetProcAddress("glAttachObjectARB");
    glGetInfoLogARB = (PFNGLGETINFOLOGARBPROC)
        glXGetProcAddress("glGetInfoLogARB");
    glLinkProgramARB = (PFNGLLINKPROGRAMARBPROC)
        glXGetProcAddress("glLinkProgramARB");
    glUseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC)
        glXGetProcAddress("glUseProgramObjectARB");
    glGetUniformLocationARB = (PFNGLGETUNIFORMLOCATIONARBPROC)
        glXGetProcAddress("glGetUniformLocationARB");
    glUniform3fARB = (PFNGLUNIFORM3FARBPROC)
        glXGetProcAddress("glUniform3fARB");
    glUniform2fARB = (PFNGLUNIFORM2FARBPROC)
        glXGetProcAddress("glUniform2fARB");
    glUniform4fvARB = (PFNGLUNIFORM4FVARBPROC)
        glXGetProcAddress("glUniform4fvARB");
    glUniform1fARB = (PFNGLUNIFORM1FARBPROC)
        glXGetProcAddress("glUniform1fARB");
    glUniform1fvARB = (PFNGLUNIFORM1FVARBPROC)
        glXGetProcAddress("glUniform1fvARB");

}

#endif



//-----------------------------------
// some globals
#define DESIRED_FPS 60.0

SDL_Surface* gDrawSurface = NULL;
int width = 1024;
int height = 768;
bool switch_shader=false;

//Object Handles
int venus_obj;
int floor_obj;

//GL

GLhandleARB v,f,p;
float lpos[4] = {0,0,4,0};
float a=0.0;

//-----------------------------------
// Function prototypes
void InfLoop();
void SetupSDL();

//void setGlut(int argc,char** argv);
void setShaders();
void setColorShader();
void drawScene();
void LoadObjs();
void drawFloor();
void setStripesShader();

GLint getUniLoc(GLhandleARB p,const GLcharARB *name)
{
	GLint loc;
	
	loc = glGetUniformLocationARB(p,name);

	if(loc==-1)
		printf("No such uniform named \"%s\"\n",name);

//	printOpenGLError();
	return loc;
}



//-----------------------------------
int main(int argc, char** argv)
{
    SetupSDL();
    SetupGLSLProcs();
    LoadObjs();
//    drawFloor();
    InfLoop();
    return 0;
}


//-----------------------------------
// Setup SDL and OpenGL
void SetupSDL(void)
{
    // init video system
    const SDL_VideoInfo* vidinfo;
    if( SDL_Init(SDL_INIT_VIDEO) < 0 )
    {
        fprintf(stderr,"Failed to initialize SDL Video!\n");
        exit(1);
    }

    // tell system which funciton to process when exit() call is made
    atexit(SDL_Quit);

    // get optimal video settings
    vidinfo = SDL_GetVideoInfo();
    if(!vidinfo)
    {
        fprintf(stderr,"Coudn't get video information!\n%s\n", SDL_GetError());
        exit(1);
    }

    // set opengl attributes
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE,        5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,      5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,       5);
#ifdef __APPLE__
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,      32);
#else
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,      16);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,    1);

    // get a framebuffer
    gDrawSurface = SDL_SetVideoMode(width,height,vidinfo->vfmt->BitsPerPixel,
        SDL_OPENGL);

    if( !gDrawSurface )
    {
        fprintf(stderr,"Couldn't set video mode!\n%s\n", SDL_GetError());
        exit(1);
    }

	SDL_WM_SetCaption("Brick Shader - GLSL", "MAXIMIZE ME");
//	SDL_WM_GetCaption(&titlestring, &iconstring);
//	if(titlestring) {
//		printf("Got title string: %s\n", titlestring);
//	}

//	if(iconstring) {
//		printf("Got icon string: %s\n", iconstring);
//	}


    // set opengl viewport and perspective view
    glViewport(0,0,width,height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	
  //  glOrtho( -1.0, 1.0, -1.0, 1.0, 5.0, 15.0 );

 //   glFrustum( -1.0, 1.0, -1.0, 1.0, 2.0, 10.0 );
	 //glOrtho( -1.0, 1.0, -1.0, 1.0, 5.0, 15.0 );
    //gluPerspective( 90.0, 1.0, 0.1, 20.0 );
	
  //    gluPerspective(120, 4.0f / 3.0f, .0001, 100);

glFrustum (-1.0, 1.0, -1.0, 1.0, /* transformation */
                    1.5, 20.0); 

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

//	gluLookAt(2.0,2.0,2.0,
//             1.0,1.0,1.0,
//              0.0f,1.0f,0.0f);


//nearly right.. can do better

//	gluLookAt(1.2,0.8,3.2,
//             0.0,0.7,0.0,
//              0.0f,1.0f,0.0f);


//	gluLookAt(1.8,1.8,1.8,
//             0.0,0.7,0.0,
//              0.0f,1.0f,0.0f);

	glLightfv(GL_LIGHT0, GL_POSITION,lpos);			 		// Position The Light
	glEnable(GL_LIGHT0);							// Enable Light One
	glEnable(GL_LIGHTING);							// Enable Light One



	//more .. GL settings

	glClearColor(0.0f,0.0f,0.0f,0.5f);
	glClearDepth(1.0f);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_CULL_FACE);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

         
    glLoadIdentity();	
    glTranslatef (0.0, -1.0, -5.0); /* viewing transformation */
    glScalef (1.0, 1.0, 1.0);      /* modeling transformation */
    glRotatef (20,1,0,0);	

}


char *textFileRead(char *fn) {


        FILE *fp;
        char *content = NULL;

        int f,count;
        f = open(fn, O_RDONLY);

        count = lseek(f, 0, SEEK_END);

        close(f);

        if (fn != NULL) {
                fp = fopen(fn,"rt");

                if (fp != NULL) {


                        if (count > 0) {
                                content = (char *)malloc(sizeof(char) * (count+1));
                                count = fread(content,sizeof(char),count,fp);
                                content[count] = '\0';
                        }
                        fclose(fp);
                }
        }
        return content;
}            

int textFileWrite(char *fn, char *s) {

        FILE *fp;
        int status = 0;

        if (fn != NULL) {
                fp = fopen(fn,"w");

                if (fp != NULL) {
                        
                        if (fwrite(s,sizeof(char),strlen(s),fp) == strlen(s))
                                status = 1;
                        fclose(fp);
                }
        }
        return(status);
}   



void setShaders() {

	char *vs,*fs;

	v = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	f = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

	vs = textFileRead("./brick.vert");
	fs = textFileRead("./brick.frag");

	const char * ff = fs;
	const char * vv = vs;

	glShaderSourceARB(v, 1, &vv,NULL);
	glShaderSourceARB(f, 1, &ff,NULL);

	free(vs);free(fs);

	glCompileShaderARB(v);
	glCompileShaderARB(f);

	p = glCreateProgramObjectARB();
	
	glAttachObjectARB(p,f);
	glAttachObjectARB(p,v);

	glLinkProgramARB(p);
	glUseProgramObjectARB(p);

	glUniform3fARB(getUniLoc(p,"BrickColor"),1.0,0.3,0.2);
	glUniform3fARB(getUniLoc(p,"MortarColor"),0.85,0.86,0.84);
	glUniform3fARB(getUniLoc(p,"BrickSize"),0.30,0.15,0.30);
	glUniform3fARB(getUniLoc(p,"BrickPct"),0.90,0.85,0.90);
//	glUniform3fARB(getUniLoc(p,"MortarPct"),0.10,0.15,0.10);
	glUniform3fARB(getUniLoc(p,"LightPosition"),0.0,0.0,4.0);


}

void setColorShader() {

	char *vs,*fs;

	v = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	f = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

	vs = textFileRead("./blue.vert");
	fs = textFileRead("./blue.frag");

	const char * ff = fs;
	const char * vv = vs;

	glShaderSourceARB(v, 1, &vv,NULL);
	glShaderSourceARB(f, 1, &ff,NULL);

	free(vs);free(fs);

	glCompileShaderARB(v);
	glCompileShaderARB(f);

	p = glCreateProgramObjectARB();
	
	glAttachObjectARB(p,f);
	glAttachObjectARB(p,v);

	glLinkProgramARB(p);
	glUseProgramObjectARB(p);

}

void setToonShader()
{
	char *vs,*fs;

	v = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	f = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

	vs = textFileRead("./toon.vert");
	fs = textFileRead("./toon.frag");

	const char * ff = fs;
	const char * vv = vs;

	glShaderSourceARB(v, 1, &vv,NULL);
	glShaderSourceARB(f, 1, &ff,NULL);

	free(vs);free(fs);

	glCompileShaderARB(v);
	glCompileShaderARB(f);

	p = glCreateProgramObjectARB();
	
	glAttachObjectARB(p,f);
	glAttachObjectARB(p,v);

	glLinkProgramARB(p);
	glUseProgramObjectARB(p);

        GLint loc1,loc2,loc3,loc4;
        float specIntensity = 0.98;
        float sc[4] = {0.8,0.8,0.8,1.0};
        float threshold[2] = {0.5,0.25};
        float colors[12] = {0.4,0.4,0.8,1.0,
                        0.2,0.2,0.4,1.0,
                        0.1,0.1,0.1,1.0};

        loc1 = glGetUniformLocationARB(p,"specIntensity");
        glUniform1fARB(loc1,specIntensity);

        loc2 = glGetUniformLocationARB(p,"specColor");
        glUniform4fvARB(loc2,1,sc);
//      glUniform4fARB(loc2,sc[0],sc[1],sc[2],sc[3]);
        loc3 = glGetUniformLocationARB(p,"t");
        glUniform1fvARB(loc3,2,threshold);

        loc4 = glGetUniformLocationARB(p,"colors");
        glUniform4fvARB(loc4,3,colors);
}

void setStripesShader()
{
	char *vs,*fs;

	v = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	f = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

	vs = textFileRead("./stripes.vert");
	fs = textFileRead("./stripes.frag");

	const char * ff = fs;
	const char * vv = vs;

	glShaderSourceARB(v, 1, &vv,NULL);
	glShaderSourceARB(f, 1, &ff,NULL);

	free(vs);free(fs);

	glCompileShaderARB(v);
	glCompileShaderARB(f);

	p = glCreateProgramObjectARB();
	
	glAttachObjectARB(p,f);
	glAttachObjectARB(p,v);

	glLinkProgramARB(p);
	glUseProgramObjectARB(p);

	glUniform3fARB(getUniLoc(p,"LightPosition"),0.0,0.0,4.0);
	glUniform3fARB(getUniLoc(p,"LightColor"),1.0,1.0,1.0);
	glUniform3fARB(getUniLoc(p,"EyePosition"),0.0,0.0,2.0);
	glUniform3fARB(getUniLoc(p,"Specular"),0.6,0.8,0.6);
	glUniform3fARB(getUniLoc(p,"Ambient"),0.2,0.2,0.2);
	glUniform1fARB(getUniLoc(p,"Kd"),0.0001);
	glUniform3fARB(getUniLoc(p,"StripeColor"),1.0,1.0,1.0);
	glUniform3fARB(getUniLoc(p,"BackColor"),0.8,0.2,0.3);
	glUniform1fARB(getUniLoc(p,"Width"),16.0);
	glUniform1fARB(getUniLoc(p,"Fuzz"),0.1);
	glUniform1fARB(getUniLoc(p,"Scale"),10);	

}


void drawDagger() {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  
	glLoadIdentity();	
	glTranslatef (0.0, 0.0, -6.0); /* viewing transformation */
	glScalef (3.0, 3.0, 3.0);      /* modeling transformation */

	obj_draw_file(venus_obj);
}


void drawScene() {

glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  

// Draw Bricks
glPushMatrix(); 
   setShaders();      			       
    glRotatef(a,0.0,1.0,0.0);

glBegin(GL_QUADS);		// Draw The Cube Using quads
    
    glNormal3f( 0.0f, 1.0f, 0.0f);	// Normal Pointing Up
//    glColor3f(0.0f,1.0f,0.0f);	// Color Green
    glVertex3f( 1.0f, 1.0f,-1.0f);	// Top Right Of The Quad (Top)
    glVertex3f(-1.0f, 1.0f,-1.0f);	// Top Left Of The Quad (Top)
    glVertex3f(-1.0f, 1.0f, 1.0f);	// Bottom Left Of The Quad (Top)
    glVertex3f( 1.0f, 1.0f, 1.0f);	// Bottom Right Of The Quad (Top)

    
    glNormal3f( 0.0f,-1.0f, 0.0f);	// Normal Pointing Down
    //glColor3f(1.0f,0.5f,0.0f);	// Color Orange
    glVertex3f( 1.0f,-1.0f, 1.0f);	// Top Right Of The Quad (Bottom)
    glVertex3f(-1.0f,-1.0f, 1.0f);	// Top Left Of The Quad (Bottom)
    glVertex3f(-1.0f,-1.0f,-1.0f);	// Bottom Left Of The Quad (Bottom)
    glVertex3f( 1.0f,-1.0f,-1.0f);	// Bottom Right Of The Quad (Bottom)
	
  
    glNormal3f( 0.0f, 0.0f, 1.0f);	// Normal Pointing Towards The Viewer
    //glColor3f(1.0f,0.0f,0.0f);	// Color Red	
    glVertex3f( 1.0f, 1.0f, 1.0f);	// Top Right Of The Quad (Front)
    glVertex3f(-1.0f, 1.0f, 1.0f);	// Top Left Of The Quad (Front)
    glVertex3f(-1.0f,-1.0f, 1.0f);	// Bottom Left Of The Quad (Front)
    glVertex3f( 1.0f,-1.0f, 1.0f);	// Bottom Right Of The Quad (Front)

    
    glNormal3f( 0.0f, 0.0f,-1.0f);	// Normal Pointing Away From Viewer
    //glColor3f(1.0f,1.0f,0.0f);	// Color Yellow
    glVertex3f( 1.0f,-1.0f,-1.0f);	// Top Right Of The Quad (Back)
    glVertex3f(-1.0f,-1.0f,-1.0f);	// Top Left Of The Quad (Back)
    glVertex3f(-1.0f, 1.0f,-1.0f);	// Bottom Left Of The Quad (Back)
    glVertex3f( 1.0f, 1.0f,-1.0f);	// Bottom Right Of The Quad (Back)

    
    glNormal3f(-1.0f, 0.0f, 0.0f);	// Normal Pointing Left
    //glColor3f(0.0f,0.0f,1.0f);	// Color Blue
    glVertex3f(-1.0f, 1.0f, 1.0f);	// Top Right Of The Quad (Left)
    glVertex3f(-1.0f, 1.0f,-1.0f);	// Top Left Of The Quad (Left)
    glVertex3f(-1.0f,-1.0f,-1.0f);	// Bottom Left Of The Quad (Left)
    glVertex3f(-1.0f,-1.0f, 1.0f);	// Bottom Right Of The Quad (Left)
    
    glNormal3f( 1.0f, 0.0f, 0.0f);	// Normal Pointing Right
    //glColor3f(1.0f,0.0f,1.0f);	// Color Violet
    glVertex3f( 1.0f, 1.0f,-1.0f);	// Top Right Of The Quad (Right)
    glVertex3f( 1.0f, 1.0f, 1.0f);	// Top Left Of The Quad (Right)
    glVertex3f( 1.0f,-1.0f, 1.0f);	// Bottom Left Of The Quad (Right)
    glVertex3f( 1.0f,-1.0f,-1.0f);	// Bottom Right Of The Quad (Right)
  glEnd();			// End Drawing The Cube



a+=3;

glPopMatrix();

glPushMatrix();
// Draw Cup
if(switch_shader==true)
setToonShader();
else
setStripesShader();

glRotatef(a,0.0,1.0,0.0);

//if(a <-2) a=0;
obj_draw_file(venus_obj);

a+=2;
glPopMatrix();

glPushMatrix();
//Draw Floor
setStripesShader();
obj_draw_file(floor_obj);
glPopMatrix();

SDL_GL_SwapBuffers();


}


void drawFloor()
{

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  

  setShaders();
  obj_draw_file(floor_obj);

  SDL_GL_SwapBuffers();
}


void drawCup()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  
   
SDL_GL_SwapBuffers();
}

//-----------------------------------
// Infinite Loop
void InfLoop()
{
    //--- infinite loop with event queue processing
    SDL_Event event;
    while(1)
    {

//      a=0.0;
      drawScene();

       while( SDL_PollEvent( &event ))
        {
            switch( event.type )
            {
            case SDL_KEYDOWN:
            printf("The %s key was pressed!\n",
                   SDL_GetKeyName(event.key.keysym.sym));

//Press Up arrow to move light in Y direction
            if(event.key.keysym.sym==SDLK_UP)
		{
                lpos[1]+=1.0;
               	glLightfv(GL_LIGHT0, GL_POSITION,lpos);			 		// Position The Light
                printf("The light has been respositioned to: %f %f %f",lpos[0],lpos[1],lpos[2]);
                }
//Press Down arrow to move light in -Y direction
            if(event.key.keysym.sym==SDLK_DOWN)
		{
                lpos[1]-=1.0;
               	glLightfv(GL_LIGHT0, GL_POSITION,lpos);			 		// Position The Light
                printf("The light has been respositioned to: %f %f %f",lpos[0],lpos[1],lpos[2]);
                }
//Press right arrow to move light in X direction
            if(event.key.keysym.sym==SDLK_RIGHT)
		{
                lpos[0]+=1.0;
               	glLightfv(GL_LIGHT0, GL_POSITION,lpos);			 		// Position The Light
                printf("The light has been respositioned to: %f %f %f",lpos[0],lpos[1],lpos[2]);
                }
//Press left arrow to move light in -X direction
            if(event.key.keysym.sym==SDLK_LEFT)
		{
                lpos[0]-=1.0;
               	glLightfv(GL_LIGHT0, GL_POSITION,lpos);			 		// Position The Light
                printf("The light has been respositioned to: %f %f %f",lpos[0],lpos[1],lpos[2]);
                }
//Press page up key to move light in Z direction
            if(event.key.keysym.sym==SDLK_PAGEUP)
		{
                lpos[2]+=1.0;
               	glLightfv(GL_LIGHT0, GL_POSITION,lpos);			 		// Position The Light
                printf("The light has been respositioned to: %f %f %f",lpos[0],lpos[1],lpos[2]);
                }
//Press page down key to move light in -Z direction
            if(event.key.keysym.sym==SDLK_PAGEDOWN)
		{
                lpos[2]-=1.0;
               	glLightfv(GL_LIGHT0, GL_POSITION,lpos);			 		// Position The Light
                printf("The light has been respositioned to: %f %f %f",lpos[0],lpos[1],lpos[2]);
                }
            if(event.key.keysym.sym==SDLK_COMMA)
		{
		switch_shader=true;
		}
               if(event.key.keysym.sym==SDLK_PERIOD)
		{
		switch_shader=false;
		}
	    break;
	

	    case SDL_MOUSEMOTION:
                printf("Mouse moved by %d,%d to (%d,%d)\n", 
                       event.motion.xrel, event.motion.yrel,
                       event.motion.x, event.motion.y);
            break;   

            case SDL_MOUSEBUTTONDOWN:
                printf("Mouse button %d pressed at (%d,%d)\n",
                      event.button.button, event.button.x, event.button.y);
            break;

            
           
	    case SDL_QUIT:
                exit(0);
            break;
	    }
        }
     } // -- while event in queue

    } // -- infinite loop

//-------------------------------------------------------------------


void LoadObjs()
{
	venus_obj = obj_add_file("venus.obj");
        floor_obj = obj_add_file("floor.obj");
}
