//
//  shader.h
//  slumber
//
//  Created by Elliot Fiske on 5/1/15.
//
//

#ifndef __slumber__shader__
#define __slumber__shader__

#include <stdio.h>
#include <string>
#include "GLSL.h"
#include <Eigen/Dense>

GLuint linkProgram(std::string vertexShaderFile, std::string fragmentShaderFile);

class BaseMVPShader {
public:
   BaseMVPShader(std::string vertexShaderFile, std::string fragmentShaderFile);
   
   GLuint programID;
   
   void startUsingShader();
   void setNormalArray(GLuint arrayID);
   void setIndexArray(GLuint arrayID);
   
   void setPositionArray(GLuint arrayID);
   void setProjectionMatrix(Eigen::Matrix4f projectionMatrix);
   void setModelMatrix(Eigen::Matrix4f modelMatrix);
   void setViewMatrix(Eigen::Matrix4f viewMatrix);
   
   // Clean-up
   void disableAttribArrays();
   
protected:
   GLuint position_AttributeID;
   GLuint normal_AttributeID;
   
   GLuint projectionMatrix_UniformID;
   GLuint viewMatrix_UniformID;
   GLuint modelMatrix_UniformID;
   
};

#endif /* defined(__slumber__shader__) */
