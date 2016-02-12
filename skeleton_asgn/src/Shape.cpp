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
vector<float>   valid_bones;
vector<vector<int> > multid_valid_bones;
vector<float> num_bones_for_vertex;
vector<float> gpu_skinning_weights;

int num_frames = 0;
GLuint new_pos_buf_ID;

int k = 0;

Shape::Shape() :
	eleBufID(0),
	posBufID(0),
	norBufID(0),
	texBufID(0),
   weightBufID(0),
   numBoneBufID(0),
   boneNdxBufID(0)
{
}

Shape::~Shape()
{
}

/* Process the skinning weight and animation frame data we got */
void Shape::processData(const std::string &resource_dir) {
   loaded_weights = true;
   skinning_weights = load_weights(resource_dir + "cheb_attachment.txt");
   bind_pose = load_animation(resource_dir + "cheb_skel_walk.txt");
   anim_frames = bind_pose;
   
   // Invert the bind pose matrices
   for (int ndx = 0; ndx < NUM_BONES; ndx++) {
      bind_pose[ndx] = bind_pose[ndx].inverse().eval();
   }
   
   num_frames = bind_pose.size() / NUM_BONES;
   
   vector<vector<float> > multid_gpu_skinning_weights;
   
   // Populate the "valid bones" array and set up the corresponding gpu_skinning_weights
   for (int ndx = 0; ndx < posBuf.size() / 3; ndx++) {
      multid_valid_bones.push_back(vector<int>());
      multid_gpu_skinning_weights.push_back(vector<float>());
   }
   
   for (int ndx = 0; ndx < skinning_weights.size(); ndx++) {
      int curr_vertex = ndx / NUM_BONES;
      
      if (abs(skinning_weights[ndx]) > 0.001f) {
         multid_valid_bones[curr_vertex].push_back(ndx % NUM_BONES);
         multid_gpu_skinning_weights[curr_vertex].push_back(skinning_weights[ndx]);
      }
   }
   
   // Pad out the valid_bones so that it always has 15 bone-dices for each vertex
   for (int ndx = 0; ndx < posBuf.size() / 3; ndx++) {
      num_bones_for_vertex.push_back(multid_valid_bones[ndx].size());
      
      while (multid_valid_bones[ndx].size() < 15) {
         multid_valid_bones[ndx].push_back(0);
         multid_gpu_skinning_weights[ndx].push_back(0);
      }
   }
   
   // Convert the multid vectors into a 1d vector
   for (int vert_ndx = 0; vert_ndx < multid_valid_bones.size(); vert_ndx++) {
      for (int bone_ndx = 0; bone_ndx < 15; bone_ndx++) {
         valid_bones.push_back(multid_valid_bones[vert_ndx][bone_ndx]);
         gpu_skinning_weights.push_back(multid_gpu_skinning_weights[vert_ndx][bone_ndx]);
      }
   }
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
         processData(resource_dir);
      }
	}
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
   glBufferData(GL_ARRAY_BUFFER, gpu_skinning_weights.size()*sizeof(float), &gpu_skinning_weights[0], GL_STATIC_DRAW);
   
   // Send the bone-number array to the GPU
   glGenBuffers(1, &numBoneBufID);
   glBindBuffer(GL_ARRAY_BUFFER, numBoneBufID);
   glBufferData(GL_ARRAY_BUFFER, num_bones_for_vertex.size()*sizeof(float), &num_bones_for_vertex[0], GL_STATIC_DRAW);

   // Send the bone-ndx array to the GPU
   glGenBuffers(1, &boneNdxBufID);
   glBindBuffer(GL_ARRAY_BUFFER, boneNdxBufID);
   glBufferData(GL_ARRAY_BUFFER, valid_bones.size()*sizeof(float), &valid_bones[0], GL_STATIC_DRAW);
	
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
      
      vector<int> curr_bones = multid_valid_bones[i/3];
      
      for (int ndx = 0; ndx < curr_bones.size(); ndx++) {
         int j = curr_bones[ndx];
         
         Vector4f weight_changed_vertex;
         weight_changed_vertex << orig_vertex.x(), orig_vertex.y(), orig_vertex.z(), 1;
          
         weight_changed_vertex = bind_pose[j] * weight_changed_vertex;
         weight_changed_vertex = anim_frames[(k+1)*NUM_BONES + j] * weight_changed_vertex;
         weight_changed_vertex *= gpu_skinning_weights[ndx];
         
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

void Shape::do_gpu_skinning(const std::shared_ptr<Program> prog) const {
   int h_weight0, h_weight1, h_weight2, h_weight3;
   int h_bones0, h_bones1, h_bones2, h_bones3;
   int h_num_bones;
   
   // Send the bone positions and bind poses to the GPU
   glUniformMatrix4fv(prog->getUniform("BONE_POS"), 18, GL_FALSE, anim_frames[(k + 1) * NUM_BONES].data());
   glUniformMatrix4fv(prog->getUniform("BIND_BONE_POS"), 18, GL_FALSE, bind_pose[0].data());
   GLSL::checkError(GET_FILE_LINE);
   
   // Tell the GPU how to interpret my skinning weights
   h_weight0 = prog->getAttribute("weights0");
   h_weight1 = prog->getAttribute("weights1");
   h_weight2 = prog->getAttribute("weights2");
   h_weight3 = prog->getAttribute("weights3");
   GLSL::enableVertexAttribArray(h_weight0);
   GLSL::enableVertexAttribArray(h_weight1);
   GLSL::enableVertexAttribArray(h_weight2);
   GLSL::enableVertexAttribArray(h_weight3);
   
   glBindBuffer(GL_ARRAY_BUFFER, weightBufID);
   unsigned stride = 15*sizeof(float);
   
   glVertexAttribPointer(h_weight0, 4, GL_FLOAT, GL_FALSE, stride, (const void *)( 0  * sizeof(float) ));
   glVertexAttribPointer(h_weight1, 4, GL_FLOAT, GL_FALSE, stride, (const void *)( 4  * sizeof(float) ));
   glVertexAttribPointer(h_weight2, 4, GL_FLOAT, GL_FALSE, stride, (const void *)( 8  * sizeof(float) ));
   glVertexAttribPointer(h_weight3, 4, GL_FLOAT, GL_FALSE, stride, (const void *)( 12 * sizeof(float) ));
   
   
   // Tell the GPU how to interpret my bone indices
   h_bones0 = prog->getAttribute("bones0");
   h_bones1 = prog->getAttribute("bones1");
   h_bones2 = prog->getAttribute("bones2");
   h_bones3 = prog->getAttribute("bones3");
   GLSL::enableVertexAttribArray(h_bones0);
   GLSL::enableVertexAttribArray(h_bones1);
   GLSL::enableVertexAttribArray(h_bones2);
   GLSL::enableVertexAttribArray(h_bones3);
   
   glBindBuffer(GL_ARRAY_BUFFER, boneNdxBufID);
   // stride the same
   
   glVertexAttribPointer(h_bones0, 4, GL_FLOAT, GL_FALSE, stride, (const void *)( 0  * sizeof(float) ));
   glVertexAttribPointer(h_bones1, 4, GL_FLOAT, GL_FALSE, stride, (const void *)( 4  * sizeof(float) ));
   glVertexAttribPointer(h_bones2, 4, GL_FLOAT, GL_FALSE, stride, (const void *)( 8  * sizeof(float) ));
   glVertexAttribPointer(h_bones3, 4, GL_FLOAT, GL_FALSE, stride, (const void *)( 12 * sizeof(float) ));
   
   // Tell the GPU how to interpret my num_bones vertex array
   h_num_bones = prog->getAttribute("num_bones");
   GLSL::enableVertexAttribArray(h_num_bones);
   glBindBuffer(GL_ARRAY_BUFFER, numBoneBufID);
   glVertexAttribPointer(h_num_bones, 1, GL_FLOAT, GL_FALSE, 0, 0);
}

bool prev_cpu_skinning = true;
void Shape::disableVertexAttribs(const std::shared_ptr<Program> prog) const {
   int h_weight0, h_weight1, h_weight2, h_weight3;
   int h_bones0, h_bones1, h_bones2, h_bones3;
   int h_num_bones;
   
   h_weight0 = prog->getAttribute("weights0");
   h_weight1 = prog->getAttribute("weights1");
   h_weight2 = prog->getAttribute("weights2");
   h_weight3 = prog->getAttribute("weights3");
   h_bones0 = prog->getAttribute("bones0");
   h_bones1 = prog->getAttribute("bones1");
   h_bones2 = prog->getAttribute("bones2");
   h_bones3 = prog->getAttribute("bones3");
   h_num_bones = prog->getAttribute("num_bones");
   
   GLSL::disableVertexAttribArray(h_weight0);
   GLSL::disableVertexAttribArray(h_weight1);
   GLSL::disableVertexAttribArray(h_weight2);
   GLSL::disableVertexAttribArray(h_weight3);
   GLSL::disableVertexAttribArray(h_bones0);
   GLSL::disableVertexAttribArray(h_bones1);
   GLSL::disableVertexAttribArray(h_bones2);
   GLSL::disableVertexAttribArray(h_bones3);
   GLSL::disableVertexAttribArray(h_num_bones);
}

bool did_it = false;

void Shape::draw(const std::shared_ptr<Program> prog, bool cpu_skinning) const
{
   k++;
   k %= num_frames - 1;
   
   if ((!prev_cpu_skinning && cpu_skinning) || !did_it) {
      glBindBuffer(GL_ARRAY_BUFFER, posBufID);
      glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_STATIC_DRAW);
      GLSL::checkError(GET_FILE_LINE);
   }
   
   prev_cpu_skinning = cpu_skinning;
   
   if (cpu_skinning) {
      do_cpu_skinning();
   }
   
   do_gpu_skinning(prog);
   
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
   
   disableVertexAttribs(prog);

	GLSL::disableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	GLSL::checkError(GET_FILE_LINE);
}
