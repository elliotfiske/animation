#pragma once
#ifndef _SHAPECAGE_H_
#define _SHAPECAGE_H_

#include <memory>
#include <vector>

class Grid;

class ShapeCage
{
public:
	ShapeCage();
	virtual ~ShapeCage();
	void load(const std::string &meshName);
	void setGrid(std::shared_ptr<Grid> g) { grid = g; }
	void toLocal();
	void toWorld();
	void init();
	void draw(int h_pos, int h_tex) const;
	
private:
	std::vector<unsigned int> elemBuf;
	std::vector<float> posBuf;
	std::vector<float> norBuf;
	std::vector<float> texBuf;
	std::vector<float> posLocalBuf;
	std::vector<float> tileIndexBuf;
	unsigned posBufID;
	unsigned texBufID;
	unsigned elemBufID;
	std::shared_ptr<Grid> grid;
};

#endif
