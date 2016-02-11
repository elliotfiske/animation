#version 120
attribute vec4 vertPos;
attribute vec3 vertNor;
attribute vec2 vertTex;
uniform mat4 P;
uniform mat4 MV;
varying vec3 fragNor;

// New Attributes

// How heavily each of the bones affect the vertex
attribute vec4 weights0;
attribute vec4 weights1;
attribute vec4 weights2;
attribute vec4 weights3;
attribute vec2 weights4_nah;

// Which bones affect the vertex
attribute vec4 bones0;
attribute vec4 bones1;
attribute vec4 bones2;
attribute vec4 bones3;

// How many bones affect the current vertex?
attribute float num_bones;

// The rigidtransform of all the bones
uniform mat4 BONE_POS[18];

// Mj(0)-1
uniform mat4 BIND_BONE_POS[18];

uniform int gpu_rendering;

float getWeightForNdx(int ndx) {
   if (ndx < 4) {
      return weights0[ndx];
   }
   else if (ndx < 8) {
      return weights1[ndx-4];
   }
   else if (ndx < 12) {
      return weights2[ndx-8];
   }
   else if (ndx < 16) {
      return weights3[ndx-12];
   }
   else if (ndx < 20) {
      return weights4_nah[ndx-16];
   }
}

void main()
{
   vec4 boned_pos = vec4(0, 0, 0, 0);
   
   for (int i = 0; i < 18; i++) {
      float curr_weight = getWeightForNdx(i);
      if (curr_weight == 0) {
         continue;
      }
      
      vec4 curr_result = BIND_BONE_POS[i] * vertPos;
      curr_result[3] = 1;
      curr_result = BONE_POS[i] * curr_result;
      curr_result *= curr_weight;
      
      boned_pos += curr_result;
   }
   
   boned_pos.w = 1;
   
   gl_Position = P * MV * ((gpu_rendering == 1) ? boned_pos : vertPos);
	fragNor = (MV * vec4(vertNor, 0.0)).xyz;
}
