#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Camera.h"
#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "Particle.h"
#include "Texture.h"

using namespace std;
using namespace Eigen;

bool keyToggles[256] = {false}; // only for English keyboards!

GLFWwindow *window; // Main application window
string RESOURCE_DIR = ""; // Where the resources are loaded from

shared_ptr<Program> progSimple;
shared_ptr<Program> prog;
shared_ptr<Camera> camera;
vector< shared_ptr<Particle> > particles;
shared_ptr<Texture> texture;
double t, h, e2;

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

static void initGL()
{
	GLSL::checkVersion();
	
	// Set background color
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	// Enable z-buffer test
	glEnable(GL_DEPTH_TEST);
	// Enable alpha blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	progSimple = make_shared<Program>();
	progSimple->setShaderNames(RESOURCE_DIR + "simple_vert.glsl", RESOURCE_DIR + "simple_frag.glsl");
	progSimple->setVerbose(false); // Set this to true when debugging.
	progSimple->init();
	progSimple->addUniform("P");
	progSimple->addUniform("MV");
	
	prog = make_shared<Program>();
	prog->setVerbose(true); // Set this to true when debugging.
	prog->setShaderNames(RESOURCE_DIR + "particle_vert.glsl", RESOURCE_DIR + "particle_frag.glsl");
	prog->init();
	prog->addUniform("P");
	prog->addUniform("MV");
	prog->addAttribute("vertPos");
	prog->addAttribute("vertTex");
	prog->addUniform("radius");
	prog->addUniform("alphaTexture");
	prog->addUniform("color");
	
	texture = make_shared<Texture>();
	texture->setFilename(RESOURCE_DIR + "alpha.jpg");
	texture->init();
	
	camera = make_shared<Camera>();
	
	// Initialize OpenGL for particles.
	for(auto p : particles) {
		p->init();
	}
	
	// If there were any OpenGL errors, this will print something.
	// You can intersperse this line in your code to find the exact location
	// of your OpenGL error.
	GLSL::checkError(GET_FILE_LINE);
}

// Sort particles by their z values in camera space
class ParticleSorter {
public:
	bool operator()(size_t i0, size_t i1) const
	{
		// Particle positions in world space
		const Vector3d &x0 = particles[i0]->getPosition();
		const Vector3d &x1 = particles[i1]->getPosition();
		// Particle positions in camera space
		float z0 = V.row(2) * Vector4f(x0(0), x0(1), x0(2), 1.0f);
		float z1 = V.row(2) * Vector4f(x1(0), x1(1), x1(2), 1.0f);
		return z0 < z1;
	}
	
	Matrix4f V; // current view matrix
};
ParticleSorter sorter;

// http://stackoverflow.com/questions/1577475/c-sorting-and-keeping-track-of-indexes
template <typename T>
vector<size_t> sortIndices(const vector<T> &v) {
	// initialize original index locations
	vector<size_t> idx(v.size());
	for (size_t i = 0; i != idx.size(); ++i) idx[i] = i;
	// sort indexes based on comparing values in v
	sort(idx.begin(), idx.end(), sorter);
	return idx;
}

void renderGL()
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
	P->pushMatrix();
	camera->applyProjectionMatrix(P);
	MV->pushMatrix();
	camera->applyViewMatrix(MV);
	// Set view matrix for the sorter
	sorter.V = MV->topMatrix();
	
	// Draw particles
	prog->bind();
	texture->bind(prog->getUniform("alphaTexture"), 0);
	glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, P->topMatrix().data());
	// Sort particles by Z for transparency rendering.
	// Since we don't want to modify the contents of the vector, we compute the
	// sorted indices and traverse the particles in this sorted order.
	for(auto i : sortIndices(particles)) {
		particles[i]->draw(prog, MV);
	}
	texture->unbind(0);
	prog->unbind();
	
	//////////////////////////////////////////////////////
	// Cleanup
	//////////////////////////////////////////////////////
	
	// Pop stacks
	MV->popMatrix();
	P->popMatrix();
	
	GLSL::checkError(GET_FILE_LINE);
}

void saveParticles(const char *filename)
{
	ofstream out(filename);
	if(!out.good()) {
		cout << "Could not open " << filename << endl;
		return;
	}
	
	// 1st line:
	// <n> <h> <e2>
	out << particles.size() << " " << h << " " << " " << e2 << endl;

	// Rest of the lines:
	// <mass> <position> <velocity> <color> <radius>
	
	//
	// IMPLEMENT ME
	//
	
	out.close();
	cout << "Wrote galaxy to " << filename << endl;
}

void loadParticles(const char *filename)
{
	ifstream in;
	in.open(filename);
	if(!in.good()) {
		cout << "Cannot read " << filename << endl;
		return;
	}

	// 1st line:
	// <n> <h> <e2>
	int n;
	in >> n;
	in >> h;
	in >> e2;

	// Rest of the lines:
	// <mass> <position> <velocity> <color> <radius>
   double mass, radius;
   double x, y, z;
   float  xf, yf, zf;
   Vector3d position;
   Vector3d velocity;
   Vector3f color;
   
	
   for (int ndx = 0; ndx < n; ndx++) {
      in >> mass;
      
      // parse position
      in >> x;
      in >> y;
      in >> z;
      position << x, y, z;
      
      // parse velocity
      in >> x;
      in >> y;
      in >> z;
      velocity << x, y, z;
      
      // parse color
      in >> xf;
      in >> yf;
      in >> zf;
      color << xf, yf, zf;
      
      in >> radius;
      auto p = make_shared<Particle>();
      p->setMass(mass);
      p->setPosition(position);
      p->setVelocity(velocity);
      p->setColor(color);
      p->setRadius(radius);
      
      particles.push_back(p);
   }

	in.close();
	cout << "Loaded galaxy from " << filename << endl;
}

void createParticles()
{
	srand(0);
	t = 0.0;
	h = 1.0;
   e2 = 1e-4;
   
   double r = 1.0;
   double a = 2.0;
   
   auto heavy = make_shared<Particle>();
   heavy->setMass(1e-3);
   heavy->setPosition(Vector3d(0, 0, 0));
   heavy->setVelocity(Vector3d(0, 0, 0));
   
   double y = sqrt(heavy->getMass() * (2/r - 1/a));
   
   auto light = make_shared<Particle>();
   light->setMass(1e-6);
   light->setPosition(Vector3d(r, 0, 0));
   light->setVelocity(Vector3d(0, y, 0));
   
   particles.push_back(heavy);
   particles.push_back(light);
}

#define G 1.0f

/** TODO: PARALLELIZE */
void stepParticles()
{
   // Stores the components of each particle's summed forces
   vector<Vector3d> forces;
   
   for (int ndx = 0; ndx < particles.size(); ndx++) {
      Vector3d curr_force(0, 0, 0);
      forces.push_back(curr_force);
      
      // Compute forces from every other particle
      for (int other_ndx = 0; other_ndx < particles.size(); other_ndx++) {
         if (other_ndx == ndx) {
            continue; // Don't influence yourself, you'll go blind
         }
         
         double mi = particles[ndx]->getMass();
         double mj = particles[other_ndx]->getMass();
         
         Vector3d diff = particles[other_ndx]->getPosition() - particles[ndx]->getPosition();
         
         // this is the scalar junk underneath the 'Gmimj' term
         double scalar_divisor = pow ( diff.norm() * diff.norm() + e2, 3.0/2.0);
         
         Vector3d curr_result = G * mi * mj / scalar_divisor * diff;
         forces[ndx] += curr_result;
      }
   }
   
   // Apply forces to particles
   for (int ndx = 0; ndx < particles.size(); ndx++) {
      // Update velocity
      double h_m_inverse = h * 1/particles[ndx]->getMass();
      Vector3d new_velocity = particles[ndx]->getVelocity() + h_m_inverse * forces[ndx];
      
      particles[ndx]->setVelocity(new_velocity);
      
      // Update position
      Vector3d new_position = particles[ndx]->getPosition() + h * new_velocity;
      particles[ndx]->setPosition(new_position);
   }
}

int main(int argc, char **argv)
{
	if(argc != 2 && argc != 3) {
		// Wrong number of arguments
		cout << "Usage: Lab09 <RESOURCE_DIR> <(OPTIONAL) INPUT FILE>" << endl;
		cout << "   or: Lab09 <#steps>       <(OPTIONAL) INPUT FILE>" << endl;
		exit(0);
	}
	// Create the particles...
	if(argc == 2) {
		// ... without input file
		createParticles();
	} else {
		// ... with input file
		loadParticles(argv[2]);
	}
	try {
		// Try parsing `steps`
		int steps = stoi(argv[1]);
		// Success!
		cout << "Running without OpenGL for " << steps << " steps" << endl;
		// Run without OpenGL
		for(int k = 0; k < steps; ++k) {
			stepParticles();
		}
      
      cout << "Particle positions: " << endl;
      for (int ndx = 0; ndx < particles.size(); ndx++) {
         Vector3d posn = particles[ndx]->getPosition();
         cout << ndx << ": " << "(" << posn.x() << ", " << posn.y() << ", " << posn.z() << ")" << endl;
      }
      
	} catch(const invalid_argument& ia) {
		// `steps` could not be parsed
		cout << "Running with OpenGL" << endl;
		// Run with OpenGL until the window is closed
		RESOURCE_DIR = argv[1] + string("/");
		// Set error callback.
		glfwSetErrorCallback(error_callback);
		// Initialize the library.
		if(!glfwInit()) {
			return -1;
		}
		// Create a windowed mode window and its OpenGL context.
		window = glfwCreateWindow(640, 480, "YOUR NAME", NULL, NULL);
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
		initGL();
		// Loop until the user closes the window.
		while(!glfwWindowShouldClose(window)) {
			// Step simulation.
			stepParticles();
			// Render scene.
			renderGL();
			// Swap front and back buffers.
			glfwSwapBuffers(window);
			// Poll for and process events.
			glfwPollEvents();
		}
		// Quit program.
		glfwDestroyWindow(window);
		glfwTerminate();
	}
	cout << "Elapsed time: " << (t*3.261539827498732e6) << " years" << endl;
	return 0;
}
