#ifndef _MATRIX16
#define _MATRIX16


class CVector4D 
{
public:
  float x, y, z, w;

  inline CVector4D(void) {};
  inline CVector4D(float vx, float vy, float vz, float vw) {
    x = vx; 
    y = vy; 
    z = vz;
	w = vw;
  };
  inline void set(float vx, float vy, float vz, float vw) {
    x = vx;   // Setʸֵ
    y = vy; 
    z = vz; 
	w = vw;
  };
};

class CMatrix16
{ 
public:
	float mt[16];
	CMatrix16::CMatrix16(){ memset(mt, 0, 16*sizeof(float)); 
	                        mt[0] = mt[5] = mt[10] = mt[15] = 1.0f; };

	CMatrix16::CMatrix16(float m0, float m1, float m2, float m3,
				    	float m4, float m5, float m6, float m7,
				    	float m8, float m9, float m10, float m11,
				    	float m12, float m13, float m14, float m15)
	{
		mt[0]=m0;   mt[1]=m1;   mt[2]=m2;   mt[3]=m3;
		mt[4]=m4;   mt[5]=m5;   mt[6]=m6;   mt[7]=m7;
		mt[8]=m8;   mt[9]=m9;   mt[10]=m10; mt[11]=m11;
		mt[12]=m12; mt[13]=m13; mt[14]=m14; mt[15]=m15;	
	}

   inline CVector4D operator*(CVector4D & vec4) { 
	return CVector4D(mt[0] * vec4.x + mt[4] * vec4.y + mt[8] * vec4.z + mt[12] * vec4.w ,
					 mt[1] * vec4.x + mt[5] * vec4.y + mt[9] * vec4.z + mt[13] * vec4.w ,
					 mt[2] * vec4.x + mt[6] * vec4.y + mt[10]* vec4.z + mt[14] * vec4.w ,
					 mt[3] * vec4.x + mt[7] * vec4.y + mt[11]* vec4.z + mt[15] * vec4.w );		 

   };


   inline CMatrix16 GetmvMatrixInverse(){
         CMatrix16 temp;
         
	temp.mt[0]  = mt[0]; temp.mt[1] = mt[4]; temp.mt[2]  = mt[8];
	temp.mt[4]  = mt[1]; temp.mt[5] = mt[5]; temp.mt[6]  = mt[9];
	temp.mt[8]  = mt[2]; temp.mt[9] = mt[6]; temp.mt[10] = mt[10];
	temp.mt[3]  = 0.0f; temp.mt[7] = 0.0f; temp.mt[11] = 0.0f;
	temp.mt[15] = 1.0f;

	temp.mt[12] = -(mt[12] * mt[0]) - (mt[13] * mt[1]) - (mt[14] * mt[2]);
	temp.mt[13] = -(mt[12] * mt[4]) - (mt[13] * mt[5]) - (mt[14] * mt[6]);
	temp.mt[14] = -(mt[12] * mt[8]) - (mt[13] * mt[9]) - (mt[14] * mt[10]);
	return temp;
   };



};



#endif 
