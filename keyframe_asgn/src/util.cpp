//
//  util.cpp
//  Asgn1
//
//  Created by Elliot Fiske on 1/26/16.
//
// Handles busywork like generating the u -> s table, and drawing all the
//  grids and frames.

#include "util.hpp"

using namespace std;
using namespace Eigen;


#define MAX_SAMPLES 5

Vector4f uvecify(float u) {
   return Vector4f(0.0f, 1.0f, 2*u, 3*u*u);
}

float buildTable(std::vector<std::pair<float,float> > *usTable, std::vector<Eigen::Vector3f> cps, Eigen::Matrix4f Bcr)
{
   usTable->clear();
   
   int ncps = cps.size();
   
   // Get the G and B matrices
   MatrixXf G(3, cps.size());
   MatrixXf Gk(3,4);
   for(int i = 0; i < cps.size(); ++i) {
      G.block<3,1>(0,i) = cps[i];
   }
   
   float total_dist = 0.0f;
   float s = 0;
   
   for(int k = 0; k < ncps - 3; ++k) {
      Gk = G.block<3,4>(0,k);
      
      for (int sampleNum = 0; sampleNum < MAX_SAMPLES; sampleNum++) {
         float ub = (float) (sampleNum + 1) / (float) MAX_SAMPLES;
         float ua = (float) (sampleNum)     / (float) MAX_SAMPLES;
         
         float ubua_2 =     (ub - ua) / 2.0f;
         float ubua_2_pos = (ub + ua) / 2.0f;
         
         Vector4f u_prime_1 = uvecify(ubua_2 * -0.77459f + ubua_2_pos);
         Vector4f u_prime_2 = uvecify(ubua_2 * 0.0f      + ubua_2_pos);
         Vector4f u_prime_3 = uvecify(ubua_2 * 0.77459f  + ubua_2_pos);
         
         Vector3f p_prime_1 = Gk * Bcr * u_prime_1;
         Vector3f p_prime_2 = Gk * Bcr * u_prime_2;
         Vector3f p_prime_3 = Gk * Bcr * u_prime_3;
         
         float w1 = 5.0f/9.0f;
         float w2 = 8.0f/9.0f;
         float w3 = 5.0f/9.0f;
         
         s = ubua_2 * (
           w1*p_prime_1.norm() + w2*p_prime_2.norm() + w3*p_prime_3.norm()
         );
         
         usTable->push_back(make_pair(ua + k, total_dist));
         
         total_dist += s;
      }
   }
   
   return (*usTable)[(ncps-4) * MAX_SAMPLES].second;
}