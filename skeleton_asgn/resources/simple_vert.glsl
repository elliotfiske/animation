#version 120
attribute vec4 vertPos;
attribute vec3 vertNor;
//attribute vec2 vertTex;
uniform mat4 P;
uniform mat4 MV;
varying vec3 fragNor;

// New Attributes

// How heavily each of the bones affect the vertex
attribute vec4 weights0;
attribute vec4 weights1;
attribute vec4 weights2;
attribute vec4 weights3;

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
}

float getBoneNdxForNdx(int ndx) {
   if (ndx < 4) {
      return bones0[ndx];
   }
   else if (ndx < 8) {
      return bones1[ndx-4];
   }
   else if (ndx < 12) {
      return bones2[ndx-8];
   }
   else if (ndx < 16) {
      return bones3[ndx-12];
   }
}

void main()
{
   vec4 result_vertex = vec4(0, 0, 0, 0);
   
   for (int ndx = 0; ndx < num_bones; ndx++) {
      int bone_ndx = int(getBoneNdxForNdx(ndx));
      
      float curr_weight = getWeightForNdx(ndx);

      vec4 bind_pose_vert = BIND_BONE_POS[bone_ndx] * vertPos;
      vec4 animated_vert = BONE_POS[bone_ndx] * bind_pose_vert;
      vec4 weighted_vert = animated_vert * curr_weight;
      
      result_vertex += weighted_vert;
   }
   
   gl_Position = P * MV * ((gpu_rendering == 1) ? result_vertex : vertPos);
	fragNor = (MV * vec4(vertNor, 0.0)).xyz;
	fragNor = vec3(0, num_bones/15, 0);
}
