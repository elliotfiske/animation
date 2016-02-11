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

using namespace std;

bool keyToggles[256] = {false}; // only for English keyboards!

GLFWwindow *window; // Main application window
string RESOURCE_DIR = ""; // Where the resources are loaded from

shared_ptr<Program> prog;
shared_ptr<Camera> camera;
shared_ptr<Shape> wobbler;

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
   prog->addUniform("gpu_rendering");
	prog->addAttribute("vertPos");
   prog->addAttribute("vertNor");
   prog->addAttribute("vertTex");
   
   // New stuff
   prog->addAttribute("weights0");
   prog->addAttribute("weights1");
   prog->addAttribute("weights2");
   prog->addAttribute("weights3");
   prog->addAttribute("weights4_nah");
	
   prog->addAttribute("bones0");
   prog->addAttribute("bones1");
   prog->addAttribute("bones2");
   prog->addAttribute("bones3");
   
   prog->addAttribute("num_bones");
   
   prog->addUniform("BONE_POS");
   prog->addUniform("BIND_BONE_POS");
   
   prog->setVerbose(true);
   
   wobbler = make_shared<Shape>();
	wobbler->loadMesh(RESOURCE_DIR + "cheb2.obj", RESOURCE_DIR);
	wobbler->init(prog);
	
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
	
	// Send projection matrix (same for all bunnies)
	glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, P->topMatrix().data());
	
	// Apply some transformations to the modelview matrix.
	// Each bunny should get a different transformation.
	
	// The center of the bunny is at (-0.2802, 0.932, 0.0851)
	Eigen::Vector3f center(-0.2802, 0.932, 0.0851);
	
	// Alpha is the linear interpolation parameter between 0 and 1
	float alpha = std::fmod(t, 1.0f);
	
	// The axes of rotatio for the source and target bunnies
	Eigen::Vector3f axis0, axis1;
	axis0(0) = keyToggles[(unsigned)'x'] ? 1.0 : 0.0f;
	axis0(1) = keyToggles[(unsigned)'y'] ? 1.0 : 0.0f;
	axis0(2) = keyToggles[(unsigned)'z'] ? 1.0 : 0.0f;
	axis1(0) = keyToggles[(unsigned)'X'] ? 1.0 : 0.0f;
	axis1(1) = keyToggles[(unsigned)'Y'] ? 1.0 : 0.0f;
	axis1(2) = keyToggles[(unsigned)'Z'] ? 1.0 : 0.0f;
	if(axis0.norm() > 0.0f) {
		axis0.normalize();
	}
	if(axis1.norm() > 0.0f) {
		axis1.normalize();
	}
	
	MV->pushMatrix();
//   MV->multMatrix(get_curr_anim());
	glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, MV->topMatrix().data());
	MV->popMatrix();
   
   bool cpu_skinning = keyToggles[(unsigned) 'g'];
   
   GLSL::checkError(GET_FILE_LINE);
   glUniform1i(prog->getUniform("gpu_rendering"), cpu_skinning ? 0 : 1);
   GLSL::checkError(GET_FILE_LINE);
   
	wobbler->draw(prog, cpu_skinning);
	
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
