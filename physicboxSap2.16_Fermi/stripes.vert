uniform vec3 LightPosition;
uniform vec3 LightColor;
uniform vec3 EyePosition;
uniform vec3 Specular;
uniform vec3 Ambient;
uniform float Kd;

varying vec3 DiffuseColor;
varying vec3 SpecularColor;

void main(void)
{
	vec3 ecPosition = vec3 (gl_ModelViewMatrix * gl_Vertex);
	vec3 tnorm	= normalize (gl_NormalMatrix * gl_Normal);
	vec3 lightVec   = normalize (LightPosition - ecPosition);
	vec3 viewVec	= normalize (EyePosition - ecPosition);
	vec3 hVec	= normalize (viewVec + lightVec);

	float spec = clamp(dot(hVec, tnorm), 0.0, 1.0);
	spec = pow(spec, 16.0);

	DiffuseColor	= LightColor * vec3 (Kd * dot(lightVec, tnorm));
	DiffuseColor	= clamp(Ambient + DiffuseColor, 0.0, 1.0);
	SpecularColor	= clamp((LightColor * Specular * spec), 0.0, 1.0);

	gl_TexCoord[0]	= gl_MultiTexCoord0;
	gl_Position	= ftransform();
}
