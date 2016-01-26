//
//  helicopter.hpp
//  Asgn1
//
//  Created by Elliot Fiske on 1/25/16.
//
//

#ifndef helicopter_hpp
#define helicopter_hpp

#define EIGEN_DONT_ALIGN_STATICALLY

#include <stdio.h>
#include <Eigen/Dense>
#include "MatrixStack.h"
#include "Program.h"

void loadHelicopter(std::string RESOURCE_DIR);
void drawHelicopter(const Eigen::Vector3f& pos, const Eigen::Quaternionf& rot, double t, MatrixStack* MV, std::shared_ptr<Program> prog);

#endif /* helicopter_hpp */
