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
#include "Texture.h"
#include "Link.hpp"
#include "CeresWrapper.hpp"

using namespace std;
using namespace Eigen;

bool keyToggles[256] = {false}; // only for English keyboards!

GLFWwindow *window; // Main application window
string RESOURCE_DIR = ""; // Where the resources are loaded from

shared_ptr<Program> progSimple;
shared_ptr<Program> progTex;
shared_ptr<Camera> camera;
shared_ptr<Shape> shape;
shared_ptr<Texture> texture;

shared_ptr<Link> root_link;

Vector2f mouse;

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

int joint_state = 0;

static void char_callback(GLFWwindow *window, unsigned int key)
{
	keyToggles[key] = !keyToggles[key];
   if (key == 's') {
      joint_state ++;
      joint_state %= 3;
   }
}

double curr_mouse_x = 0;
double curr_mouse_y = 0;

SolvedAngles curr_angles;

void window2world(double xmouse, double ymouse)
{
   // Get current window size.
   int width, height;
   glfwGetWindowSize(window, &width, &height);
   // Convert from window coord to world coord assuming that we're
   // using an orthgraphic projection from -s to s.
   double s = 5.0; // window size
   float aspect = (float)width/height;

   curr_mouse_x = s * 2.0f * ((xmouse / width) - 0.5f)* aspect;
   curr_mouse_y = s * 2.0f * (((height - ymouse) / height) - 0.5f);

   curr_angles = solveAngles(curr_mouse_x, curr_mouse_y, joint_state);
   
   root_link->set_nth_angle(0, curr_angles.ang_0);
   root_link->set_nth_angle(1, curr_angles.ang_1);
   root_link->set_nth_angle(2, curr_angles.ang_2);
   root_link->set_nth_angle(3, curr_angles.ang_3);
   root_link->set_nth_angle(4, curr_angles.ang_4);
}

bool mouse_is_down = false;

static void cursor_position_callback(GLFWwindow* window, double xmouse, double ymouse)
{
   if (mouse_is_down) {
      window2world(xmouse, ymouse);
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
      window2world(xmouse, ymouse);
      mouse_is_down = true;
   }
   else if (action == GLFW_RELEASE) {
      mouse_is_down = false;
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
	
	progSimple = make_shared<Program>();
	progSimple->setShaderNames(RESOURCE_DIR + "simple_vert.glsl", RESOURCE_DIR + "simple_frag.glsl");
	progSimple->setVerbose(false); // Set this to true when debugging.
	progSimple->init();
	progSimple->addUniform("P");
	progSimple->addUniform("MV");
	
	progTex = make_shared<Program>();
	progTex->setVerbose(true); // Set this to true when debugging.
	progTex->setShaderNames(RESOURCE_DIR + "tex_vert.glsl", RESOURCE_DIR + "tex_frag.glsl");
	progTex->init();
	progTex->addUniform("P");
	progTex->addUniform("MV");
	progTex->addAttribute("vertPos");
	progTex->addAttribute("vertNor");
	progTex->addAttribute("vertTex");
	progTex->addUniform("texture0");
	
	texture = make_shared<Texture>();
	texture->setFilename(RESOURCE_DIR + "metal_texture_15_by_wojtar_stock.jpg");
	texture->init();
	
	shape = make_shared<Shape>();
	shape->loadMesh(RESOURCE_DIR + "link.obj");
	shape->init();
   
   root_link = make_shared<Link>();
   root_link->parent_offset = 0.0f; // Root link is smack dab in the center
   root_link->add_child(root_link, 5);
	
	camera = make_shared<Camera>();
	
	// Initialize time.
	glfwSetTime(0.0);
	
	// If there were any OpenGL errors, this will print something.
	// You can intersperse this line in your code to find the exact location
	// of your OpenGL error.
	GLSL::checkError(GET_FILE_LINE);
}

void render()
{
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
   MV->pushMatrix();
   
   Vector3f translation(0.0f, 0.0f, -1.0f);
   MV->translate(translation);
   
   float aspect = (float)width/height;
   P->pushMatrix();
   double s = 5.0;
   P->ortho(-s*aspect, s*aspect, -s, s, -2.0, 2.0);

	
	// Draw grid
	progSimple->bind();
	glUniformMatrix4fv(progSimple->getUniform("P"), 1, GL_FALSE, P->topMatrix().data());
	glUniformMatrix4fv(progSimple->getUniform("MV"), 1, GL_FALSE, MV->topMatrix().data());
	glLineWidth(2.0f);
	float x0 = -5.0f;
	float x1 = 5.0f;
	float y0 = -5.0f;
	float y1 = 5.0f;
	glColor3f(0.2f, 0.2f, 0.2f);
	glBegin(GL_LINE_LOOP);
	glVertex2f(x0, y0);
	glVertex2f(x1, y0);
	glVertex2f(x1, y1);
	glVertex2f(x0, y1);
	glEnd();
	int gridSize = 10;
	glLineWidth(1.0f);
	glBegin(GL_LINES);
	for(int i = 1; i < gridSize; ++i) {
		if(i == gridSize/2) {
			glColor3f(0.2f, 0.2f, 0.2f);
		} else {
			glColor3f(0.8f, 0.8f, 0.8f);
		}
		float x = x0 + i / (float)gridSize * (x1 - x0);
		glVertex2f(x, y0);
		glVertex2f(x, y1);
	}
	for(int i = 1; i < gridSize; ++i) {
		if(i == gridSize/2) {
			glColor3f(0.2f, 0.2f, 0.2f);
		} else {
			glColor3f(0.8f, 0.8f, 0.8f);
		}
		float y = y0 + i / (float)gridSize * (y1 - y0);
		glVertex2f(x0, y);
		glVertex2f(x1, y);
	}
	glEnd();
	progSimple->unbind();
	
	// Draw shape
	progTex->bind();
	texture->bind(progTex->getUniform("texture0"), 0);
	glUniformMatrix4fv(progTex->getUniform("P"), 1, GL_FALSE, P->topMatrix().data());
   root_link->draw(MV.get(), progTex, shape);
	texture->unbind(0);
	progTex->unbind();
	
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
