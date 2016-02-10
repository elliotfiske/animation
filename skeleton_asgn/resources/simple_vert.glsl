#version 120
attribute vec4 vertPos;
attribute vec3 vertNor;
attribute vec2 vertTex;
uniform mat4 P;
uniform mat4 MV;
varying vec3 fragNor;

// New Attributes

// How heavily each of the bones affect the vertex
//attribute vec4 weights0;
//attribute vec4 weights1;
//attribute vec4 weights2;
//attribute vec4 weights3;
//
//// Which bones affect the vertex
//attribute vec4 bones0;
//attribute vec4 bones1;
//attribute vec4 bones2;
//attribute vec4 bones3;
//
//// How many bones affect the current vertex?
//attribute int num_bones;

// The rigidtransform of all the bones
uniform mat4 BONE_POS[18];

void main()
{
   vec4 boned_pos = BONE_POS[0] * vertPos;
   
	gl_Position = P * MV * boned_pos + 0.0001 * vec4(vertTex, 0, 0);
	fragNor = (MV * vec4(vertNor, 0.0)).xyz;
}
