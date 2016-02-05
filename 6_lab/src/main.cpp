#include <iostream>
#include <vector>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "ShapeCage.h"
#include "Texture.h"
#include "Grid.h"

using namespace std;
using namespace Eigen;

GLFWwindow *window; // Main application window
string RESOURCE_DIR = ""; // Where the resources are loaded from
shared_ptr<Program> progSimple;
shared_ptr<Program> progTex;
shared_ptr<Grid> grid;
shared_ptr<ShapeCage> shape;
shared_ptr<Texture> texture;

bool keyToggles[256] = {false}; // only for English keyboards!

Vector2f window2world(double xmouse, double ymouse)
{
	// Convert from window coords to world coords
	// (Assumes orthographic projection)
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	Eigen::Vector4f p;
	// Inverse of viewing transform
	p(0) = xmouse / (float)width;
	p(1) = (height - ymouse) / (float)height;
	p(0) = 2.0f * (p(0) - 0.5f);
	p(1) = 2.0f * (p(1) - 0.5f);
	p(2) = 0.0f;
	p(3) = 1.0f;
	// Inverse of model-view-projection transform
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
	double aspect = (double)width/height;
	double s = 1.1;
	P->ortho2D(-s*aspect, s*aspect, -s, s);
	p = (P->topMatrix() * MV->topMatrix()).inverse() * p;
	return p.segment<2>(0);
}

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

	switch(key) {
		case 'r':
			grid->reset();
			break;
		case 's':
			grid->save("cps.txt");
			break;
		case 'l':
			grid->load("cps.txt");
			break;
	}
}

static void cursor_position_callback(GLFWwindow* window, double xmouse, double ymouse)
{
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if(state == GLFW_PRESS) {
		grid->moveCP(window2world(xmouse, ymouse));
	} else {
		grid->findClosest(window2world(xmouse, ymouse));
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// Not used for this lab
}

static void init()
{
	GLSL::checkVersion();
	
	// Set background color
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// Enable z-buffer test
	glEnable(GL_DEPTH_TEST);
	
	// Initialize the GLSL program.
	progSimple = make_shared<Program>();
	progSimple->setVerbose(true); // Set this to true when debugging.
	progSimple->setShaderNames(RESOURCE_DIR + "simple_vert.glsl", RESOURCE_DIR + "simple_frag.glsl");
	progSimple->init();
	progSimple->addUniform("P");
	progSimple->addUniform("MV");

	progTex = make_shared<Program>();
	progTex->setVerbose(true); // Set this to true when debugging.
	progTex->setShaderNames(RESOURCE_DIR + "tex_vert.glsl", RESOURCE_DIR + "tex_frag.glsl");
	progTex->init();
	progTex->addAttribute("vertPos");
	progTex->addAttribute("vertTex");
	progTex->addUniform("P");
	progTex->addUniform("MV");
	progTex->addUniform("colorTexture");

	texture = make_shared<Texture>();
	texture->setFilename(RESOURCE_DIR + "wood_tex.jpg");
	texture->init();

	// Grid of control points
	grid = make_shared<Grid>();
	grid->setSize(5, 5);

	// Load scene
	shape = make_shared<ShapeCage>();
	shape->load(RESOURCE_DIR + "man.obj");
	shape->setGrid(grid);
	shape->toLocal();
	shape->init();

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
	
	// Clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if(keyToggles[(unsigned)'c']) {
		glEnable(GL_CULL_FACE);
	} else {
		glDisable(GL_CULL_FACE);
	}
	if(keyToggles[(unsigned)'z']) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
	P->pushMatrix();
	MV->pushMatrix();
	
	double aspect = (double)width/height;
	double s = 1.1;
	P->ortho2D(-s*aspect, s*aspect, -s, s);
	
	// Draw cage
	progSimple->bind();
	glUniformMatrix4fv(progSimple->getUniform("P"), 1, GL_FALSE, P->topMatrix().data());
	glUniformMatrix4fv(progSimple->getUniform("MV"), 1, GL_FALSE, MV->topMatrix().data());
	grid->draw();
	progSimple->unbind();

	// Draw textured shape
	progTex->bind();
	texture->bind(progTex->getUniform("colorTexture"), 0);
	glUniformMatrix4fv(progTex->getUniform("P"), 1, GL_FALSE, P->topMatrix().data());
	glUniformMatrix4fv(progTex->getUniform("MV"), 1, GL_FALSE, MV->topMatrix().data());
	shape->toWorld();
	shape->draw(progTex->getAttribute("vertPos"), progTex->getAttribute("vertTex"));
	texture->unbind(0);
	progTex->unbind();

	// Pop matrix stacks.
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
