#define MAX_BUFFERS 4

const GLenum colorAttachment[] = {
  GL_COLOR_ATTACHMENT0_EXT,
  GL_COLOR_ATTACHMENT1_EXT,
  GL_COLOR_ATTACHMENT2_EXT,
  GL_COLOR_ATTACHMENT3_EXT
};


class FBO{
//	FRAME BUFFER OBJECTS
	GLuint fb,depth_rb,stencil_rb;
public:
//	TEXTURES
	GLuint tex[MAX_BUFFERS]; 
	GLuint shadowMapDepthTexture;
//	GLuint stencilTexture;

private:
	GLenum texKind[MAX_BUFFERS];
	int fboWidth,fboHeight;

public:
	void init(int fw,int fh);
	void checkFramebufferStatus();
	void activate();
	void draw(int index);
	void inactivate();
	void bind(int index);
	int getWidth();
	int getHeight();
	void deleteBuffer();
	void createTexture(int index
				   ,GLint internalFormat,GLenum texTarget,GLenum format,GLenum type
				   ,GLint filter,bool ifUseStencil);

};

void FBO::checkFramebufferStatus(){
    GLenum status;
    status = (GLenum) glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    switch(status) {
        case GL_FRAMEBUFFER_COMPLETE_EXT:
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
            printf("Unsupported framebuffer format\n");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
            printf("Framebuffer incomplete, missing attachment\n");
            break;
//        case GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT:
//            printf("Framebuffer incomplete, duplicate attachment\n");
//            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
            printf("Framebuffer incomplete, attached images must have same dimensions\n");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
            printf("Framebuffer incomplete, attached images must have same format\n");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
            printf("Framebuffer incomplete, missing draw buffer\n");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
            printf("Framebuffer incomplete, missing read buffer\n");
            break;
//        default:
 //           assert(0);
    }
}
void FBO::activate(){
    glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fb);
/*	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1,1,-1,1,-100,100);
	glMatrixMode(GL_MODELVIEW);
*/
}
void FBO::draw(int index){
	glDrawBuffer(colorAttachment[index]);
	glReadBuffer(colorAttachment[index]);
	glViewport(0, 0, fboWidth,fboHeight);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
}
void FBO::inactivate(){
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
}
void FBO::bind(int index){
	glBindTexture(texKind[index],tex[index]);
}
int FBO::getWidth(){
	return fboWidth;
}
int FBO::getHeight(){
	return fboHeight;
}
void FBO::init(int fw,int fh){
	glGenFramebuffersEXT(1,&fb);
	glGenRenderbuffersEXT(1,&depth_rb);
	glGenRenderbuffersEXT(1,&stencil_rb);
	for(int i=0;i<MAX_BUFFERS;i++)
		glGenTextures(1,&(tex[i]));
	glGenTextures(1, &shadowMapDepthTexture);
//	glGenTextures(1, &stencilTexture);
	fboWidth=fw;fboHeight=fh;
}
void FBO::deleteBuffer(){
	for(int i=0;i<MAX_BUFFERS;i++)
		glDeleteTextures( 1, &(tex[i]) );
    glDeleteRenderbuffersEXT( 1, &depth_rb );
    glDeleteFramebuffersEXT( 1, &fb );
}
void FBO::createTexture(int index
				   ,GLint internalFormat,GLenum texTarget,GLenum format,GLenum type
				   ,GLint filter,bool ifUseStencil){
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fb);

	glBindTexture(texTarget,tex[index]);
	glTexImage2D(texTarget,0,internalFormat,fboWidth,fboHeight,0,format,type,NULL);
	glTexParameteri(texTarget,GL_TEXTURE_MIN_FILTER,filter);
	glTexParameteri(texTarget,GL_TEXTURE_MAG_FILTER,filter);
	glTexParameterf(texTarget,GL_TEXTURE_WRAP_S,GL_CLAMP);//_TO_EDGE_EXT);
	glTexParameterf(texTarget,GL_TEXTURE_WRAP_T,GL_CLAMP);//_TO_EDGE_EXT);

	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,colorAttachment[index],texTarget,tex[index],0);
	texKind[index]=texTarget;


//=======

	if(ifUseStencil){
		glBindTexture(GL_TEXTURE_2D, shadowMapDepthTexture);
		glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH24_STENCIL8_EXT,fboWidth,fboHeight,0,GL_DEPTH_STENCIL_EXT,GL_UNSIGNED_INT_24_8_EXT,NULL);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filter);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filter);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
//	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_MODE_ARB,GL_COMPARE_R_TO_TEXTURE_ARB);
//	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_TEXTURE_2D,shadowMapDepthTexture,0);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_STENCIL_ATTACHMENT_EXT,GL_TEXTURE_2D,shadowMapDepthTexture,0);
	}

//  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depth_rb);
//  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, fboWidth,fboHeight);

//  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT, shadowMapDepthTexture);

/*
	if(ifUseStencil){
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, stencil_rb);
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,
			GL_STENCIL_INDEX, fboWidth,fboHeight);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
			GL_STENCIL_ATTACHMENT_EXT,
			GL_RENDERBUFFER_EXT, stencil_rb);
	}
*/

/*
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depth_rb);
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, fboWidth,fboHeight);
  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                 GL_RENDERBUFFER_EXT, depth_rb);
*/
/*
	glBindTexture(GL_TEXTURE_2D, shadowMapDepthTexture);
	glTexImage2D(
		GL_TEXTURE_2D,				// Target
		0,							// Mip-level
		GL_DEPTH_COMPONENT24,		// InternalFormat
		fboWidth,				// width size
		fboHeight,			// height size
		0,							// border
		GL_DEPTH_COMPONENT,			// input pixel format
		GL_UNSIGNED_INT,			// input pixel type
		NULL);						// input pixels
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filter);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filter);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
//	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_MODE_ARB,GL_COMPARE_R_TO_TEXTURE_ARB);
//	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);

//	glGenFramebuffersEXT(1, &shadowMapFBODepth);
//	glBindFramebufferEXT(GL_RENDERBUFFER_EXT, depth_rb);

	glFramebufferTexture2DEXT(
		GL_FRAMEBUFFER_EXT,
		GL_DEPTH_ATTACHMENT_EXT,
		GL_TEXTURE_2D,
		shadowMapDepthTexture,
		0);
*/
    checkFramebufferStatus();

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}
