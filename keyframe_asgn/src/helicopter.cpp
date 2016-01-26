//
//  helicopter.cpp
//  Asgn1
//
//  Created by Elliot Fiske on 1/25/16.
//
//

#include "helicopter.hpp"
#include "GLSL.h"
#include "Shape.h"

using namespace Eigen;
using namespace std;

shared_ptr<Shape> helibody_1;
shared_ptr<Shape> helibody_2;
shared_ptr<Shape> heliprop_1;
shared_ptr<Shape> heliprop_2;

void loadHelicopter(std::string RESOURCE_DIR) {
   helibody_1 = make_shared<Shape>();
   helibody_1->loadMesh(RESOURCE_DIR + "helicopter_body1.obj");
   helibody_1->init();
   
   helibody_2 = make_shared<Shape>();
   helibody_2->loadMesh(RESOURCE_DIR + "helicopter_body2.obj");
   helibody_2->init();
   
   heliprop_1 = make_shared<Shape>();
   heliprop_1->loadMesh(RESOURCE_DIR + "helicopter_prop1.obj");
   heliprop_1->init();
   
   heliprop_2 = make_shared<Shape>();
   heliprop_2->loadMesh(RESOURCE_DIR + "helicopter_prop2.obj");
   heliprop_2->init();
}


Matrix4f addQuaternionToStack(const Quaternionf& rot) {
   Matrix4f R = Matrix4f::Identity();
   R.block<3, 3>(0, 0) = rot.toRotationMatrix();
   return R;
}

void drawHelicopter(const Eigen::Vector3f& pos, const Eigen::Quaternionf& rot, double t, MatrixStack* MV, std::shared_ptr<Program> prog) {
   Eigen::Vector3f axis_prop1, axis_prop2;
   axis_prop1 << 0.0f, 1.0f, 0.0f;
   axis_prop2 << 0.0f, 0.0f, 1.0f;
   
   Eigen::Quaternionf quatern_prop1, quatern_prop2;
   quatern_prop1 = Eigen::AngleAxisf(t*90.0f/180.0f*M_PI, axis_prop1);
   quatern_prop2 = Eigen::AngleAxisf(t*90.0f/180.0f*M_PI, axis_prop2);
   
   Eigen::Vector3f prop1_center, prop2_center;
   prop1_center << -0.0133f, 0.4819f, 0.0f;
   prop2_center << 0.6228f, 0.1179f, 0.1365f;
   
   // BODY
   glUniform3f(prog->getUniform("UaColor"), 0.0f, 0.0f, 0.3f);
   glUniform3f(prog->getUniform("UdColor"), 0.0f, 0.0f, 0.8f);
   
   MV->pushMatrix();
   MV->translate(pos);
   MV->multMatrix(addQuaternionToStack(rot));
   glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, MV->topMatrix().data());
   helibody_1->draw(prog);
   MV->popMatrix();
   
   // BODY 2
   glUniform3f(prog->getUniform("UaColor"), 0.0f, 0.3f, 0.0f);
   glUniform3f(prog->getUniform("UdColor"), 0.0f, 0.8f, 0.0f);
   
   MV->pushMatrix();
   MV->translate(pos);
   MV->multMatrix(addQuaternionToStack(rot));
   glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, MV->topMatrix().data());
   helibody_2->draw(prog);
   MV->popMatrix();
   
   
   
   // PROPELLORS
   glUniform3f(prog->getUniform("UaColor"), 0.2f, 0.2f, 0.2f);
   glUniform3f(prog->getUniform("UdColor"), 0.8f, 0.8f, 0.8f);
   
   MV->pushMatrix();
   
   MV->translate(pos);
   MV->multMatrix(addQuaternionToStack(rot));
   
   // Spinning propellor
   MV->translate(-prop1_center);
   MV->multMatrix(addQuaternionToStack(quatern_prop1));
   MV->translate(prop1_center);
   
   glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, MV->topMatrix().data());
   heliprop_1->draw(prog);
   MV->popMatrix();
   
   
   MV->pushMatrix();
   MV->translate(pos);
   MV->multMatrix(addQuaternionToStack(rot));
   
   // Spinning propellor
   MV->translate(prop2_center);
   MV->multMatrix(addQuaternionToStack(quatern_prop2));
   MV->translate(-prop2_center);
   
   glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, MV->topMatrix().data());
   heliprop_2->draw(prog);
MV->popMatrix();
}