//
//  util.hpp
//  Asgn1
//
//  Created by Elliot Fiske on 1/26/16.
//
//

#ifndef util_hpp
#define util_hpp

#include <stdio.h>
#include <vector>
#include <Eigen/Dense>

float buildTable(std::vector<std::pair<float,float> > *usTable, std::vector<Eigen::Vector3f> cps, Eigen::Matrix4f Bcr);

#endif /* util_hpp */
