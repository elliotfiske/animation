#include <iostream>
#include <vector>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Camera.h"
#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "Shape.h"
#include "helicopter.hpp"

using namespace std;
using namespace Eigen;

bool keyToggles[256] = {false}; // only for English keyboards!

GLFWwindow *window; // Main application window
string RESOURCE_DIR = ""; // Where the resources are loaded from

shared_ptr<Program> prog;
shared_ptr<Camera> camera;
shared_ptr<Shape> bunny;

Matrix4f Bcr; // Catmull-Rom B matrix
vector<Vector3f> cps; // Control points
vector<Quaternionf> quaternions; // Random quaternions

static void error_callback(int error, const char *description)
{
	cerr << description << endl;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}

static void char_callback(GLFWwindow *window, unsigned int key)
{
	keyToggles[key] = !keyToggles[key];
}

static void cursor_position_callback(GLFWwindow* window, double xmouse, double ymouse)
{
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if(state == GLFW_PRESS) {
		camera->mouseMoved(xmouse, ymouse);
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// Get the current mouse position.
	double xmouse, ymouse;
	glfwGetCursorPos(window, &xmouse, &ymouse);
	// Get current window size.
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	if(action == GLFW_PRESS) {
		bool shift = mods & GLFW_MOD_SHIFT;
		bool ctrl  = mods & GLFW_MOD_CONTROL;
		bool alt   = mods & GLFW_MOD_ALT;
		camera->mouseClicked(xmouse, ymouse, shift, ctrl, alt);
	}
}

static void init()
{
	GLSL::checkVersion();
	
	// Set background color
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// Enable z-buffer test
	glEnable(GL_DEPTH_TEST);
	
	keyToggles[(unsigned)'c'] = true;
	
	prog = make_shared<Program>();
	prog->setShaderNames(RESOURCE_DIR + "simple_vert.glsl", RESOURCE_DIR + "simple_frag.glsl");
	prog->setVerbose(false); // Set this to true when debugging.
	prog->init();
	prog->addUniform("P");
	prog->addUniform("MV");
	prog->addAttribute("vertPos");
	prog->addAttribute("vertNor");
	
	bunny = make_shared<Shape>();
	bunny->loadMesh(RESOURCE_DIR + "bunny.obj");
	bunny->init();
   
   loadHelicopter(RESOURCE_DIR);
   
	camera = make_shared<Camera>();
   
   Bcr << 0.0f, -1.0f,  2.0f, -1.0f,
          2.0f,  0.0f, -5.0f,  3.0f,
          0.0f,  1.0f,  4.0f, -3.0f,
          0.0f,  0.0f, -1.0f,  1.0f;
   Bcr *= 0.5;
   
   // Control Points were kind of close together. Spread 'em out!
   float SPREAD = 1.5f;
   
   // Init control points
   cps.push_back(Vector3f(0.0f, 0.0f, 0.0f)   * SPREAD);
   cps.push_back(Vector3f(1.2f, 1.0f, 0.0f)   * SPREAD);
   cps.push_back(Vector3f(-1.2f, 1.2f, -0.5f) * SPREAD);
   cps.push_back(Vector3f(-1.2f, 0.8f, 0.5f)  * SPREAD);
   cps.push_back(Vector3f(1.2f, 0.0f, 0.7f)   * SPREAD);
   cps.push_back(Vector3f(1.0f, 0.0f, -0.7f)  * SPREAD);
   
   // Same as first 4, to connect everybody :3
   cps.push_back(Vector3f(0.0f, 0.0f, 0.0f) * SPREAD);
   cps.push_back(Vector3f(1.2f, 1.0f, 0.0f) * SPREAD);
   cps.push_back(Vector3f(-1.2f, 1.2f, -0.5f) * SPREAD);
   cps.push_back(Vector3f(-1.2f, 0.8f, 0.5f) * SPREAD);
   
   // Make some arbitrary rotation keyframes to spin me right round
   //   quaternions.push_back(AngleAxisf(90.0f, Vector3f(0.0f, 1.0f, 0.0f)));
   Eigen::Vector3f y_axis;
   y_axis << 0.0f, 1.0f, 0.0f;
   
   Eigen::Quaternionf q1;
   q1 = Eigen::AngleAxisf(90.0f, y_axis);
   
   quaternions.push_back(q1);
   
   
	// Initialize time.
	glfwSetTime(0.0);
	
	// If there were any OpenGL errors, this will print something.
	// You can intersperse this line in your code to find the exact location
	// of your OpenGL error.
	GLSL::checkError(GET_FILE_LINE);
}

vector<Matrix4f> drawSpline(Matrix4f currMVMat) {
   vector<Matrix4f> result;
   
   // Draw control points
   int ncps = (int)cps.size();
   glPointSize(5.0f);
   glColor3f(1.0f, 0.0f, 0.0f);
   glBegin(GL_POINTS);
   for(int i = 0; i < ncps; ++i) {
      Vector3f cp = cps[i];
      if(keyToggles[(unsigned)'k']) {
         glVertex3f(cp(0), cp(1), cp(2));
      }
   }
   glEnd();
   glLineWidth(1.0f);
   if(keyToggles[(unsigned)'l']) {
      glColor3f(1.0f, 0.5f, 0.5f);
      glBegin(GL_LINE_STRIP);
      for(int i = 0; i < ncps; ++i) {
         Vector3f cp = cps[i];
//         glVertex3f(cp(0), cp(1), cp(2));
      }
      glEnd();
   }
   
   // Draw spline
   MatrixXf G(3,ncps);
   MatrixXf Gk(3,4);
   for(int i = 0; i < ncps; ++i) {
      G.block<3,1>(0,i) = cps[i];
   }
   glLineWidth(1.0f);
   for(int k = 0; k < ncps - 3; ++k) {
      int n = 32; // curve discretization
      // Gk is the 3x4 block starting at column k
      Gk = G.block<3,4>(0,k);
      glBegin(GL_LINE_STRIP);
      glColor3f(1.0f, 0.0f, 0.0f);
      for(int i = 0; i < n; ++i) {
         // u goes from 0 to 1 within this segment
         float u = i / (n - 1.0f);
         // Compute spline point at u
         Vector4f uVec(1.0f, u, u*u, u*u*u);
         Vector3f P = Gk * Bcr * uVec;
         if(keyToggles[(unsigned)'k']) {
            glVertex3fv(P.data());
         }
      }
      glEnd();
   }
   
   return result;
}

void render()
{
   // Update time.
   double t = glfwGetTime();
	
	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);
	
	// Use the window size for camera.
	glfwGetWindowSize(window, &width, &height);
	camera->setAspect((float)width/(float)height);
	
	// Clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if(keyToggles[(unsigned)'c']) {
		glEnable(GL_CULL_FACE);
	} else {
		glDisable(GL_CULL_FACE);
	}
	if(keyToggles[(unsigned)'l']) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
	
	// Apply camera transforms
	P->pushMatrix();
	camera->applyProjectionMatrix(P);
	MV->pushMatrix();
	camera->applyViewMatrix(MV);
	
	//////////////////////////////////////////////////////
	// Draw origin frame using old-style OpenGL
	// (before binding the program)
	//////////////////////////////////////////////////////
	
	// Setup the projection matrix
	GLSL::checkError(GET_FILE_LINE);
	glMatrixMode(GL_PROJECTION);
	GLSL::checkError(GET_FILE_LINE);
	glPushMatrix();
	glLoadMatrixf(P->topMatrix().data());
	
	// Setup the modelview matrix
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadMatrixf(MV->topMatrix().data());
	
	// Draw frame
	glLineWidth(2);
	glBegin(GL_LINES);
	glColor3f(1, 0, 0);
	glVertex3f(0, 0, 0);
	glVertex3f(1, 0, 0);
	glColor3f(0, 1, 0);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 1, 0);
	glColor3f(0, 0, 1);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 0, 1);
	glEnd();
	glLineWidth(1);
   
   // Draw spliny the spline
   drawSpline(MV->topMatrix());
	
	// Pop modelview matrix
	glPopMatrix();
	
	// Pop projection matrix
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	
	//////////////////////////////////////////////////////
	// Now draw the shape using modern OpenGL
	//////////////////////////////////////////////////////
	
	// Bind the program
	prog->bind();
	
	// Send projection matrix (same for all helis)
	glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, P->topMatrix().data());
   
   Eigen::Quaternionf quatern_prop1;
   
   // Draw the static helicopters
   for (int helicopter_ndx = 0; helicopter_ndx < cps.size() - 3; helicopter_ndx++) {
      
      // Generate the G Matrix for this helicopter's corresponding control points
      MatrixXf G(3, 4);
      for (int cp_ndx = 0; cp_ndx < 4; cp_ndx++) {
         G.block<3, 1>(0, cp_ndx) = cps[cp_ndx + helicopter_ndx];
      }
      
      Vector4f curr_helicopter_d_u_vec(0.0f, 1.0f, 2.0f, 3.0f); // u == 0
      Vector3f curr_helicopter_tangent = (G * Bcr * curr_helicopter_d_u_vec).normalized();
      Quaternionf curr_heli_rot = Quaternionf::FromTwoVectors(Vector3f(-1.0f, 0.0f, 0.0f), curr_helicopter_tangent);
      
      drawHelicopter(cps[helicopter_ndx], curr_heli_rot, t, MV.get(), prog);
   }
   
   // Draw interpolated helicopter
   float kfloat;
   float u = std::modf(std::fmod(t*0.4f, cps.size()-4.0f), &kfloat);
   int k = (int)std::floor(kfloat);
   Vector4f u_vec, d_u_vec;
   
   u_vec << 1, u, u*u, u*u*u;
   d_u_vec << 0, 1, 2*u, 3*u*u;
   
   MatrixXf G(3, 4);
   for (int i = 0; i < 4; i++) {
      G.block<3, 1>(0, i) = cps[i + k];
   }
   
   Vector3f interpolated_pos = G * Bcr * u_vec;
   Vector3f tangent = (G * Bcr * d_u_vec).normalized();
   
//   Vector4f uVec(1.0f, u, u*u, u*u*u);
//   Vector4f qVec = (G * (Bcr * uVec));
//   Quaternionf q;
//   q.w() = qVec(0);
//   q.vec() = qVec.segment<3>(1);
//   q.normalize();
   
   Quaternionf interp_rot = Quaternionf::FromTwoVectors(Vector3f(-1.0f, 0.0f, 0.0f), tangent);
   
   drawHelicopter(interpolated_pos, interp_rot, t, MV.get(), prog);
	
	// Unbind the program
	prog->unbind();
	
	//////////////////////////////////////////////////////
	// Cleanup
	//////////////////////////////////////////////////////
	
	// Pop stacks
	MV->popMatrix();
	P->popMatrix();
	
	GLSL::checkError(GET_FILE_LINE);
}

int main(int argc, char **argv)
{
	if(argc < 2) {
		cout << "Please specify the resource directory." << endl;
		return 0;
	}
	RESOURCE_DIR = argv[1] + string("/");
	
	// Set error callback.
	glfwSetErrorCallback(error_callback);
	// Initialize the library.
	if(!glfwInit()) {
		return -1;
	}
	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(640, 480, "ELLIOT FISKE", NULL, NULL);
	if(!window) {
		glfwTerminate();
		return -1;
	}
	// Make the window's context current.
	glfwMakeContextCurrent(window);
	// Initialize GLEW.
	glewExperimental = true;
	if(glewInit() != GLEW_OK) {
		cerr << "Failed to initialize GLEW" << endl;
		return -1;
	}
	glGetError(); // A bug in glewInit() causes an error that we can safely ignore.
	cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	// Set vsync.
	glfwSwapInterval(1);
	// Set keyboard callback.
	glfwSetKeyCallback(window, key_callback);
	// Set char callback.
	glfwSetCharCallback(window, char_callback);
	// Set cursor position callback.
	glfwSetCursorPosCallback(window, cursor_position_callback);
	// Set mouse button callback.
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	// Initialize scene.
	init();
	// Loop until the user closes the window.
	while(!glfwWindowShouldClose(window)) {
		// Render scene.
		render();
		// Swap front and back buffers.
		glfwSwapBuffers(window);
		// Poll for and process events.
		glfwPollEvents();
	}
	// Quit program.
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
