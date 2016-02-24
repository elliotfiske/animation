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

#define EIGEN_DONT_ALIGN_STATICALLY
#include <Eigen/Dense>
#include "MatrixStack.h"
#include "Shape.h"


class Link: public std::enable_shared_from_this<Link>
{
public:
   EIGEN_MAKE_ALIGNED_OPERATOR_NEW
   Link();
   virtual ~Link();
   
   std::shared_ptr<Link> parent;
   
   std::vector<std::shared_ptr<Link> > children;
   
   void add_child(std::shared_ptr<Link> me, int how_many);
   void set_nth_angle(int ndx, double angle);
   
   float angle;
   float parent_offset; // How far the mesh center is away from the joint
   
   void draw(MatrixStack *M, const std::shared_ptr<Program> prog, const std::shared_ptr<Shape> shape);
   
   // What shape to draw when we draw this Link
   static std::shared_ptr<Shape> shape;
};

#endif /* Link_hpp */
