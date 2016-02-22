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
#include <Eigen/Geometry>

#include "GLSL.h"
#include "Program.h"

using namespace Eigen;
using namespace std;

Link::Link() :
   angle(M_PI * 0.1f)
{
}

Link::~Link()
{
   
}

void Link::add_child(std::shared_ptr<Link> me, int how_many) {
   how_many--;
   
   if (how_many == 0) {
      return;
   }
   
   shared_ptr<Link> new_link = make_shared<Link>();
   children.push_back(new_link);
   new_link->parent = me;
   
   new_link->add_child(new_link, how_many);
}

void Link::draw(MatrixStack *M, const std::shared_ptr<Program> prog, const std::shared_ptr<Shape> shape){
   
   Affine3f transform(Translation3f(0.5, 0, 0));
   Matrix4f i_to_p_E = transform.matrix();
   Matrix4f mesh_to_i_E = Matrix4f::Identity();
   
   Matrix3f rot_matrix = AngleAxisf(angle, Vector3f::UnitZ()).matrix();
   i_to_p_E.block<3,3>(0,0) = rot_matrix;
   
   M->pushMatrix();
   M->multMatrix(i_to_p_E);
   M->pushMatrix();
   M->multMatrix(mesh_to_i_E);
   
   glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, M->topMatrix().data());
   
   shape->draw(prog);
   
   M->popMatrix();
   
   for (int ndx = 0; ndx < children.size(); ndx++) {
      children[ndx]->draw(M, prog, shape);
   }
   
   M->popMatrix();
}