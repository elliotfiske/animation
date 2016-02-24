//
//  CeresWrapper.hpp
//  Lab07
//
//  Created by Elliot Fiske on 2/22/16.
//
//

#ifndef CeresWrapper_hpp
#define CeresWrapper_hpp

#include <stdio.h>

#define NO_SPRING 0
#define STRAIGHT 1
#define JAGGED 2

typedef struct SolvedAngles {
   double ang_0;
   double ang_1;
   double ang_2;
   double ang_3;
   double ang_4;
} SolvedAngles;

SolvedAngles solveAngles(double target_x, double target_y, int joint_state);

#endif /* CeresWrapper_hpp */
