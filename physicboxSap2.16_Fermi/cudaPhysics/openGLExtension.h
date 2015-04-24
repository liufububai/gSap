void checkGLExtensions(){
	GLenum err = glewInit();
	if (GLEW_OK != err){
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
	}
//	fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

	const char *extensions = (const char*)glGetString(GL_EXTENSIONS);
	const char *version = (const char*)glGetString(GL_VERSION);

//	cout << version << endl;

	if (wglSwapIntervalEXT)
		wglSwapIntervalEXT(0); 
}
