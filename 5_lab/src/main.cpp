#include <iostream>
#include <vector>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"

using namespace std;
using namespace Eigen;

GLFWwindow *window; // Main application window
string RESOURCE_DIR = ""; // Where the resources are loaded from
shared_ptr<Program> prog;

bool keyToggles[256] = {false}; // only for English keyboards!
Vector2f mouse;

Vector4f coeffs0;
Vector4f coeffs1;
float xmid;

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
	double s = 0.6;
	P->ortho2D(-s*aspect, s*aspect, -s, s);
	MV->translate(Vector3f(-0.5, -0.5, 0.0));
	p = (P->topMatrix() * MV->topMatrix()).inverse() * p;
	mouse = p.segment<2>(0);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// Not used for this lab
}

static void init()
{
	GLSL::checkVersion();
	
	// Set background color
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	// Enable z-buffer test
	glEnable(GL_DEPTH_TEST);
	
	// Initialize the GLSL program.
	prog = make_shared<Program>();
	prog->setVerbose(false); // Set this to true when debugging.
	prog->setShaderNames(RESOURCE_DIR + "simple_vert.glsl", RESOURCE_DIR + "simple_frag.glsl");
	prog->init();
	prog->addUniform("P");
	prog->addUniform("MV");
	
	//
	// Compute the coefficients here
	//
   
   /*
    f0(0.0)=0.0, 
    f′0(0.0)=0.0,
    f0(0.4)=f1(0.4), 
    f′0(0.4)=f′1(0.4), 
    f1(0.5)=0.2, 
    f′1(0.5)=0.0,
    f1(1.0)=1.0, 
    f′1(1.0)=0.0.
   */
   float m_3 = 0.4f * 0.4f * 0.4f;
   float m_2 = 0.4f * 0.4f;
   float m_1 = 0.4f;
   
   float x_3 = 0.5f * 0.5f * 0.5f;
   float x_2 = 0.5f * 0.5f;
   float x_1 = 0.5f;

   
   VectorXf b(8);
   b << 0.0f, 0.0f, 0.0f, 0.0f,    0.2f, 0.0f, 1.0f, 0.0f;
   
   MatrixXf A(8, 8);
   A << 0.0f, 0.0f, 0.0f, 1.0f,    0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,    0.0f, 0.0f, 0.0f, 0.0f,
   
        m_3,  m_2,  m_1,  1.0f,    -m_3, -m_2, -m_1, -1.0f,
        3*m_2, 2*m_1, 1.0f, 0.0f,   -3*m_2, -2*m_1, -1.0f, 0.0f,
   
        0.0f, 0.0f, 0.0f, 0.0f,     x_3,  x_2,  x_1,  1.0f,
        0.0f, 0.0f, 0.0f, 0.0f,     3*x_2, 2*x_1, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,     1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 0.0f,     3.0f, 2.0f, 1.0f, 0.0f;
   
   VectorXf c = A.colPivHouseholderQr().solve(b);
   
	xmid = 0.4f;
//	coeffs0 << -2.0f, 3.0f, 0.0f, 0.0f;
//	coeffs1 << -2.0f, 3.0f, 0.0f, 0.0f;
   
   coeffs0 << c(0), c(1), c(2), c(3);
   coeffs1 << c(4), c(5), c(6), c(7);

	// If there were any OpenGL errors, this will print something.
	// You can intersperse this line in your code to find the exact location
	// of your OpenGL error.
	GLSL::checkError(GET_FILE_LINE);
}

#define NUM_SAMPLES 100
#define TANGENT_LENGTH 0.14f

Vector4f xvecify(float x) {
   return Vector4f(1.0f, x, x, x);
}

float calcCubic(float x, const Vector4f& coeffs) {
   return coeffs(0) * x * x * x +
          coeffs(1) * x * x     +
          coeffs(2) * x         +
          coeffs(3);
}

float calcDXCubic(float x, const Vector4f& coeffs) {
   return 3 * coeffs(0) * x * x +
          2 * coeffs(1) * x     +
          1 * coeffs(2);
}

void drawCubics() {
   glLineWidth(1.0f);
   
   glBegin(GL_LINE_STRIP);
   
   glColor3f(1.0f, 0.0f, 0.0f);
   
   float cubic1start = 0.0f;
   float cubic2start = xmid;
   float cubic2end   = 1.0f;
   for (int x = 0; x < NUM_SAMPLES; x++) {
      float x_norm = (float) x / NUM_SAMPLES;
      float real_x = cubic1start * (1 - x_norm) + cubic2start * x_norm;
      
      glVertex2f(real_x, calcCubic(real_x, coeffs0));
   }
   
   glColor3f(0.0f, 1.0f, 0.0f);
   
   for (int x = 0; x < NUM_SAMPLES; x++) {
      float x_norm = (float) x / NUM_SAMPLES;
      float real_x = cubic2start * (1 - x_norm) + cubic2end * x_norm;
      
      glVertex2f(real_x, calcCubic(real_x, coeffs1));
   }
   glEnd();
   
   glBegin(GL_LINE_STRIP);
   
   glColor3f(1.0f, 1.0f, 1.0f);
   float mx = mouse(0);
   float my = 0;
   float dy_dx = 0;
   
   if (mx >= cubic1start && mx <= cubic2start) {
      my = calcCubic(mx, coeffs0);
      dy_dx = calcDXCubic(mx, coeffs0);
   }
   else if (mx <= cubic2end) {
      my = calcCubic(mx, coeffs1);
      dy_dx = calcDXCubic(mx, coeffs1);
   }
   
   float dx = 1 / (dy_dx + 1);
   float dy = 1 - dx;
   
   Vector2f tangent_vec(dx, dy);
   tangent_vec.normalize();
   tangent_vec *= TANGENT_LENGTH;
   
   glVertex2f(mx - tangent_vec(0), my - tangent_vec(1));
   glVertex2f(mx + tangent_vec(0), my + tangent_vec(1));
   
   glEnd();
}

void render()
{
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
	
	double aspect = (double)width/height;
	double s = 0.6;
	P->ortho2D(-s*aspect, s*aspect, -s, s);
	MV->translate(Vector3f(-0.5, -0.5, 0.0));
	
	// Bind the program
	prog->bind();
	glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, P->topMatrix().data());
	glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, MV->topMatrix().data());
	
	// Draw grid
	int gridSize = 5;
	glLineWidth(2.0f);
	glColor3f(0.2f, 0.2f, 0.2f);
	glBegin(GL_LINES);
	for(int i = 1; i < gridSize; ++i) {
		float x = i / (float)gridSize;
		glVertex2f(x, 0.0f);
		glVertex2f(x, 1.0f);
	}
	for(int i = 1; i < gridSize; ++i) {
		float y = i / (float)gridSize;
		glVertex2f(0.0f, y);
		glVertex2f(1.0f, y);
	}
	glEnd();
	glLineWidth(4.0f);
	glColor3f(0.8f, 0.8f, 0.8f);
	glBegin(GL_LINE_LOOP);
	glVertex2f(0.0f, 0.0f);
	glVertex2f(1.0f, 0.0f);
	glVertex2f(1.0f, 1.0f);
	glVertex2f(0.0f, 1.0f);
	glEnd();

   drawCubics();

	// Unbind the program
	prog->unbind();

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
