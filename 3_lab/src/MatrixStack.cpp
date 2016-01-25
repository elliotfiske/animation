#include "MatrixStack.h"

#include <stdio.h>
#include <cassert>
#include <vector>

#include <Eigen/Geometry>

using namespace std;
using namespace Eigen;

MatrixStack::MatrixStack()
{
	mstack = make_shared< stack<Matrix4f> >();
	mstack->push(Matrix4f::Identity());
}

MatrixStack::~MatrixStack()
{
}

void MatrixStack::pushMatrix()
{
	const Matrix4f &top = mstack->top();
	mstack->push(top);
	assert(mstack->size() < 100);
}

void MatrixStack::popMatrix()
{
	assert(!mstack->empty());
	mstack->pop();
	// There should always be one matrix left.
	assert(!mstack->empty());
}

void MatrixStack::loadIdentity()
{
	Matrix4f &top = mstack->top();
	top = Matrix4f::Identity();
}

void MatrixStack::translate(const Vector3f &trans)
{
	Matrix4f &top = mstack->top();
	Matrix4f E = Matrix4f::Identity();
	E(0,3) = trans(0);
	E(1,3) = trans(1);
	E(2,3) = trans(2);
	top *= E;
}

void MatrixStack::scale(const Vector3f &scale)
{
	Matrix4f &top = mstack->top();
	Matrix4f E = Matrix4f::Identity();
	E(0,0) = scale(0);
	E(1,1) = scale(1);
	E(2,2) = scale(2);
	top *= E;
}

void MatrixStack::scale(float s)
{
	Matrix4f &top = mstack->top();
	Matrix4f E = Matrix4f::Identity();
	E(0,0) = s;
	E(1,1) = s;
	E(2,2) = s;
	top *= E;
}

void MatrixStack::rotate(float angle, const Vector3f &axis)
{
	Matrix4f &top = mstack->top();
	Matrix4f E = Matrix4f::Identity();
	E.block<3,3>(0,0) = AngleAxisf(angle * M_PI / 180.0f, axis).toRotationMatrix();
	top *= E;
}

void MatrixStack::multMatrix(const Matrix4f &matrix)
{
	Matrix4f &top = mstack->top();
	top *= matrix;
}

void MatrixStack::ortho(float left, float right, float bottom, float top, float zNear, float zFar)
{
	assert(left != right);
	assert(bottom != top);
	assert(zFar != zNear);
	// Sets the top of the stack
	Matrix4f &M = mstack->top();
	M = Matrix4f::Zero();
	M(0,0) = 2.0f / (right - left);
	M(1,1) = 2.0f / (top - bottom);
	M(2,2) = -2.0f / (zFar - zNear);
	M(0,3) = - (right + left) / (right - left);
	M(1,3) = - (top + bottom) / (top - bottom);
	M(2,3) = - (zFar + zNear) / (zFar - zNear);
	M(3,3) = 1.0f;
}

void MatrixStack::ortho2D(float left, float right, float bottom, float top)
{
	ortho(left, right, bottom, top, -1.0, 1.0);
}

void MatrixStack::perspective(float fovy, float aspect, float zNear, float zFar)
{
	assert(fovy != 0.0f);
	assert(aspect != 0.0f);
	assert(zFar != zNear);
	// Sets the top of the stack
	Matrix4f &M = mstack->top();
	M = Matrix4f::Zero();
	float tanHalfFovy = tan(0.5f * fovy * M_PI / 180.0f);
	M(0,0) = 1.0f / (aspect * tanHalfFovy);
	M(1,1) = 1.0f / (tanHalfFovy);
	M(2,2) = -(zFar + zNear) / (zFar - zNear);
	M(2,3) = -(2.0f * zFar * zNear) / (zFar - zNear);
	M(3,2) = -1.0f;
}

void MatrixStack::frustum(float left, float right, float bottom, float top, float nearval, float farval)
{
	// http://cgit.freedesktop.org/mesa/mesa/tree/src/mesa/math/m_matrix.c
	float x, y, a, b, c, d;
	x = (2.0f*nearval) / (right-left);
	y = (2.0f*nearval) / (top-bottom);
	a = (right+left) / (right-left);
	b = (top+bottom) / (top-bottom);
	c = -(farval+nearval) / ( farval-nearval);
	d = -(2.0f*farval*nearval) / (farval-nearval);

	// Sets the top of the stack
	Matrix4f &M = mstack->top();
	M(0,0) = x;     M(0,1) = 0.0f;  M(0,2) = a;      M(0,3) = 0.0f;
	M(1,0) = 0.0f;  M(1,1) = y;     M(1,2) = b;      M(1,3) = 0.0f;
	M(2,0) = 0.0f;  M(2,1) = 0.0f;  M(2,2) = c;      M(2,3) = d;
	M(3,0) = 0.0f;  M(3,1) = 0.0f;  M(3,2) = -1.0f;  M(3,3) = 0.0f;
}

void MatrixStack::lookAt(const Vector3f &eye, const Vector3f &center, const Vector3f &up)
{
	// http://cgit.freedesktop.org/mesa/mesa/tree/src/glu/mesa/glu.c?h=mesa_3_2_dev
	Vector3f x, y, z;
	z = (eye - center).normalized();
	y = up;
	x = y.cross(z);
	y = z.cross(x);
	x.normalize();
	y.normalize();
	Matrix4f M = Matrix4f::Identity();
	M.block<1,3>(0,0) = x;
	M.block<1,3>(1,0) = y;
	M.block<1,3>(2,0) = z;
	multMatrix(M);
	translate(-eye);
}

const Matrix4f &MatrixStack::topMatrix() const
{
	return mstack->top();
}

void MatrixStack::print(const Matrix4f &mat, const char *name) const
{
	if(name) {
		printf("%s = [\n", name);
	}
	for(int i = 0; i < 4; ++i) {
		for(int j = 0; j < 4; ++j) {
			printf("%- 5.2f ", mat(i,j));
		}
		printf("\n");
	}
	if(name) {
		printf("];");
	}
	printf("\n");
}

void MatrixStack::print(const char *name) const
{
	print(mstack->top(), name);
}

// void MatrixStack::printStack() const
// {
// 	// Copy everything to a non-const stack
// 	auto tempStack = mstack;
// 	while(!tempStack.empty()) {
// 		Matrix4f &top = tempStack.top();
// 		print(top);
// 		tempStack.pop();
// 	}
// }

// #include <iostream>
// int main(int argc, char **argv)
// {
// 	MatrixStack M;
// 	M.frustum(-1, 1, -2, 2, 0.1, 10.0);
// 	std::cout << M.topMatrix() << std::endl;
// }
