#version 120

varying vec3 fragNor;

void main()
{
	vec3 normal = normalize(fragNor);
	// Map normal in the range [-1, 1] to color in range [0, 1];
	vec3 color = 0.5*normal + 0.5;
	gl_FragColor = vec4(color, 1.0);
   gl_FragColor = vec4(fragNor, 1.0f);
}
