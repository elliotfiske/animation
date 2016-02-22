//
//  Link.cpp
//  Lab07
//
//  Created by Elliot Fiske on 2/21/16.
//
//

#include "Link.hpp"

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "GLSL.h"
#include "Program.h"

using namespace Eigen;

Link::Link() :
   angle(0)
{
}

Link::~Link()
{
   
}

void Link::draw(MatrixStack *M, const std::shared_ptr<Program> prog, const std::shared_ptr<Shape> shape){
   
   Matrix4f i_to_p_E = Matrix4f::Identity();
   Matrix4f mesh_to_i_E = Matrix4f::Identity();
   
   
   M->pushMatrix();
   M->multMatrix(i_to_p_E);
   M->pushMatrix();
   M->multMatrix(mesh_to_i_E);
   
   glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, M->topMatrix().data());
   
   shape->draw(prog);
   
   M->popMatrix();
   
   for (int ndx = 0; ndx < children.size(); ndx++) {
      children[ndx].draw(M, prog, shape);
   }
   
   M->popMatrix();
}