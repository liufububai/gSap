 /*#define STRINGIFY(A) #A

// vertex shader
const char *vertexShader = STRINGIFY(
#version 120
//#extension GL_EXT_bindable_uniform : enable
#extension GL_EXT_gpu_shader4 : enable

const int rigidBodySize = 128;
const float PI = 3.14159276;

//==== QUATERNION ====
float4 mulQuaternion(float4 a,float4 b){
	float4 ans;
	ans.xyz=cross(a.xyz,b.xyz);
	ans.xyz+=a.w*b.xyz+b.w*a.xyz;
	ans.w=a.w*b.w-dot(a.xyz,b.xyz);
	return ans;
}

float4 invertQuaternion(float4 a){
	return float4(-a.xyz,a.w);
}

float4 normalizeQuaternion(float4 a){
	float qLength=length(a.xyzw);

	return a/qLength;
}

float3 rotateVectorWQuaternion(float4 quat,float4 inVec){
	inVec.w=0;
	float4 quatInv = invertQuaternion(quat);
	float4 rotatedPos = mulQuaternion(mulQuaternion(quat,inVec),quatInv);
	return rotatedPos.xyz;
}

float4 w2Quaternion(float3 w){
	float4 ans;
	float lw=length(w);

	ans.xyz = normalize(w);
	ans.w = lw*0.5;
	ans.xyz *= sin(ans.w);
	ans.w = cos(ans.w);

	if(lw < 0.0001)
		ans=float4(0,0,0,1);

	return ans;
}

//==== RIGID BODY ====
float2 index2TexCrd(float index){
	float2 texCrd;
	float d=1/(float)rigidBodySize*0.5;
	float tmp=index/(float)rigidBodySize;
	texCrd.x=modf(tmp,texCrd.y);
	texCrd=float2(texCrd.x+d,texCrd.y/(float)rigidBodySize+d);
	return texCrd;
}


uniform sampler2D posTex;//: TEXUNIT0;
uniform sampler2D quatTex;//: TEXUNIT1;

float4 quat={1,1,0,0.5*PI};
float3 centerMass={0.5,0,0};

void main(float4 inPosition: POSITION,
		  float4 inNormal: NORMAL,
		  float2 inTexCrd: TEXCOORD,
		  uniform  float4x4 modelViewProjMatrix,
		  uniform  float4x4 modelViewMatrix,
		  out float4 outPos: POSITION,
		  out float3 outTexCrd: TEXCOORD1,
		  out float4 outColor: COLOR0)
{

			  float4 centerOfMass;
			  float index=gl_InstanceID;//inTexCrd.x;// + o_id;
			  float2 texCrd=index2TexCrd(index);
			  centerOfMass=tex2D(posTex,texCrd);
			  quat=tex2D(quatTex,texCrd);

			  float4 rotatedPos;
			  rotatedPos.xyz=rotateVectorWQuaternion(quat,inPosition);
			  rotatedPos.w=1;

			  rotatedPos.xyz+=centerOfMass.xyz;

			  outPos=mul(modelViewProjMatrix,rotatedPos);

			  //==== LIGHTING ====
			  float3 lightDir=float3(1,1,1);
			  outColor.xyz=0.2+0.8*float3(pow(sin(inTexCrd.x/7.0f),2),
				  pow(sin(inTexCrd.x/3.0f),2),
				  pow(sin(inTexCrd.x/4.0f),2));
			  inNormal.w=0;?
			  inNormal.xyz=rotateVectorWQuaternion(quat,inNormal);
			  inNormal=mul(modelViewMatrix,inNormal);
			  outColor.xyz*=dot(normalize(inNormal.xyz),normalize(lightDir));
}
);*/
// Vertex shader.
const char * vertexShader =
    "#version 120\n"
    "#extension GL_EXT_gpu_shader4 : enable\n"
	"uniform sampler2D posTex;\n"
	"uniform sampler2D quatTex;\n"
	"const float PI = 3.1415926;\n"
    "void main(void)\n"
    "{\n"
	"     vec4 centerOfMass;\n"
	"     vec2 texCrd;\n"
	"     texCrd = vec2((gl_InstanceID/128)/128.0, (gl_InstanceID-gl_InstanceID/128*128)/128.0);\n"
	"     centerOfMass = texture2D(posTex, texCrd);\n"
	"     vec4 quat = texture2D(quatTex, texCrd);\n"//---------------------------------------------------------------------
	"     vec4 inquat = vec4(-quat.xyz, quat.w);\n"
	"     vec4 inVec = gl_Vertex;\n"
	"     if(mod(gl_InstanceID, 2) == 1) inVec = 1.2*inVec;\n"
	"     inVec.w = 0.0;\n"
	"     vec4 ans;\n"
	"     ans.xyz = cross(quat.xyz, inVec.xyz);\n"
	"     ans.xyz = ans.xyz + quat.w*inVec.xyz + inVec.w*quat.xyz;\n"
	"     ans.w = quat.w*inVec.w - dot(quat.xyz, inVec.xyz);\n"
	"     vec4 rotatedPos;\n"
	"     rotatedPos.xyz = cross(ans.xyz, inquat.xyz);\n"
	"     rotatedPos.xyz = rotatedPos.xyz + ans.w*inquat.xyz + inquat.w*ans.xyz;\n"
	"     rotatedPos.w = 1.0;\n"
	"     rotatedPos.xyz = rotatedPos.xyz + centerOfMass.xyz;\n"
    "     gl_Position = gl_ModelViewProjectionMatrix * rotatedPos;\n"
	//===== LIGHTING =====
	"     vec3 lightDir = vec3(1.0, 1.0, 1.0);\n"	
	"     vec4 outColor;\n"
	"     outColor.xyz = 0.2 + 0.8*vec3(pow(sin(texCrd.x*70.0),2), pow(sin(texCrd.x*30.0),2), pow(sin(texCrd.x*40.0),2));\n"
	"     vec4 inNormal;\n"
	"     inNormal.xyz = gl_Normal;\n"
	"     inNormal.w = 0.0;\n"
	"     ans.xyz = cross(quat.xyz, inNormal.xyz);\n"
	"     ans.xyz = ans.xyz + quat.w*inNormal.xyz + inNormal.w*quat.xyz;\n"
	"     ans.w = quat.w*inNormal.w - dot(quat.xyz, inNormal.xyz);\n"
	"     vec4 ans1;\n"
	"     ans1.xyz = cross(ans.xyz, inquat.xyz);\n"
	"     ans1.xyz = ans1.xyz + ans.w*inquat.xyz + inquat.w*ans.xyz;\n"
	"     inNormal.xyz = ans1.xyz;\n"
	"     inNormal.xyz = gl_NormalMatrix * inNormal.xyz;\n"
	"     gl_FrontColor = outColor * dot(normalize(inNormal.xyz), normalize(lightDir));\n"
    "}\n"
    "\n";