uniform sampler2D colorMap;
uniform sampler2DShadow shadowmap;
varying vec4 shadowTexcoord;
//varying vec4 diffuse;
void main()
{
    const float epsilon = 0.5;

	/*float factor = 1.0;
	float depth = shadow2DProj(shadowmap, shadowTexcoord).r + epsilon;
	depth = clamp(depth, 0.0, 1.0);
	if(depth != 1.0)factor = 0.25;*/
	
	float factor = 0.0;
    float x,y;
    float xPixelOffset = 0.0005;
    float yPixelOffset = 0.0005;
	for (y = -1.5 ; y <=1.5 ; y+=1.0)
			for (x = -1.5 ; x <=1.5 ; x+=1.0)
			factor += shadow2DProj(shadowmap, shadowTexcoord + vec4(x * xPixelOffset * shadowTexcoord.w, y * yPixelOffset * shadowTexcoord.w, 0.01, 0.0) ).r;;
	
	factor /= 16.0 ;


	if(shadowTexcoord.x >= 0.0 && shadowTexcoord.y >= 0.0 
	&& shadowTexcoord.x <= 1.0 && shadowTexcoord.y <= 1.0 )
	{
	gl_FragColor = texture(colorMap, gl_TexCoord[0].st) * factor;//vec4(gl_Color.rgb* diffuse.rgb * factor, 1.0);
	}
	else
	{
	gl_FragColor = texture(colorMap, gl_TexCoord[0].st);//vec4(gl_Color.rgb* diffuse.rgb, 1.0);
	}
}

