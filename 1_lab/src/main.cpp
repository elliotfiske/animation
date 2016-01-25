#include <iostream>
#include <math.h>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Eigen/Dense>

#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"

using namespace std;
using namespace Eigen;

GLFWwindow *window; // Main application window
string RESOURCE_DIR = ""; // Where the resources are loaded from
shared_ptr<Program> prog;

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
	prog->setShaderNames(RESOURCE_DIR + "lab01_vert.glsl", RESOURCE_DIR + "lab01_frag.glsl");
	prog->init();
	prog->addUniform("P");
	prog->addUniform("MV");

	// Initialize time.
	glfwSetTime(0.0);
}

float A = 1;
float B = 3;
float a = 8;
float b = 7;
float greek_d = 1;

static void render()
{
	// Update time.
	double t = glfwGetTime();

	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	float aspect = width/(float)height;
	glViewport(0, 0, width, height);

	// Clear buffers.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Create matrix stacks.
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
	// Apply projection.
	P->pushMatrix();
	P->perspective(45.0f, aspect, 0.01f, 100.0f);
	// Apply camera transform.
	MV->pushMatrix();
	MV->translate(Vector3f(0, 0, -5));
	
	// Draw mesh using GLSL.
	prog->bind();
	glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, P->topMatrix().data());
	glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, MV->topMatrix().data());
	
	// Old-school OpenGL
	// OK for a shape consisting of a few (<1000) vertices
	
	glColor3f(0.0f, 0.0f, 0.0f);
	glBegin(GL_LINE_STRIP);
   
   int a = 0;
   int b = 1;
   int c = 2;
   
   for (int degrees = -1; degrees < 360; degrees += 1) {
      float real_a = (sin(t * 0.5f) + 1) * a + 1;
      float real_b = (sin(t * 0.5f) + 1) * b + 3;
      
      float x = A * sin(real_a * degrees * M_PI / 180 + greek_d);
      float y = B * cos(real_b * degrees * M_PI / 180);
      
      int time_degrees = (int) (degrees + t * 200) % 360;
      float red = (time_degrees) / 360.0f;
      float green = 1 - red;
      glColor3f(red, green, 0.0f);
      glVertex3f(x, y, 0.0f);
   }
	glEnd();

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
	// Initialize the scene.
	init();
	// Loop until the user closes the window.
	while(!glfwWindowShouldClose(window)) {
		// Render the scene.
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
