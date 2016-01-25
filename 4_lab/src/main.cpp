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
Vector2f cameraRotations(0, 0);
Vector2f mousePrev(-1, -1);

// Control points
vector<Vector3f> cps;

enum SplineType
{
	CATMULL_ROM = 0,
	BASIS,
	SPLINE_TYPE_COUNT
};

SplineType type = CATMULL_ROM;

Matrix4f Bcr, Bb;

vector<pair<float,float> > usTable;

void buildTable();

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
				buildTable();
				break;
			case GLFW_KEY_C:
				cps.clear();
				buildTable();
				break;
			case GLFW_KEY_R:
				cps.clear();
				int n = 8;
				for(int i = 0; i < n; ++i) {
					float alpha = i / (n - 1.0f);
					float angle = 2.0f * M_PI * alpha;
					float radius = cos(2.0f * angle);
					Vector3f cp;
					cp(0) = radius * cos(angle);
					cp(1) = radius * sin(angle);
					cp(2) = (1.0f - alpha)*(-0.5) + alpha*0.5;
					cps.push_back(cp);
				}
				buildTable();
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
			float aspect = (float)width/height;
			Vector4f x;
			x(0) = 2.0f * ((xmouse / width) - 0.5f)* aspect;
			x(1) = 2.0f * (((height - ymouse) / height) - 0.5f);
			x(2) = 0.0f;
			x(3) = 1.0f;
			// Build the current modelview matrix.
			auto MV = make_shared<MatrixStack>();
			MV->rotate(cameraRotations(1), Vector3f(1, 0, 0));
			MV->rotate(cameraRotations(0), Vector3f(0, 1, 0));
			// Since the modelview matrix transforms from world to eye coords,
			// we want to invert to go from eye to world.
			x = MV->topMatrix().inverse() * x;
			cps.push_back(x.segment<3>(0));
			buildTable();
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
	prog->setVerbose(false); // Set this to true when debugging.
	prog->setShaderNames(RESOURCE_DIR + "simple_vert.glsl", RESOURCE_DIR + "simple_frag.glsl");
	prog->init();
	prog->addUniform("P");
	prog->addUniform("MV");
	
	// Initialize time.
	glfwSetTime(0.0);
	
	keyToggles[(unsigned)'l'] = true;
	
	Bcr << 0.0f, -1.0f,  2.0f, -1.0f,
		   2.0f,  0.0f, -5.0f,  3.0f,
		   0.0f,  1.0f,  4.0f, -3.0f,
		   0.0f,  0.0f, -1.0f,  1.0f;
	Bcr *= 0.5;
	
	Bb << 1.0f, -3.0f,  3.0f, -1.0f,
		  4.0f,  0.0f, -6.0f,  3.0f,
		  1.0f,  3.0f,  3.0f, -3.0f,
		  0.0f,  0.0f,  0.0f,  1.0f;
	Bb /= 6.0f;

	// If there were any OpenGL errors, this will print something.
	// You can intersperse this line in your code to find the exact location
	// of your OpenGL error.
	GLSL::checkError(GET_FILE_LINE);
}

#define MAX_SAMPLES 5

Vector4f uvecify(float u) {
   return Vector4f(0.0f, 1.0f, 2*u, 3*u*u);
}

void buildTable()
{
	usTable.clear();
   
   int ncps = cps.size();
   
   // Get the G and B matrices
   MatrixXf G(3, cps.size());
   MatrixXf Gk(3,4);
   Matrix4f B = (type == CATMULL_ROM ? Bcr : Bb);
   for(int i = 0; i < cps.size(); ++i) {
      G.block<3,1>(0,i) = cps[i];
   }
   
   float total_dist = 0.0f;
   
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

         Vector3f p_prime_1 = Gk * B * u_prime_1;
         Vector3f p_prime_2 = Gk * B * u_prime_2;
         Vector3f p_prime_3 = Gk * B * u_prime_3;
         
         float w1 = 5.0f/9.0f;
         float w2 = 8.0f/9.0f;
         float w3 = 5.0f/9.0f;
         
         float s = ubua_2 * (
            w1*p_prime_1.norm() + w2*p_prime_2.norm() + w3*p_prime_3.norm()
         );
         
         usTable.push_back(make_pair(ua + k, total_dist));
         
         total_dist += s;
      }
   }
}

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
	//double t = glfwGetTime();
	
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
	P->ortho(-aspect, aspect, -1.0, 1.0, -2.0, 2.0);
	MV->rotate(cameraRotations(1), Vector3f(1, 0, 0));
	MV->rotate(cameraRotations(0), Vector3f(0, 1, 0));
	
	// Bind the program
	prog->bind();
	glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, P->topMatrix().data());
	glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, MV->topMatrix().data());
	
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
	if(ncps >= 4) {
		// Draw spline
		MatrixXf G(3,ncps);
		MatrixXf Gk(3,4);
		Matrix4f B = (type == CATMULL_ROM ? Bcr : Bb);
		for(int i = 0; i < ncps; ++i) {
			G.block<3,1>(0,i) = cps[i];
		}
		glLineWidth(3.0f);
		for(int k = 0; k < ncps - 3; ++k) {
			int n = 32; // curve discretization
			// Gk is the 3x4 block starting at column k
			Gk = G.block<3,4>(0,k);
			glBegin(GL_LINE_STRIP);
			if(k % 2 == 0) {
				// Even segment color
				glColor3f(0.0f, 1.0f, 0.0f);
			} else {
				// Odd segment color
				glColor3f(0.0f, 0.0f, 1.0f);
			}
			for(int i = 0; i < n; ++i) {
				// u goes from 0 to 1 within this segment
				float u = i / (n - 1.0f);
				// Compute spline point at u
				Vector4f uVec(1.0f, u, u*u, u*u*u);
				Vector3f P = Gk * B * uVec;
				glVertex3fv(P.data());
			}
			glEnd();
		}
		
		// Draw equally spaced points on the spline curve
		if(keyToggles[(unsigned)'a'] && !usTable.empty()) {
			float ds = 0.2;
			glColor3f(1.0f, 0.0f, 0.0f);
			glPointSize(10.0f);
			glBegin(GL_POINTS);
			float smax = usTable.back().second; // spline length
			for(float s = 0.0f; s < smax; s += ds) {
				// Convert from s to (concatenated) u
				float uu = s2u(s);
				// Convert from concatenated u to the usual u between 0 and 1.
				float kfloat;
				float u = std::modf(uu, &kfloat);
				// k is the index of the starting control point
				int k = (int)std::floor(kfloat);
				// Gk is the 3x4 block starting at column k
				Gk = G.block<3,4>(0,k);
				// Compute spline point at u
				Vector4f uVec(1.0f, u, u*u, u*u*u);
				Vector3f P = Gk * B * uVec;
				glVertex3fv(P.data());
			}
			glEnd();
		}
	}

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
