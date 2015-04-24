uniform sampler2D shadowmap;
varying vec3 norm;
varying vec4 shadowTexcoord;
varying vec4 diffuse;
void main()
{
	const float epsilon = 0.5;

    shadowTexcoord.z += 0.0005;
	float factor = 1.0;
	//float depth = shadow2DProj(shadowmap, shadowTexcoord).r + epsilon;
	float depth = texture2D(shadowmap, shadowTexcoord.st).z;
	//depth = clamp(depth, 0.0, 1.0);
        //if(depth != 1.0)factor = 0.25;
        
    factor = depth < shadowTexcoord.z ? 0.5 : 1.0;
    /*float factor = 0.0;
    float x,y;
    float xPixelOffset = 0.0005;
    float yPixelOffset = 0.0005;
	for (y = -1.5 ; y <=1.5 ; y+=1.0)
			for (x = -1.5 ; x <=1.5 ; x+=1.0)
			factor += shadow2DProj(shadowmap, shadowTexcoord + vec4(x * xPixelOffset * shadowTexcoord.w, y * yPixelOffset * shadowTexcoord.w, 0.00, 0.0) ).r;;
	
	factor /= 16.0 ;*/
	
	if(shadowTexcoord.x >= 0.0 && shadowTexcoord.y >= 0.0 
	&& shadowTexcoord.x <= 1.0 && shadowTexcoord.y <= 1.0 )
   	{
   	    gl_FragColor = vec4(gl_Color.rgb * (factor+0.5), 1.0);
		//gl_FragColor = vec4(gl_Color.rgb* diffuse.rgb * factor, 1.0);
	}
	else
	{
	    gl_FragColor = vec4(gl_Color.rgb, 1.0);
		//gl_FragColor = vec4(gl_Color.rgb* diffuse.rgb, 1.0);
	}
}
