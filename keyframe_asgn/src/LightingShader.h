//
//  LightingShader.h
//  slumber
//
//  Created by Elliot Fiske on 5/21/15.
//
//

#ifndef __slumber__LightingShader__
#define __slumber__LightingShader__

#include <stdio.h>
#include "shader.h"

class LightingShader : public BaseMVPShader {
public:
   LightingShader(std::string vertexShaderFile, std::string fragmentShaderFile);
   
   GLuint shadowMap_ID;
   GLuint uv_AttributeID;
   GLuint diffuseTexture_UniformID;
   
   void setUVArray(GLuint arrayID);
   
   void setLightPos(Eigen::Vector3f lightPos);
   void setAmbientColor(float color[]);
   void setDiffuseColor(float color[]);
   void setSpecularColor(float color[]);
   void setShininess(float shininess);
   void setAttenuation(float attenuation);
   void setLightMVP(Eigen::Matrix4f lightMVP);
   void setHighlightVP(Eigen::Matrix4f VP);
   
private:
   GLuint lightPos_UniformID;
   GLuint ambientMaterial_uniformID;
   GLuint diffuseMaterial_UniformID;
   GLuint specularMaterial_UniformID;
   GLuint shininess_UniformID;
   GLuint lightMVP_UniformID;
   GLuint highlightVP_UniformID;
   GLuint attenuation_UniformID;
};

#endif /* defined(__slumber__LightingShader__) */
