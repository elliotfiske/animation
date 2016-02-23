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

//bool
double last_angle;

double weights[] = { 0, 2, 0.5, 0.5, 2 };
//         T spring_val = (spring_ndx % 2 == 0) ? T(-M_PI_4) : T(M_PI_4);

struct StraightLines {
   int spring_ndx;
   
   template <typename T>
   bool operator()(const T* const x, T* residual) const {
      
      // We don't care about the root link, but we want
      //  all the next angles to be kinda close to 0.
      T curr_x = x[spring_ndx];
   
      while (curr_x > T(M_PI)) {
         curr_x -= T(M_2_PI);
      }
   
      while (curr_x < T(-M_PI)) {
         curr_x += T(M_2_PI);
      }
      
      curr_x /= T(M_PI);
      
      residual[0] = T(0.01) * ( curr_x * curr_x * curr_x );// * (weights[spring_ndx]);
      return true;
   }
};

SolvedAngles solveAngles(double target_x, double target_y) {
//   google::InitGoogleLogging("you don't matter haha"); TODO: delete me
   
   // The variable to solve for with its initial value. It will be
   // mutated in place by the solver.
   double x[5];
   x[0] = 0;
   x[1] = 0;
   x[2] = 0;
   x[3] = 0;
   x[4] = 0;
   
   int flipped = 1;
   
   if (target_y > 0) {
      target_y *= -1;
      flipped = -1;
   }
   
   mouse_x = target_x;
   mouse_y = target_y;
   
   // Build the problem.
   Problem problem;
   
   // Set up the only cost function (also known as residual). This uses
   // auto-differentiation to obtain the derivative (jacobian).
   CostFunction* cost_function =
   new AutoDiffCostFunction<IKFunctor, 2, 5>(new IKFunctor);
   problem.AddResidualBlock(cost_function, NULL, x);
   
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
   
//   std::cout << summary.BriefReport() << "\n";
   std::cout << " RESULTS: (" << x[0] << ", " << x[1] << ", " << x[2] << ", " << x[3] << ", " << x[4] << ") \n";
   
   SolvedAngles result;
   result.ang_0 = x[0] * flipped;
   result.ang_1 = x[1] * flipped;
   result.ang_2 = x[2] * flipped;
   result.ang_3 = x[3] * flipped;
   result.ang_4 = x[4] * flipped;
   return result;
}