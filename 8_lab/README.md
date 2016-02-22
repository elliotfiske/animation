Lab 8 -  Nonlinear Least Squares with Google Ceres
==================================================

Today we're going to solve a couple of nonlinear least squares problems with
Google's Ceres solver <http://ceres-solver.org/index.html>.

Please download the code for the lab and go over the code.

- On the web: <http://users.csc.calpoly.edu/~ssueda/teaching/CSC474/2016W/labs/L08/L08.zip>
- On the server: `cp ~ssueda/www/teaching/CSC474/2016W/labs/L08/L08.zip .`

The only file included in the zip file is the Makefile. Much of today's lab is
on building, installing, and compiling the tutorial example from the Ceres
webpage.


Task 1
------

- Download Ceres. We will be using CMake once again to build the code.
- Extract Ceres into a folder. Add an environment variable called `CERES_DIR`
  that points to this folder. Remember to `source`.
- Create a `build` folder inside `CERES_DIR` and `cd` into it.

### OSX & Linux ###

Type `ccmake ..` (not `cmake`) to enter the CMake GUI. Press 'c'. You'll see
some errors related to Eigen. Press 'e' to escape. Now we can change some
build options. Make the following changes. (You'll need to enter the full path
for `CERES_DIR` and `EIGEN3_INCLUDE_DIR`.)

	 BUILD_DOCUMENTATION             OFF
	 BUILD_EXAMPLES                  ON
	 BUILD_SHARED_LIBS               OFF
	 BUILD_TESTING                   ON
	 CMAKE_BUILD_TYPE                Release
	 CMAKE_INSTALL_PREFIX            CERES_DIR/build
	 CUSTOM_BLAS                     OFF
	 CXSPARSE                        OFF
	 CXX11                           ON
	 EIGENSPARSE                     ON
	 EIGEN_INCLUDE_DIR               EIGEN3_INCLUDE_DIR
	 EXPORT_BUILD_DIR                OFF
	 GFLAGS                          OFF
	 LAPACK                          OFF
	 MINIGLOG                        ON
	 OPENMP                          OFF
	 SCHUR_SPECIALIZATIONS           OFF
	 SUITESPARSE                     OFF

Press 't' to enter the advanced settings, and add the following:

	 CMAKE_CXX_FLAGS                  -DMAX_LOG_LEVEL=-100

Press 'c', then 'e', and then 'g'. Now type `make -j4` to compile Ceres. If it
completes successfully, you should be able to run the examples. Try running
`CERES_DIR/build/bin/helloworld`. You should see the following:

	iter      cost      cost_change  |gradient|   |step|    tr_ratio  tr_radius  ls_iter  iter_time  total_time
	   0  4.512500e+01    0.00e+00    9.50e+00   0.00e+00   0.00e+00  1.00e+04        0    2.60e-05    1.93e-03
	   1  4.511598e-07    4.51e+01    9.50e-04   9.50e+00   1.00e+00  3.00e+04        1    3.60e-05    2.03e-03
	   2  5.012552e-16    4.51e-07    3.17e-08   9.50e-04   1.00e+00  9.00e+04        1    5.01e-06    2.05e-03
	Ceres Solver Report: Iterations: 2, Initial cost: 4.512500e+01, Final cost: 5.012552e-16, Termination: CONVERGENCE
	x : 0.5 -> 10

If you see more detailed output, then `MAX_LOG_LEVEL` wasn't defined properly,
so you'll need to rebuild CERES.


### Windows ###

- Run the CMake GUI.
- Enter the folder pointed to by `CERES_DIR` into "Where is the source code:".
- Enter the the `build` folder inside `CERES_DIR` into "Where to build the
  binaries:".
- Press the "Configure" button. Select your compiler and press "Finish".
- Press "OK" on the error dialog.
- Change the setting to the following:

<img src="images/image1.jpg" width="800px"/>

- Enter the advanced settings, and add `/DMAX_LOG_LEVEL=-100` to the end of
  `CMAKE_CXX_FLAGS`.

<img src="images/image2.jpg" width="800px"/>

- Press the "Configure" button and then the "Generate" button.
- Quit CMake.
- Open `Ceres.sln` in `CERES_DIR\build`.
- Build the solution in both Release and Debug modes.
- Run "helloworld" by selecting it as the startup project. You should see the
  output above.


Task 2
------

Using `CERES_DIR/examples/helloworld.cc` as a starting point, try a new cost
function:

$$
f(x,y) = 0.2 ((x+1)^2 + (y+1)^2) + sin(3(x+1)) + sin(3(y+1)) + 2
$$

This is a jaggy paraboloid. Since the cost function is now a function of two
variables, you need to make the following changes to the driver.

	double x[2];
	x[0] = 0.0;
	x[1] = 0.0;
	ceres::Problem problem;
	ceres::CostFunction* cost_function = 
		new ceres::AutoDiffCostFunction<CostFunctor1, 1, 2>(new CostFunctor1);
	...

The template variables are

- Reference to the struct that implements the `operator()` function
- Number of elements in the function output (`residual`)
- Number of elements in the function input (`x`)

Since Ceres is templated, to define a constant (e.g., 2.0), you should cast it
first: `T foo = T(2.0);`.

The global solution is at `-1.50423 -1.50423`, as shown in the figure below.
You may need to give Ceres a different starting point to get this global
minimum.

<img src="images/image3.jpg" width="800px"/>
