//
//  Link.hpp
//  Lab07
//
//  Created by Elliot Fiske on 2/21/16.
//
//

#ifndef Link_hpp
#define Link_hpp

#include <stdio.h>
#include <memory>
#include <vector>
#include <Eigen/Dense>
#include "MatrixStack.h"
#include "Shape.h"

class Link: public std::enable_shared_from_this<Link>
{
public:
   Link();
   virtual ~Link();
   
   std::shared_ptr<Link> parent;
   
   std::vector<Link> children;
   
   float angle;
//   Eigen::Matrix4f i_to_p_E;
//   Eigen::Matrix4f mesh_to_i_E;
   
   void draw(MatrixStack *M, const std::shared_ptr<Program> prog, const std::shared_ptr<Shape> shape);
   
   // What shape to draw when we draw this Link
   static std::shared_ptr<Shape> shape;
};

#endif /* Link_hpp */
