//
//  shader.cpp
//  slumber
//
//  Created by Elliot Fiske on 5/1/15.
//
//

#include "shader.h"
#include <cassert>
#include <iostream>
#include "GLSL.h"

using namespace std;
using namespace Eigen;

/**
 * Do the actual compiling + linking of the shader
 */
GLuint linkProgram(string vertexShaderFile, string fragmentShaderFile) {
   vertexShaderFile   = SHADER_FOLDER + vertexShaderFile;
   fragmentShaderFile = SHADER_FOLDER + fragmentShaderFile;
   
   GLint error_flag;
   GLuint new_ProgramID;
   
   // Create shader handles
   GLuint VS = glCreateShader(GL_VERTEX_SHADER);
   GLuint FS = glCreateShader(GL_FRAGMENT_SHADER);
   
   // Read shader sources
   const char *vshader = GLSL::textFileRead(vertexShaderFile.c_str());
   const char *fshader = GLSL::textFileRead(fragmentShaderFile.c_str());
   glShaderSource(VS, 1, &vshader, NULL);
   glShaderSource(FS, 1, &fshader, NULL);
   
   // Compile vertex shader
   glCompileShader(VS);
   GLSL::printError();
   glGetShaderiv(VS, GL_COMPILE_STATUS, &error_flag);
   GLSL::printShaderInfoLog(VS);
   if (!error_flag) {
      printf("Error compiling vertex shader %s\n", vertexShaderFile.c_str());
   }
   
   // Compile fragment shader
   glCompileShader(FS);
   GLSL::printError();
   glGetShaderiv(FS, GL_COMPILE_STATUS, &error_flag);
   GLSL::printShaderInfoLog(FS);
   if (!error_flag) {
      printf("Error compiling fragment shader %s\n", fragmentShaderFile.c_str());
   }
   
   // Create the program and link
   new_ProgramID = glCreateProgram();
   glAttachShader(new_ProgramID, VS);
   glAttachShader(new_ProgramID, FS);
   glLinkProgram(new_ProgramID);
   
   GLSL::printError();
   glGetProgramiv(new_ProgramID, GL_LINK_STATUS, &error_flag);
   GLSL::printProgramInfoLog(new_ProgramID);
   if (!error_flag) {
      printf("Error linking shaders %s and %s\n", vertexShaderFile.c_str(), fragmentShaderFile.c_str());
   }
   
   return new_ProgramID;
}

/**
 * Initialize and link a vertex and fragment base MVP shader
 *  from the specified file names.
 */
BaseMVPShader::BaseMVPShader(string vertexShaderFile, string fragmentShaderFile) {
   programID = linkProgram(vertexShaderFile, fragmentShaderFile);
   
   // Make handles to attribute data
   position_AttributeID = GLSL::getAttribLocation(programID, "aPosition");
   normal_AttributeID   = GLSL::getAttribLocation(programID, "aNormal");
   
   // Make handles to uniforms
   projectionMatrix_UniformID = GLSL::getUniformLocation(programID, "uProjMatrix");
   viewMatrix_UniformID       = GLSL::getUniformLocation(programID, "uViewMatrix");
   modelMatrix_UniformID      = GLSL::getUniformLocation(programID, "uModelMatrix");
}

// --------- BASE MVP SHADER SHADER SETTERS -----------
void BaseMVPShader::startUsingShader() {
   glUseProgram(programID);
}

void BaseMVPShader::setPositionArray(GLuint arrayID) {
   GLSL::enableVertexAttribArray(position_AttributeID);
   glBindBuffer(GL_ARRAY_BUFFER, arrayID);
   glVertexAttribPointer(position_AttributeID, 3, GL_FLOAT, GL_FALSE, 0, 0);
}

void BaseMVPShader::setNormalArray(GLuint arrayID) {
   GLSL::enableVertexAttribArray(normal_AttributeID);
   glBindBuffer(GL_ARRAY_BUFFER, arrayID);
   glVertexAttribPointer(normal_AttributeID, 3, GL_FLOAT, GL_FALSE, 0, 0);
}

void BaseMVPShader::setIndexArray(GLuint arrayID) {
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, arrayID);
}

void BaseMVPShader::setProjectionMatrix(Matrix4f projectionMatrix) {
   glUniformMatrix4fv(projectionMatrix_UniformID, 1, GL_FALSE, projectionMatrix);
}

void BaseMVPShader::setViewMatrix(Matrix4f viewMatrix) {
   glUniformMatrix4fv(viewMatrix_UniformID, 1, GL_FALSE, value_ptr(viewMatrix));
}

void BaseMVPShader::setModelMatrix(Matrix4f modelMatrix) {
   glUniformMatrix4fv(modelMatrix_UniformID, 1, GL_FALSE, value_ptr(modelMatrix));
}

void BaseMVPShader::disableAttribArrays() {
   glDisableVertexAttribArray(position_AttributeID);
   glDisableVertexAttribArray(normal_AttributeID);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


