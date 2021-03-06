#pragma once
#ifndef __Grid__
#define __Grid__

#include <vector>

#define EIGEN_DONT_ALIGN_STATICALLY
#include <Eigen/Dense>

class Grid
{
public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	
	Grid();
	virtual ~Grid();
	void setSize(int nrows, int ncols);
	void reset();
	void moveCP(const Eigen::Vector2f &p);
	void findClosest(const Eigen::Vector2f &p);
	void draw() const;
	
	int indexAt(int row, int col) const;
	int getRows() const { return nrows; };
	int getCols() const { return ncols; };
	std::vector<Eigen::Vector2f> getTileCPs(int index) const;
	const std::vector<Eigen::Vector2f> & getAllCPs() const;
	void save(const char *filename) const;
	void load(const char *filename);
	
private:
	
	std::vector<Eigen::Vector2f> cps;
	int nrows;
	int ncols;
	int closest;
};

#endif
