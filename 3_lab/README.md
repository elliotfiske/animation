Lab 3 - Interpolation of Rigid Transforms
=========================================

Today we're going to linearly interpolate between two rigid transforms. Also,
the Stanford bunny is going to make its first appearance in the course.

Please download the code for the lab and go over the code.

- On the web: <http://users.csc.calpoly.edu/~ssueda/teaching/CSC474/2016W/labs/L03/L03.zip>
- On the server: `cp ~ssueda/www/teaching/CSC474/2016W/labs/L03/L03.zip .`

There are a few more source files for this lab compared to previous labs.
Please be sure you understand what the code is doing — how vertex buffers
work, how shaders work, etc. Also, note how I am drawing the axis at the
origin. Old-school OpenGL is useful for these types of things.

Task 1
------

When you first run the code, you'll see a bunny. However, it is not centered
at the origin. (That's just how the vertex positions in bunny.obj were
defined. In general, when you download a mesh file from the Internet, there is
no guarantee that the origin is at the center of the object.) The center of
this bunny is at `(-0.2802, 0.932, 0.0851)`. Use the center variable in the
code to place the bunny at the origin. To do this, you should use the
`translate()` function of the MatrixStack class by calling `MV.translate()`.
If you have forgotten what the matrix stack does, make sure to cache it back
into your memory. Use the provided positions, `p0` and `p1`, to draw two
bunnies. Remember to use `MV.push()` and `MV.pop()` appropriately. Then use
the provided `alpha` variable to draw a linearly interpolated bunny. You
should see a bunny translating between the source and target bunnies.

<img src="images/image1.jpg" width="200px"/>
<img src="images/image2.jpg" width="200px"/>
<img src="images/image3.jpg" width="200px"/>

Task 2
------

The two quaternion variables, `q0` and `q1`, define the rotations of the
source and target bunnies. Note how they are being constructed from axis
angles. The first quaternion corresponds to a rotation by $+90^{\circ}$ about
`axis0`, and the second quaternion is $+90^{\circ}$ about `axis1`. Use these
quaternions to rotate the two bunnies:

	Matrix4f R = Matrix4f::Identity();
	R.block<3,3>(0,0) = q0.toRotationMatrix();

Use the resulting 4x4 matrix, `R`, to modify the top element of the matrix
stack by calling `MV.multMatrix()`. Now you should be able to press the x, y,
z, X, Y, and Z keys to rotate the two bunnies. *Each bunny should rotate about
its center point.* The lowercased letters change the axis of rotation of the
source bunny, and the uppercased letters change the target bunny. Now add
rotation interpolation using the slerp() function built into the quaternion
class:

	R.block<3,3>(0,0) = q0.slerp(alpha, q1).toRotationMatrix();

Summary: In this code, we're using three different representations of 3D
orientations. First, we use axis angle, since it is the most intuitive
geometrically. Then we convert this into a quaternion, since we are
interpolating. Finally, we convert this into a matrix so that we can apply it
to the matrix stack.

<img src="images/image4.jpg" width="300px"/>
<img src="images/image5.jpg" width="300px"/>
