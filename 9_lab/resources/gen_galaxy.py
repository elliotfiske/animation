import random, math

with open("elliot_galaxy.txt", "w") as output:
	# n, h, whatever
	output.write("1002 0.002 0.0001\n")

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
	output.write("0.03 2 0.5 0 -2 0 0 0.9 0.9 0.9 0.05\n")

	for x in range(0, 8):
		for y in range(0, 8):
			for z in range(0, 8):
				real_x = float(x) / 8 + 0.5 + 1
				real_y = float(y) / 8
				real_z = float(z) / 8 - 0.5

				output.write("0.03 {0} {1} {2} -2 0 0 {3} {4} {5} 0.02\n".format(real_x, real_y, real_z, float(x)/8, float(y)/8, float(z)/8))

	# for lol in range(0, 25):
	# 	for new_r in range(0, 20):
	# 		real_r = float(new_r) / 10.0

	# 		pitch = random.uniform(0, 6.282)
	# 		yaw   = random.uniform(0, 6.282)

	# 		x = math.cos(yaw)*math.cos(pitch) * real_r  + 2
	# 		y = math.sin(yaw)*math.cos(pitch) * real_r + 0.5
	# 		z = math.sin(pitch) * real_r

	# 		output.write("0.0003 {0} {1} {2} 0 0 0 0 1 0 0.02\n".format(x, y, z))
