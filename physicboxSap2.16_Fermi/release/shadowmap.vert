#version 120
#extension GL_EXT_gpu_shader4 : enable
uniform sampler2D posTex;
uniform sampler2D quatTex;
const float PI = 3.1415926;
void main(void)
{
	vec4 centerOfMass;
	vec2 texCrd;
	texCrd = vec2((gl_InstanceID/128)/128.0, (gl_InstanceID-gl_InstanceID/128*128)/128.0);
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
	gl_FrontColor  = gl_Color;
	//===== LIGHTING =====
	/*vec3 lightDir = vec3(1.0, 1.0, 1.0);
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
	gl_FrontColor = outColor * dot(normalize(inNormal.xyz), normalize(lightDir));*/
}
