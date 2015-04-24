uniform vec3 LightPosition;

const float SpecularContribution = 0.3;
const float DiffuseContribution  = 1.0 - SpecularContribution;

varying float LightIntensity;
varying vec3  MCposition;

void main(void)
{
    vec3 ecPosition = vec3 (gl_ModelViewMatrix * gl_Vertex);
    vec3 tnorm      = normalize(gl_NormalMatrix * gl_Normal);
    vec3 lightVec   = normalize(LightPosition - ecPosition);
    vec3 reflectVec = reflect(-lightVec, tnorm);
    vec3 viewVec    = normalize(-ecPosition);
    float diffuse   = max(dot(lightVec, tnorm), 0.0);
    float spec      = 0.0;

    if (diffuse > 0.0)
    {
        spec = max(dot(reflectVec, viewVec), 0.0);
        spec = pow(spec, 16.0);
    }

    LightIntensity  = DiffuseContribution * diffuse +
                      SpecularContribution * spec;

    MCposition      = gl_Vertex.xyz;

//fill mortar over the top

    if(gl_Normal.y == 1.0 && gl_Normal.x == 0.0 && gl_Normal.z == 0.0)	
	MCposition.y = 0.0;
    
    if(gl_Normal.y == -1.0 && gl_Normal.x == 0.0 && gl_Normal.z == 0.0)	
	MCposition.y = 0.0;
   
//  || gl_Normal.x == 1.0 || gl_Normal.z == 1.0 || gl_Normal.z == -1.0)

//    if(gl_Normal.x == -1.0 || gl_Normal.x == 1.0 || gl_Normal.z == 1.0 || gl_Normal.z == -1.0)	
//      {
//	if(gl_Vertex.y < 0.15)
//        MCposition.y = 0.0;
//       }
      

    gl_Position     = ftransform();
}
