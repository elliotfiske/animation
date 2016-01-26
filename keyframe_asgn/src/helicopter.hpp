//
//  helicopter.hpp
//  Asgn1
//
//  Created by Elliot Fiske on 1/25/16.
//
//

#ifndef helicopter_hpp
#define helicopter_hpp

#include <stdio.h>
#include <Eigen/Dense>
#include "MatrixStack.h"
#include "Program.h"

void loadHelicopter(std::string RESOURCE_DIR);
void drawHelicopter(Eigen::Vector3f pos, Eigen::Quaternionf rot, double t, MatrixStack* MV_void, std::shared_ptr<Program> prog);

#endif /* helicopter_hpp */
