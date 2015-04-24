uniform vec3  BrickColor, MortarColor;
uniform vec3  BrickSize;
uniform vec3  BrickPct;
//uniform vec3  MortarPct;

varying vec3  MCposition;
varying float LightIntensity;

//#define Integral(x, p, notp) ((floor(x)*(p)) + max(fract(x) - (notp), 0.0))

void main(void)
{
    vec3  color;
    vec3  position, useBrick;
    vec2 t,fw;
    vec2 add1,add2;
    float count=0.0;
   
    add1 = vec2(0.0,0.0);
    add1 = vec2(0.0,0.0);
    
    position = MCposition / BrickSize;

    if (position.y == 0.0)
      color = MortarColor;
else
{
    if (fract(position.y * 0.5) > 0.5)
       position.x += 0.5;


    if (fract(position.y * 0.5) < 0.5)
       position.z += 0.5;

//	 fw = fwidth(position.xy);
//	 useBrick.xy = (Integral(position.xy + fw, BrickPct.xy, MortarPct.xy) -
//		        Integral(position.xy, BrickPct.xy, MortarPct.xy) ) / fw;

//	 fw = fwidth(position.yz);
//	 useBrick.yz = (Integral(position.yz + fw, BrickPct.yz, MortarPct.yz) -
// 		        Integral(position.yz, BrickPct.yz, MortarPct.yz) ) / fw;



    position.xy = fract(position.xy);
    position.yz = fract(position.yz);

//    t = clamp ((BrickPct.xy - position.xy) / (1.0 - position.xy), 1.0, 0.0);  

//      useBrick.xy = t * t * (3.0 - 2.0 * t);

//    t = clamp ((BrickPct.yz - position.yz) / (1.0 - position.yz), 1.0, 0.0);  

//      useBrick.yz = t * t * (3.0 - 2.0 * t);


//    useBrick.xy = smoothstep(BrickPct.xy,BrickSize.xy,position.xy);
//    useBrick.yz = smoothstep(BrickPct.yz,BrickSize.yz,position.yz);

//    useBrick.xy = step(position.xy, BrickPct.xy);
//    useBrick.yz = step(position.yz, BrickPct.yz);
    
    useBrick.xy = smoothstep(position.xy-0.15, position.xy+0.15, BrickPct.xy);
    useBrick.yz = smoothstep(position.yz-0.15, position.yz+0.15, BrickPct.yz);

    color  = mix(MortarColor, BrickColor, useBrick.x * useBrick.y * useBrick.z);

}

 
    color *= LightIntensity;
    gl_FragColor = vec4 (color, 1.0);
}
