#pragma once
#ifndef _SHAPE_H_
#define _SHAPE_H_

#include <string>
#include <vector>
#include <memory>
#include <Eigen/Dense>

class Program;

Eigen::Matrix4f get_curr_anim();

class Shape
{
public:
	Shape();
	virtual ~Shape();
   void loadMesh(const std::string &meshName, const std::string &resource_dir);
	void init(const std::shared_ptr<Program> prog);
   void draw(const std::shared_ptr<Program> prog, bool cpu_skinning) const;
	
private:
	std::vector<unsigned int> eleBuf;
	std::vector<float> posBuf;
	std::vector<float> norBuf;
	std::vector<float> texBuf;
	unsigned eleBufID;
	unsigned posBufID;
	unsigned norBufID;
	unsigned texBufID;
   unsigned weightBufID;
   
   void do_cpu_skinning() const;
   void do_gpu_skinning() const;
};

#endif
