#include "Shape.h"
#include <iostream>

#define EIGEN_DONT_ALIGN_STATICALLY
#include <Eigen/Dense>
#include <Eigen/StdVector>

#include "GLSL.h"
#include "Program.h"
#include "Grid.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define NUM_BONES 18

using namespace std;
using namespace Eigen;

bool loaded_weights = false;
vector<float> skinning_weights;
std::vector<Eigen::Matrix4f,Eigen::aligned_allocator<Eigen::Matrix4f> > anim_frames;
std::vector<Eigen::Matrix4f,Eigen::aligned_allocator<Eigen::Matrix4f> > bind_pose;

// Stores which bones actually influence each vertex.
vector<vector<int> > valid_bones;

int num_frames = 0;
GLuint new_pos_buf_ID;

int k = 0;

Shape::Shape() :
	eleBufID(0),
	posBufID(0),
	norBufID(0),
	texBufID(0),
   weightBufID(0)
{
}

Shape::~Shape()
{
}

void Shape::loadMesh(const std::string &meshName, const std::string &resource_dir)
{
	// Load geometry
	// Some obj files contain material information.
	// We'll ignore them for this assignment.
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> objMaterials;
	string errStr;
	bool rc = tinyobj::LoadObj(shapes, objMaterials, errStr, meshName.c_str());
	if(!rc) {
		cerr << errStr << endl;
	} else {
		posBuf = shapes[0].mesh.positions;
		norBuf = shapes[0].mesh.normals;
		texBuf = shapes[0].mesh.texcoords;
		eleBuf = shapes[0].mesh.indices;
      
      // Create buffer for skinning weights
      if (!loaded_weights) {
         loaded_weights = true;
         skinning_weights = load_weights(resource_dir + "cheb_attachment.txt");
         bind_pose = load_animation(resource_dir + "cheb_skel_walk.txt");
         anim_frames = bind_pose;
         
         // Invert the bind pose matrices
         for (int ndx = 0; ndx < NUM_BONES; ndx++) {
            bind_pose[ndx] = bind_pose[ndx].inverse().eval();
         }
         
         num_frames = bind_pose.size() / NUM_BONES;
         
         for (int ndx = 0; ndx < posBuf.size(); ndx++) {
            valid_bones.push_back(vector<int>(18));
         }
         
         for (int ndx = 0; ndx < skinning_weights.size(); ndx++) {
            int curr_vertex = ndx / NUM_BONES;
            
            if (abs(skinning_weights[ndx]) > 0.001f) {
               valid_bones[curr_vertex].push_back(ndx % NUM_BONES);
            }
         }
      }
	}
}

Eigen::Matrix4f get_curr_anim() {
   k += 18;
   return anim_frames[k];
}

void Shape::init(const std::shared_ptr<Program> prog)
{
	// Send the position array to the GPU
	glGenBuffers(1, &posBufID);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_STATIC_DRAW);
   
   // Set up the skinning weights array
   glGenBuffers(1, &weightBufID);
   glBindBuffer(GL_ARRAY_BUFFER, weightBufID);
   glBufferData(GL_ARRAY_BUFFER, skinning_weights.size()*sizeof(float), &skinning_weights[0], GL_STATIC_DRAW);
	
	// Send the normal array to the GPU
	if(!norBuf.empty()) {
		glGenBuffers(1, &norBufID);
		glBindBuffer(GL_ARRAY_BUFFER, norBufID);
		glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_STATIC_DRAW);
	}
	
	// Send the texture array to the GPU
	if(!texBuf.empty()) {
		glGenBuffers(1, &texBufID);
		glBindBuffer(GL_ARRAY_BUFFER, texBufID);
		glBufferData(GL_ARRAY_BUFFER, texBuf.size()*sizeof(float), &texBuf[0], GL_STATIC_DRAW);
	}
	
	// Send the element array to the GPU
	glGenBuffers(1, &eleBufID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eleBufID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, eleBuf.size()*sizeof(unsigned int), &eleBuf[0], GL_STATIC_DRAW);
	
	// Unbind the arrays
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	GLSL::checkError(GET_FILE_LINE);
}

void Shape::do_cpu_skinning() const {
   // Modify the position buffer with our cute new skinnings and stuff
   vector<float> skinned_vertices;
   for (int i = 0; i < posBuf.size(); i += 3) {
      Vector4f orig_vertex;
      orig_vertex << posBuf[i], posBuf[i+1], posBuf[i+2], 1;
      
      Vector4f result_vertex; // Stores the result of adding up all the results
      // from multiplying orig_vertex by each bone
      result_vertex << 0, 0, 0, 0;
      
      vector<int> curr_bones = valid_bones[i/3];
      
      for (int ndx = 0; ndx < curr_bones.size(); ndx++) {
         int j = curr_bones[ndx];
         
         Vector4f weight_changed_vertex;
         weight_changed_vertex << orig_vertex.x(), orig_vertex.y(), orig_vertex.z(), 1;
         
         weight_changed_vertex = bind_pose[j] * weight_changed_vertex;
         weight_changed_vertex = anim_frames[(k+1)*NUM_BONES + j] * weight_changed_vertex;
         weight_changed_vertex *= skinning_weights[i/3*NUM_BONES + j];
         
         result_vertex += weight_changed_vertex;
      }
      
      skinned_vertices.push_back(result_vertex.x());
      skinned_vertices.push_back(result_vertex.y());
      skinned_vertices.push_back(result_vertex.z());
   }
   
   // Send the modified position array to the GPU
   glBindBuffer(GL_ARRAY_BUFFER, posBufID);
   glBufferData(GL_ARRAY_BUFFER, skinned_vertices.size()*sizeof(float), &skinned_vertices[0], GL_DYNAMIC_DRAW);
}

void Shape::do_gpu_skinning() const {
}

bool prev_cpu_skinning = true;
bool did_it = false;

void Shape::draw(const std::shared_ptr<Program> prog, bool cpu_skinning) const
{
   k++;
   k %= num_frames - 1;
   
   // Switching back to CPU skinning -> resend the vanilla vertices
   if ((!prev_cpu_skinning && cpu_skinning) || !did_it) {
      glBindBuffer(GL_ARRAY_BUFFER, posBufID);
      glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_STATIC_DRAW);
      GLSL::checkError(GET_FILE_LINE);
   }
   
   prev_cpu_skinning = cpu_skinning;
   
   if (cpu_skinning) {
      do_cpu_skinning();
   }
   else {
      // Send the bone positions and bind poses to the GPU
      glUniformMatrix4fv(prog->getUniform("BONE_POS"), 18, GL_FALSE, anim_frames[(k + 1) * NUM_BONES].data());
      glUniformMatrix4fv(prog->getUniform("BIND_BONE_POS"), 18, GL_FALSE, anim_frames[0].data());
      GLSL::checkError(GET_FILE_LINE);
      
      // Send the skinning weights to the GPU
      int h_weight0 = prog->getAttribute("weights0");
      int h_weight1 = prog->getAttribute("weights1");
      int h_weight2 = prog->getAttribute("weights2");
      int h_weight3 = prog->getAttribute("weights3");
      int h_weight4 = prog->getAttribute("weights4_nah");
      GLSL::enableVertexAttribArray(h_weight0);
      GLSL::enableVertexAttribArray(h_weight1);
      GLSL::enableVertexAttribArray(h_weight2);
      GLSL::enableVertexAttribArray(h_weight3);
      GLSL::enableVertexAttribArray(h_weight4);
      
      glBindBuffer(GL_ARRAY_BUFFER, weightBufID);
      unsigned stride = 18*sizeof(float); // TODO: in case you froget, change this back to 16 lawl
      
      glVertexAttribPointer(h_weight0, 4, GL_FLOAT, GL_FALSE, stride, (const void *)( 0  * sizeof(float) ));
      glVertexAttribPointer(h_weight1, 4, GL_FLOAT, GL_FALSE, stride, (const void *)( 4  * sizeof(float) ));
      glVertexAttribPointer(h_weight2, 4, GL_FLOAT, GL_FALSE, stride, (const void *)( 8  * sizeof(float) ));
      glVertexAttribPointer(h_weight3, 4, GL_FLOAT, GL_FALSE, stride, (const void *)( 12 * sizeof(float) ));
      glVertexAttribPointer(h_weight4, 2, GL_FLOAT, GL_FALSE, stride, (const void *)( 16 * sizeof(float) ));
   }
   
	// Bind position buffer
	int h_pos = prog->getAttribute("vertPos");
	GLSL::enableVertexAttribArray(h_pos);
   glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
	
	// Bind normal buffer
	int h_nor = prog->getAttribute("vertNor");
	if(h_nor != -1 && norBufID != 0) {
		GLSL::enableVertexAttribArray(h_nor);
		glBindBuffer(GL_ARRAY_BUFFER, norBufID);
		glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
	}
	
	// Bind texcoords buffer
	int h_tex = prog->getAttribute("vertTex");
	if(h_tex != -1 && texBufID != 0) {
		GLSL::enableVertexAttribArray(h_tex);
		glBindBuffer(GL_ARRAY_BUFFER, texBufID);
		glVertexAttribPointer(h_tex, 2, GL_FLOAT, GL_FALSE, 0, (const void *)0);
	}
	
	// Bind element buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eleBufID);
	
	// Draw
	glDrawElements(GL_TRIANGLES, (int)eleBuf.size(), GL_UNSIGNED_INT, (const void *)0);
	
	// Disable and unbind
	if(h_tex != -1) {
		GLSL::disableVertexAttribArray(h_tex);
	}
	if(h_nor != -1) {
		GLSL::disableVertexAttribArray(h_nor);
	}
	GLSL::disableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	GLSL::checkError(GET_FILE_LINE);
}
