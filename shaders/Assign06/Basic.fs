#version 430 core
// Change to 410 for macOS

layout(location=0) out vec4 out_color;
 
in vec4 vertexColor; // Now interpolated across face

in vec4 interPos;
in vec3 interNormal;

struct PointLight {
vec4 pos;
vec4 color;
};

uniform PointLight light;


void main()
{		

	//normalize
	vec3 N = normalize(interNormal);
	//calculate light vector L
	vec3 L = normalize(vec3(light.pos - interPos));
	//view vector
	vec3 V = normalize(vec3(-interPos));
	//half vector
	vec3 H = normalize(L + V);
	//vec3 conversions
	vec3 tempColor = vec3(vertexColor);
	vec3 tempLight = vec3(light.color);


	//diffuse coefficient
	float diffuse = max(0.0, dot(N, L));
	
	//color
	vec3 diffuseColor = vec3(diffuse * vertexColor * light.color);
	//shiny
	float shiny = 10.0;
	//specular coefficient
	float spec = max(0.0, dot(N, L)) * pow(max(0.0, dot(N, H)), shiny);
	//spec color
	vec3 specColor = spec * tempLight;

	

	// Just output int erpolated color
	out_color = vec4(diffuseColor + specColor, 1.0);

}
