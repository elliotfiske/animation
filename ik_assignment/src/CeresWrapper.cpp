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

// A templated cost functor that implements the residual r = 10 -
// x. The method operator() is templated so that we can then use an
// automatic differentiation wrapper around it to generate its
// derivatives.
struct CostFunctor1 {
   template <typename T>
   bool operator()(const T* const x, T* residual) const {
      T POINT_TWO = T(0.2);
      T ONE = T(1.0);
      T TWO = T(2.0);
      T THREE = T(3.0);
      
      T X_PLUS_ONE = x[0] + ONE;
      T Y_PLUS_ONE = x[1] + ONE;
      
      residual[0] = POINT_TWO * (
                                 X_PLUS_ONE*X_PLUS_ONE + Y_PLUS_ONE*Y_PLUS_ONE
                                 )
      + sin(THREE*X_PLUS_ONE) + sin(THREE*Y_PLUS_ONE)
      + TWO;
      return true;
   }
};



double mouse_x = 0;
double mouse_y = 0;

struct IKFunctor {
   template <typename T>
   bool operator()(const T* const x, T* residual) const {
      // Input: 5 angles
      
      // test with just one joint
      Matrix<T, 3, 3> my_frame_to_parent;
      T cos_angle = cos(x[0]);
      T sin_angle = sin(x[0]);
      
      my_frame_to_parent(0, 0) =  cos_angle;
      my_frame_to_parent(0, 1) = -sin_angle;
      my_frame_to_parent(0, 2) = T(0);
      
      my_frame_to_parent(1, 0) =  sin_angle;
      my_frame_to_parent(1, 1) =  cos_angle;
      my_frame_to_parent(1, 2) = T(0);
      
      my_frame_to_parent(2, 0) = T(0);
      my_frame_to_parent(2, 1) = T(0);
      my_frame_to_parent(2, 2) = T(1);
      
      Matrix<T, 3, 1> curr_point;
      curr_point << T(1), T(0), T(0);
      
      Matrix<T, 3, 1> result = my_frame_to_parent * curr_point;
      
      for (int ndx = 0; ndx < 5; ndx++) {
         
      }
      
      // x residual
      residual[0] = mouse_x - result(0, 0);
      
      // y residual
      residual[1] = mouse_y - result(1, 0);

      
      // Calculate final position of x_curr
      
      // Residual is the distance between x_curr and x_goal
      
      return true;
   }
};

SolvedAngles solveAngles(double target_x, double target_y) {
//   google::InitGoogleLogging("you don't matter haha"); TODO: delete me
   
   // The variable to solve for with its initial value. It will be
   // mutated in place by the solver.
   double x[7];
   x[0] = 0;
   x[1] = 0;
   x[2] = 0;
   x[3] = 0;
   x[4] = 0;
   
   mouse_x = target_x;
   mouse_y = target_y;
   const double initial_x = x[0];
   
   // Build the problem.
   Problem problem;
   
   // Set up the only cost function (also known as residual). This uses
   // auto-differentiation to obtain the derivative (jacobian).
   CostFunction* cost_function =
   new AutoDiffCostFunction<IKFunctor, 2, 5>(new IKFunctor);
   problem.AddResidualBlock(cost_function, NULL, x);
   
   // Run the solver!
   Solver::Options options;
   options.minimizer_progress_to_stdout = false;
   Solver::Summary summary;
   Solve(options, &problem, &summary);
   
//   std::cout << summary.BriefReport() << "\n";
   std::cout << "x : " << initial_x
   << " RESULTS: (" << x[0] << ", " << x[1] << ") \n";
   
   SolvedAngles result;
   result.ang_0 = x[0];
   result.ang_1 = x[1];
   result.ang_2 = x[2];
   result.ang_3 = x[3];
   result.ang_4 = x[4];
   return result;
}