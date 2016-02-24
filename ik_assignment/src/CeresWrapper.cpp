//
//  CeresWrapper.cpp
//  Lab07
//
//  Created by Elliot Fiske on 2/22/16.
//
//

#include "CeresWrapper.hpp"
#include "ceres/ceres.h"
#define EIGEN_DONT_ALIGN_STATICALLY
#include <Eigen/Dense>

using ceres::AutoDiffCostFunction;
using ceres::CostFunction;
using ceres::Problem;
using ceres::Solver;
using ceres::Solve;

using namespace Eigen;

double mouse_x = 0;
double mouse_y = 0;

int ceres_joint_state = 0;

struct IKFunctor {
   template <typename T>
   bool operator()(const T* const x, T* residual) const {
      // Input: 5 angles
      Matrix<T, 3, 1> result;
      result << T(1), T(0), T(1);
      
      for (int ndx = 4; ndx >= 0; ndx--) {
         
         Matrix<T, 3, 3> my_frame_to_parent;
         T cos_angle = cos(x[ndx]);
         T sin_angle = sin(x[ndx]);
         
         my_frame_to_parent(0, 0) =  cos_angle;
         my_frame_to_parent(0, 1) = -sin_angle;
         T offset = (ndx == 0) ? T(0) : T(1);
         my_frame_to_parent(0, 2) = offset;
         
         my_frame_to_parent(1, 0) =  sin_angle;
         my_frame_to_parent(1, 1) =  cos_angle;
         my_frame_to_parent(1, 2) = T(0);
         
         my_frame_to_parent(2, 0) = T(0);
         my_frame_to_parent(2, 1) = T(0);
         my_frame_to_parent(2, 2) = T(1);
         
         result = my_frame_to_parent * result;
      }
      
      // x residual
      residual[0] = T(mouse_x) - result(0, 0);
      
      // y residual
      residual[1] = T(mouse_y) - result(1, 0) + residual[0];
      
      return true;
   }
};

struct StraightLines {
   int spring_ndx;
   
   template <typename T>
   bool operator()(const T* const x, T* residual) const {
      
      // Don't care about this if it's NO_SPRING mode
      if (ceres_joint_state == NO_SPRING) {
         residual[0] = T(0);
         return true;
      }
      
      // We don't care about the root link, but we want
      //  all the next angles to be kinda close to the target.
      T curr_x = x[spring_ndx];
      
      T target_val = (spring_ndx % 2 == 0) ? T(-M_PI_2) : T(M_PI_2);
      
      if (ceres_joint_state == STRAIGHT) {
         target_val = T(0);
      }
   
      while (curr_x > T(M_PI)) {
         curr_x -= T(M_2_PI);
      }
   
      while (curr_x < T(-M_PI)) {
         curr_x += T(M_2_PI);
      }
      
      T diff = (target_val - curr_x);
      
      diff /= T(M_PI);
      
      residual[0] = T(2.9) * ( diff * diff * diff );
      return true;
   }
};

SolvedAngles solveAngles(double target_x, double target_y, int curr_joint_state) {
   ceres_joint_state = curr_joint_state;
   
   // The variable to solve for with its initial value. It will be
   // mutated in place by the solver.
   double x[5];
   x[0] = 0;
   x[1] = 0;
   x[2] = 0;
   x[3] = 0;
   x[4] = 0;
   
   Vector2d target(target_x, target_y);
   if ( target.norm() > 5 ) {
      target.normalize();
      target *= 4.9999;
   }
   
   int flipped = 1;
   double lol = 0;
   
   double angle = atan2(target_y, target_x);
   
   if (target_y > 0) {
      target(1) *= -1;
      flipped = 1;
      lol = angle * 2;
   }
   
   mouse_x = target(0);
   mouse_y = target(1);
   
   // Build the problem.
   Problem problem;
   
   // Set up the only cost function (also known as residual). This uses
   // auto-differentiation to obtain the derivative (jacobian).
   CostFunction* cost_function =
   new AutoDiffCostFunction<IKFunctor, 2, 5>(new IKFunctor);
   problem.AddResidualBlock(cost_function, NULL, x);
   
   // Add the cost functions that make the joints springy
   for (int ndx = 1; ndx < 5; ndx++) {
      StraightLines *functor = new StraightLines;
      functor->spring_ndx = ndx;
      CostFunction* spring_function =
         new AutoDiffCostFunction<StraightLines, 1, 5>(functor);
      problem.AddResidualBlock(spring_function, NULL, x);
   }
   
   // Run the solver!
   Solver::Options options;
   options.minimizer_progress_to_stdout = false;
   Solver::Summary summary;
   Solve(options, &problem, &summary);
   
   SolvedAngles result;
   result.ang_0 = x[0] * flipped + lol;
   result.ang_1 = x[1];// * flipped;
   result.ang_2 = x[2];// * flipped;
   result.ang_3 = x[3];// * flipped;
   result.ang_4 = x[4];// * flipped;
   return result;
}