#include <iostream>
#include <vector>
#include <random>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"

#define U_STEPS 100

using namespace std;
using namespace Eigen;

GLFWwindow *window; // Main application window
string RESOURCE_DIR = ""; // Where the resources are loaded from
shared_ptr<Program> prog;

bool keyToggles[256] = {false}; // only for English keyboards!
Vector2f cameraRotations(0, 0);
Vector2f mousePrev(-1, -1);

// Control points
vector<Vector3f> cps;

enum SplineType
{
	BEZIER = 0,
	CATMULL_ROM,
	BASIS,
	SPLINE_TYPE_COUNT
};

SplineType type = BEZIER;

static void error_callback(int error, const char *description)
{
	cerr << description << endl;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
	
	if(action == GLFW_PRESS) {
		switch(key) {
			case GLFW_KEY_S:
				type = (SplineType)((type + 1) % SPLINE_TYPE_COUNT);
				break;
			case GLFW_KEY_C:
				cps.clear();
				break;
			case GLFW_KEY_R:
				cps.clear();
				default_random_engine generator;
				uniform_real_distribution<float> distribution(-0.8, 0.8);
				for(int i = 0; i < 10; ++i) {
					float x = distribution(generator);
					float y = distribution(generator);
					float z = distribution(generator);
					cps.push_back(Vector3f(x, y, z));
				}
				break;
		}
	}
}

static void char_callback(GLFWwindow *window, unsigned int key)
{
	keyToggles[key] = !keyToggles[key];
}

static void cursor_position_callback(GLFWwindow* window, double xmouse, double ymouse)
{
	if(mousePrev(0) >= 0) {
		Vector2f mouseCurr;
		mouseCurr << xmouse, ymouse;
		cameraRotations += 0.5f * (mouseCurr - mousePrev);
		mousePrev = mouseCurr;
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
		if(mods & GLFW_MOD_SHIFT) {
			// Insert a new control point
			// Convert from window coord to world coord assuming that we're
			// using an orthgraphic projection from -1 to 1.
			Vector4f x;
			x(0) = 2.0f * ((xmouse / width) - 0.5f);
			x(1) = 2.0f * (((height - ymouse) / height) - 0.5f);
			x(2) = 0.0f;
			x(3) = 1.0f;
			// Apply current rotation matrix
			auto MV = make_shared<MatrixStack>();
			MV->rotate(cameraRotations(0), Vector3f(0, 1, 0));
			MV->rotate(cameraRotations(1), Vector3f(1, 0, 0));
			Matrix4f R = MV->topMatrix();
			x = R * x;
			cps.push_back(x.segment<3>(0));
		} else {
			mousePrev << xmouse, ymouse;
		}
	} else {
		mousePrev << -1, -1;
	}
}

static void init()
{
	GLSL::checkVersion();
	
	// Set background color
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// Enable z-buffer test
	glEnable(GL_DEPTH_TEST);
	
	// Initialize the GLSL program.
	prog = make_shared<Program>();
	prog->setVerbose(false);
	prog->setShaderNames(RESOURCE_DIR + "lab02_vert.glsl", RESOURCE_DIR + "lab02_frag.glsl");
	prog->init();
	prog->addUniform("P");
	prog->addUniform("MV");
	
	// Initialize time.
	glfwSetTime(0.0);
	
	keyToggles[(unsigned)'l'] = true;
}

void draw_curve(MatrixXf G, Matrix4f B) {
   glColor3f(0.0f, 0.0f, 1.0f);
   glBegin(GL_LINE_STRIP);
   for (int u_prog = 0; u_prog < U_STEPS; u_prog++) {
      float u = (float) u_prog / U_STEPS;
      
      Vector4f u_vec;
      u_vec << 1, u, u*u, u*u*u;
      
      Vector3f curve_point = G * B * u_vec;
      glVertex3f(curve_point(0), curve_point(1), curve_point(2));
   }
   glEnd();
}

void draw_axes(MatrixXf G, Matrix4f B, float u) {
   Vector4f u_vec, d_u_vec, d_2_u_vec;
   u_vec << 1, u, u*u, u*u*u;
   d_u_vec << 0, 1, 2*u, 3*u*u;
   d_2_u_vec << 0, 0, 2, 6*u;
   
   Vector3f base_point = G * B * u_vec;
   
   Vector3f p_prime = (G * B * d_u_vec);
   Vector3f p_2_prime = (G * B * d_2_u_vec);
   
   Vector3f tangent_endpoint = p_prime.normalized() * 0.5f;
   Vector3f binormal_endpoint = (p_prime.cross(p_2_prime)).normalized() * 0.5f;
   Vector3f normal_endpoint = base_point + (binormal_endpoint).cross(tangent_endpoint).normalized() * 0.5f;
   
   tangent_endpoint += base_point;
   binormal_endpoint += base_point;
   
   // TANGENT
   glColor3f(1.0f, 0.0f, 0.0f);
   glBegin(GL_LINES);
   glLineWidth(2.0f);
   glVertex3f(base_point(0), base_point(1), base_point(2));
   glVertex3f(tangent_endpoint(0), tangent_endpoint(1), tangent_endpoint(2));
   glEnd();
   
   // BINORMAL
   glColor3f(0.0f, 0.0f, 1.0f);
   glBegin(GL_LINES);
   glVertex3f(base_point(0), base_point(1), base_point(2));
   glVertex3f(binormal_endpoint(0), binormal_endpoint(1), binormal_endpoint(2));
   glEnd();
   
   // NORMAL
   glColor3f(0.0f, 1.0f, 0.0f);
   glBegin(GL_LINES);
   glVertex3f(base_point(0), base_point(1), base_point(2));
   glVertex3f(normal_endpoint(0), normal_endpoint(1), normal_endpoint(2));
   glEnd();
}

void render()
{
	// Update time.
	double t = glfwGetTime();
	
	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);
	
	// Clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
	P->pushMatrix();
	MV->pushMatrix();
	
	P->ortho2D(-1.0, 1.0, -1.0, 1.0);
	MV->rotate(cameraRotations(0), Vector3f(0, 1, 0));
	MV->rotate(cameraRotations(1), Vector3f(1, 0, 0));
	
	// Bind the program
	prog->bind();
	glUniformMatrix4fv(prog->getUniform("P"), 1, GL_TRUE, P->topMatrix().data());
	glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_TRUE, MV->topMatrix().data());
	
	// Draw control points
	int ncps = (int)cps.size();
	glPointSize(5.0f);
	glColor3f(1.0f, 0.0f, 0.0f);
	glBegin(GL_POINTS);
	for(int i = 0; i < ncps; ++i) {
		Vector3f cp = cps[i];
		glVertex3f(cp(0), cp(1), cp(2));
	}
	glEnd();
	glLineWidth(1.0f);
	if(keyToggles[(unsigned)'l']) {
		glColor3f(1.0f, 0.5f, 0.5f);
		glBegin(GL_LINE_STRIP);
		for(int i = 0; i < ncps; ++i) {
			Vector3f cp = cps[i];
			glVertex3f(cp(0), cp(1), cp(2));
		}
		glEnd();
	}
   
   // Draw curvy curves
   if(ncps >= 4) {
      Matrix4f B;
      
      switch (type) {
         case BEZIER:
            B <<  1, -3,  3, -1,
                  0,  3, -6,  3,
                  0,  0,  3, -3,
                  0,  0,  0,  1;
            break;
         case CATMULL_ROM:
            B <<  0, -1,  2, -1,
                  2,  0, -5,  3,
                  0,  1,  4, -3,
                  0,  0, -1,  1;
            
            B *= 0.5f;
            break;
         case BASIS:
            B <<  1, -3,  3, -1,
                  4,  0, -6,  3,
                  1,  3,  3, -3,
                  0,  0,  0,  1;
            
            B /= 6.0f;
            break;
         default:
            printf("WOW THIS ISN'T SUPPOSED TO HAPPEN\n");
            break;
      }
      
      MatrixXf G(3, 4);
      
      glColor3f(0.0f, 0.0f, 1.0f); // TODO: delete
      if (type == BEZIER) {
         for (int i = 0; i < 4; i++) {
            G.block<3, 1>(0, i) = cps[i];
         }
         draw_curve(G, B);
      }
      else {
         float kfloat;
         float u = std::modf(std::fmod(t*0.2f, ncps-3.0f), &kfloat);
         int k = (int)std::floor(kfloat);
       
         for (int range_start = 0; range_start <= ncps - 4; range_start++) {
            for (int i = 0; i < 4; i++) {
               G.block<3, 1>(0, i) = cps[i + range_start];
            }
            
            if (range_start == k) {
               draw_axes(G, B, u);
            }
            
            draw_curve(G, B);
         }
         
         
      }
   }
   
	// Unbind the program
	prog->unbind();

	// Pop matrix stacks.
	MV->popMatrix();
	P->popMatrix();
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
