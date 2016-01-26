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
#include "util.hpp"

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
vector<pair<Quaternionf, Quaternionf> > quaternions; // Random quaternions
vector<pair<float,float> > usTable;

float smax = 0; // Total distance of spline
#define TMAX 5  // Total length of animation

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
   cps.push_back(Vector3f(-2.2f, 1.2f, -0.5f) * SPREAD);
   cps.push_back(Vector3f(-1.2f, 0.8f, 0.5f)  * SPREAD);
   cps.push_back(Vector3f(1.0f, 0.0f, -0.7f)  * SPREAD);
   
   // Same as first 4, to connect everybody :3
   cps.push_back(Vector3f(0.0f, 0.0f, 0.0f) * SPREAD);
   cps.push_back(Vector3f(1.2f, 1.0f, 0.0f) * SPREAD);
   cps.push_back(Vector3f(-1.2f, 1.2f, -0.5f) * SPREAD);
   cps.push_back(Vector3f(-1.2f, 0.8f, 0.5f) * SPREAD);
   
   // Make some arbitrary rotation keyframes to spin me right round
   Vector3f y_axis, z_axis, x_axis;
   y_axis << 0.0f, 1.0f, 0.0f;
   z_axis << 0.0f, 0.0f, 1.0f;
   x_axis << 1.0f, 0.0f, 0.0f;
   
   Quaternionf normal, turn_around, roll_1, roll_2, tilt_forward, static_roll_1, static_roll_2, static_tilt_forward;
   normal       = AngleAxisf(0.0f, y_axis);
   turn_around  = AngleAxisf(3.0f/4.0f   * M_PI, y_axis);
   roll_1         = AngleAxisf(0.5f      * M_PI, z_axis);
   roll_2         = AngleAxisf(1.4f      * M_PI, z_axis);
   tilt_forward = AngleAxisf(-0.5f       * M_PI, x_axis);
   static_roll_1  = AngleAxisf(-0.5f     * M_PI, x_axis);
   static_roll_2  = AngleAxisf(-1.4f     * M_PI, x_axis);
   static_tilt_forward = AngleAxisf(0.5f * M_PI, z_axis);
   
   // For reasons I don't quite understand, the Catmull-Rom lerp switches the
   //  X and Z axes. Therefore, I put 2 different sets of quaternion keyframes:
   //  one for the 'static' copters, and 2 switched-axes for the 'interpolated' copter.
   
   quaternions.push_back(make_pair(normal, normal));
   quaternions.push_back(make_pair(roll_1, static_roll_1));
   quaternions.push_back(make_pair(roll_2, static_roll_2));
   quaternions.push_back(make_pair(tilt_forward, static_tilt_forward));
   quaternions.push_back(make_pair(turn_around, turn_around));
   
   // Same as first 4
   quaternions.push_back(make_pair(normal, normal));
   quaternions.push_back(make_pair(roll_1, static_roll_1));
   quaternions.push_back(make_pair(roll_2, static_roll_2));
   quaternions.push_back(make_pair(tilt_forward, static_tilt_forward));
   
   smax = buildTable(usTable, cps, Bcr);
   
	// Initialize time.
	glfwSetTime(0.0);
	
	// If there were any OpenGL errors, this will print something.
	// You can intersperse this line in your code to find the exact location
	// of your OpenGL error.
	GLSL::checkError(GET_FILE_LINE);
}

vector<Matrix4f> drawSpline(Matrix4f currMVMat) {
   vector<Matrix4f> result;
   
   int ncps = (int)cps.size();
   // Draw spline
   MatrixXf G(3,ncps);
   MatrixXf Gk(3,4);
   for(int i = 0; i < ncps; ++i) {
      G.block<3,1>(0,i) = cps[i];
   }
   glLineWidth(1.0f);
   for(int k = 0; k < ncps - 4; ++k) {
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

/* Get the corresponding s-value from the given u-value */
float s2u(float s)
{
   for (int i = 0; i < usTable.size(); i++) {
      if (s < usTable[i].second) {
         float s0 = usTable[i-1].second;
         float s1 = usTable[i].second;
         
         float u0 = usTable[i-1].first;
         float u1 = usTable[i].first;
         
         float alpha = (s - s0) / (s1 - s0);
         return (1.0f - alpha) * u0 + alpha * u1;
      }
   }
   
   return 0.0f;
}

void render()
{
   // Update time.
   double t = glfwGetTime();
   
   float tNorm = std::fmod(t, TMAX) / TMAX;
   float sNorm = tNorm;
   float s = smax * sNorm;
	
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
      
      Quaternionf thisQuat = quaternions[helicopter_ndx].second;
//      if (helicopter_ndx == 2) thisQuat = AngleAxisf(-0.25f * M_PI, Vector3f(1.0f, 0.0f, 0.0f));
      drawHelicopter(cps[helicopter_ndx], thisQuat, t, MV.get(), prog);
   }
   
   // Draw interpolated helicopter
   float kfloat;
   float u = std::modf(std::fmod(t*0.4f, cps.size()-4.0f), &kfloat);
   int k = (int)std::floor(kfloat);
   printf("K: %d\n", k);
   Vector4f u_vec, d_u_vec;
   
   u_vec << 1, u, u*u, u*u*u;
   d_u_vec << 0, 1, 2*u, 3*u*u;
   
   MatrixXf G(3, 4);
   MatrixXf G_quads(4, 4);
   for (int i = 0; i < 4; i++) {
      G.block<3, 1>(0, i) = cps[i + k];
      Quaternionf quaternionToAdd = quaternions[i + k].first;
      if (k != cps.size() && quaternions[i+k].first.dot(quaternions[i+k+1].first) < 0) {
         quaternionToAdd.inverse(); // Take the shortest path
      }
      
      G_quads.block<4, 1>(0, i) = quaternionToAdd.coeffs();
   }
   
   Vector3f interpolated_pos = G * Bcr * u_vec;
   Vector3f tangent = (G * Bcr * d_u_vec).normalized();
   
   Vector4f uVec(1.0f, u, u*u, u*u*u);
   Vector4f qVec = (G_quads * (Bcr * uVec));
   Quaternionf q;
   q.w() = qVec(0);
   q.vec() = qVec.segment<3>(1);
   q.normalize();
   
   Quaternionf adjustment;
   adjustment = AngleAxisf(M_PI, Vector3f(0.0f, 0.0f, 1.0f));
   q *= adjustment;
//   adjustment = AngleAxisf(M_PI, Vector3f(0.0f, 1.0f, 0.0f));
//   q *= adjustment;
   
   Quaternionf interp_rot = Quaternionf::FromTwoVectors(Vector3f(-1.0f, 0.0f, 0.0f), tangent);
   
   drawHelicopter(interpolated_pos, q, t, MV.get(), prog);
	
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
