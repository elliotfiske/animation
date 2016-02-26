import random, math
import numpy as np

with open("elliot_galaxy.txt", "w") as output:
	# n, h, whatever
	output.write("1502 0.0004 0.0001\n")

	# Big guy in center
	output.write("0.75 -2 -0.5 0 1 0 0 0.533884 0.9966563 0.742154 0.05\n")


	for lol in range(0, 500):
		radius = random.uniform(0.8, 1.2)
		pitch = random.uniform(0, 6.282)
		yaw   = random.uniform(0, 6.282)

		x = math.cos(yaw)*math.cos(pitch) * radius - 2
		y = math.sin(yaw)*math.cos(pitch) * radius - 0.5
		z = math.sin(pitch) * radius

		r = random.uniform(0, 0.5)
		g = random.uniform(0.2, 1)
		b = random.uniform(0.2, 1)

		output.write("0.0003 {0} {1} {2} 0 0 0 {3} {4} {5} 0.02\n".format(x, y, z, r, g, b))

	# # Big guy in center
	output.write("0.4 2 0.5 0 -2 0 0 0.9 0.9 0.9 0.05\n")

	cube_dim = 20
	z_vec = np.array([0, 0, 1])

	for x in range(0, cube_dim):
		for y in range(0, cube_dim):
			for z in range(0, cube_dim):
				real_x = float(x) / cube_dim
				real_y = float(y) / cube_dim
				real_z = float(z) / cube_dim

				pos_vec = np.array([real_x, real_y, real_z])
				vel_vec = np.cross(pos_vec, z_vec)

				velocity_x = vel_vec[0] * 12 - 6
				velocity_y = vel_vec[1] * 12 + 6
				velocity_z = 0#vel_vec[2] * 12

				my_r = float(x)/cube_dim
				my_g = float(y)/cube_dim
				my_b = float(z)/cube_dim

				output.write("0.01 {0} {1} {2} {6} {7} {8} {3} {4} {5} 0.02\n".format(real_x + 1.5, real_y, real_z - 0.5, my_r, my_b, my_g, velocity_x, velocity_y, velocity_z))

	# for lol in range(0, 25):
	# 	for new_r in range(0, 20):
	# 		real_r = float(new_r) / 10.0

	# 		pitch = random.uniform(0, 6.282)
	# 		yaw   = random.uniform(0, 6.282)

	# 		x = math.cos(yaw)*math.cos(pitch) * real_r  + 2
	# 		y = math.sin(yaw)*math.cos(pitch) * real_r + 0.5
	# 		z = math.sin(pitch) * real_r

	# 		output.write("0.0003 {0} {1} {2} 0 0 0 0 1 0 0.02\n".format(x, y, z))
