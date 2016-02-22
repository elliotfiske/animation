//
//  CeresWrapper.cpp
//  Lab07
//
//  Created by Elliot Fiske on 2/22/16.
//
//

#include "CeresWrapper.hpp"
#include "ceres/ceres.h"

using ceres::AutoDiffCostFunction;
using ceres::CostFunction;
using ceres::Problem;
using ceres::Solver;
using ceres::Solve;

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


SolvedAngles solveAngles(float target_x, float target_y) {
//   google::InitGoogleLogging("you don't matter haha"); TODO: delete me
   
   // The variable to solve for with its initial value. It will be
   // mutated in place by the solver.
   double x[2];
   x[0] = -1.2;
   x[1] = -1.2;
   const double initial_x = x[0];
   
   // Build the problem.
   Problem problem;
   
   // Set up the only cost function (also known as residual). This uses
   // auto-differentiation to obtain the derivative (jacobian).
   CostFunction* cost_function =
   new AutoDiffCostFunction<CostFunctor1, 1, 2>(new CostFunctor1);
   problem.AddResidualBlock(cost_function, NULL, x);
   
   // Run the solver!
   Solver::Options options;
   options.minimizer_progress_to_stdout = true;
   Solver::Summary summary;
   Solve(options, &problem, &summary);
   
   std::cout << summary.BriefReport() << "\n";
   std::cout << "x : " << initial_x
   << " RESULTS: (" << x[0] << ", " << x[1] << ") \n";
   
   SolvedAngles result;
   result.ang_0 = x[0];
   result.ang_1 = x[1];
   return result;
}