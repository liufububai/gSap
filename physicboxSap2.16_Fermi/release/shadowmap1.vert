#version 120
#extension GL_EXT_gpu_shader4 : enable
uniform sampler2D posTex;
uniform sampler2D quatTex;
const float PI = 3.1415926;
varying vec3 norm;
varying vec4 shadowTexcoord;
varying vec4 diffuse;
void main()
{
	vec4 centerOfMass;
	vec2 texCrd;
	texCrd = vec2((gl_InstanceID/64)/64.0, (gl_InstanceID-gl_InstanceID/64*64)/64.0);
	centerOfMass = texture2D(posTex, texCrd);
	vec4 quat = texture2D(quatTex, texCrd);
	vec4 inquat = vec4(-quat.xyz, quat.w);
	vec4 inVec = gl_Vertex;
	if(mod(gl_InstanceID, 2) == 1) inVec = 1.2*inVec;
	inVec.w = 0.0;
	vec4 ans;
	ans.xyz = cross(quat.xyz, inVec.xyz);
	ans.xyz = ans.xyz + quat.w*inVec.xyz + inVec.w*quat.xyz;
	ans.w = quat.w*inVec.w - dot(quat.xyz, inVec.xyz);
	vec4 rotatedPos;
	rotatedPos.xyz = cross(ans.xyz, inquat.xyz);
	rotatedPos.xyz = rotatedPos.xyz + ans.w*inquat.xyz + inquat.w*ans.xyz;
	rotatedPos.w = 1.0;
	rotatedPos.xyz = rotatedPos.xyz + centerOfMass.xyz;
	gl_Position = gl_ModelViewProjectionMatrix * rotatedPos;
	
	//vec4 Vpos = gl_ModelViewMatrix * gl_Vertex;
	//vec3 pos = Vpos.xyz / Vpos.w;

    vec4 Vpos = gl_ModelViewMatrix * rotatedPos;
    vec3 pos = Vpos.xyz / Vpos.w;
    
	vec3 lightdir = normalize( vec3(gl_LightSource[0].position) - pos);
	norm = normalize(gl_NormalMatrix * gl_Normal);

	float NdotL = max(0.0, dot(lightdir, norm));
	diffuse = gl_LightSource[0].diffuse * NdotL;

	vec4 texcoord = gl_TextureMatrix[7] * gl_ModelViewMatrix * rotatedPos;

	shadowTexcoord = texcoord / texcoord.w;
	shadowTexcoord = 0.5 * shadowTexcoord +0.5;

	gl_FrontColor  = gl_Color;
	//gl_Position = ftransform(); 
	//===== LIGHTING =====
	vec3 lightDir = normalize( vec3(gl_LightSource[0].position) - pos);
	vec4 outColor;
	outColor.xyz = 0.2 + 0.8*vec3(pow(sin(texCrd.x*70.0),2), pow(sin(texCrd.x*30.0),2), pow(sin(texCrd.x*40.0),2));
	vec4 inNormal;
	inNormal.xyz = gl_Normal;
	inNormal.w = 0.0;
	ans.xyz = cross(quat.xyz, inNormal.xyz);
	ans.xyz = ans.xyz + quat.w*inNormal.xyz + inNormal.w*quat.xyz;
	ans.w = quat.w*inNormal.w - dot(quat.xyz, inNormal.xyz);
	vec4 ans1;
	ans1.xyz = cross(ans.xyz, inquat.xyz);
	ans1.xyz = ans1.xyz + ans.w*inquat.xyz + inquat.w*ans.xyz;
	inNormal.xyz = ans1.xyz;
	inNormal.xyz = gl_NormalMatrix * inNormal.xyz;
	gl_FrontColor = outColor * dot(normalize(inNormal.xyz), normalize(lightDir));
}
