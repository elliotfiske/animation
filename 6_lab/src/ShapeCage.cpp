#include <iostream>

#define EIGEN_DONT_ALIGN_STATICALLY
#include <Eigen/Dense>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "ShapeCage.h"
#include "GLSL.h"
#include "Grid.h"

using namespace std;
using namespace Eigen;

ShapeCage::ShapeCage() :
	posBufID(0),
	texBufID(0),
	elemBufID(0)
{
}

ShapeCage::~ShapeCage()
{
}

void ShapeCage::load(const string &meshName)
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
		elemBuf = shapes[0].mesh.indices;
	}
}

void ShapeCage::toLocal()
{
	// Find which tile each vertex belongs to.
	// Store (row, col) into tileIndexBuf.
	int nVerts = (int)posBuf.size()/3;
	posLocalBuf.resize(nVerts*2);
	tileIndexBuf.resize(nVerts);
	int nrows = grid->getRows();
	int ncols = grid->getCols();
	// Go through all vertices
	for(int k = 0; k < nVerts; ++k) {
		float x = posBuf[3*k];
		float y = posBuf[3*k+1];
      
      float u = 0;
      float v = 0;
      
      // Find which tile this vertex belongs to
      for(int col = 0; col < ncols-1; ++col) {
         for(int row = 0; row < nrows-1; ++row) {
            // Get the four control points for corresponding to (row, col)
            int tileIndex = grid->indexAt(row, col);
            vector<Vector2f> cps = grid->getTileCPs(tileIndex);
            
            float xmin = cps[0].x();
            float ymin = cps[0].y();
            
            float xmax = cps[3].x();
            float ymax = cps[3].y();
            
            if (x > xmin && x < xmax && y > ymin && y < ymax) {
               u = (x - xmin) / (xmax - xmin);
               v = (y - ymin) / (ymax - ymin);
               
               tileIndexBuf[k] = tileIndex;
            }
         }
      }
      
      vector<Eigen::Vector2f> cps = grid->getTileCPs(0);
      
      posLocalBuf[2*k+0] = u;
		posLocalBuf[2*k+1] = v;
	}
}

void ShapeCage::toWorld()
{
	int nVerts = (int)posBuf.size()/3;
	for(int k = 0; k < nVerts; ++k) {
		float u = posLocalBuf[2*k];
		float v = posLocalBuf[2*k+1];
      
      vector<Eigen::Vector2f> cps = grid->getTileCPs(tileIndexBuf[k]);
      
      Vector2f p_0 = (1 - u) * cps[0] + u * cps[1];
      Vector2f p_1 = (1 - u) * cps[2] + u * cps[3];
      
      Vector2f p_uv = (1 - v) * p_0 + v * p_1;
      
		//
		// IMPLEMENT ME
		// Fill in posBuf from the info stored in posLocalBuf and tileIndexBuf.
		//
		posBuf[3*k+0] = p_uv.x();
		posBuf[3*k+1] = p_uv.y();
	}
	// Send the updated world position array to the GPU
	glGenBuffers(1, &posBufID);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_DYNAMIC_DRAW);
}

void ShapeCage::init()
{
	// Send the texture coordinates array (if it exists) to the GPU
	if(!texBuf.empty()) {
		glGenBuffers(1, &texBufID);
		glBindBuffer(GL_ARRAY_BUFFER, texBufID);
		glBufferData(GL_ARRAY_BUFFER, texBuf.size()*sizeof(float), &texBuf[0], GL_STATIC_DRAW);
	} else {
		texBufID = 0;
	}
	
	// Send the index array to the GPU
	glGenBuffers(1, &elemBufID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBufID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, elemBuf.size()*sizeof(unsigned int), &elemBuf[0], GL_STATIC_DRAW);
	
	// Unbind the arrays
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	assert(glGetError() == GL_NO_ERROR);
}

void ShapeCage::draw(int h_pos, int h_tex) const
{
	// Enable and bind position array for drawing
	GLSL::enableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
	
	// Enable and bind texcoord array for drawing
	GLSL::enableVertexAttribArray(h_tex);
	glBindBuffer(GL_ARRAY_BUFFER, texBufID);
	glVertexAttribPointer(h_tex, 2, GL_FLOAT, GL_FALSE, 0, 0);
	
	// Bind element array for drawing
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBufID);
	
	// Draw
	int nElements = (int)elemBuf.size();
	glDrawElements(GL_TRIANGLES, nElements, GL_UNSIGNED_INT, 0);
	
	// Disable and unbind
	GLSL::disableVertexAttribArray(h_tex);
	GLSL::disableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
