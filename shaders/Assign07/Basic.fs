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
uniform float metallic;
uniform float roughness;
//pi cuz i need it
const float PI = 3.14159265359;

//function to calculate external reflection at incoming light angle 0
vec3 getFresnelAtAngleZero(vec3 albedo, float metallic){
	//default value for insulators 
	vec3 F0 = vec3(0.04);
	//interpolates between default F0 and albedo
	F0 = mix(F0, albedo, metallic);
	return F0;
}
//This function returns the Fresnel reflectance given 
//the light vector and half vector, assuming a starting value of F0 (i.e., RF(0)).
vec3 getFresnel(vec3 F0, vec3 L, vec3 H){
	float cosAngle = max(0.0, dot(L, H));
	//Schlick approximation
	F0 = F0 + (vec3(1.0)- F0) * pow(1.0 - cosAngle, 5.0);
	return F0;

}

float getNDF(vec3 H, vec3 N, float roughness) {
	
	float a = roughness * roughness; 
	float nhdot = dot(N,H);
	float a2 = a * a;

	

	return a2 / (PI * pow(pow(nhdot, 2.0) * (a2 - 1.0)+1, 2.0));

}

float getSchlickGeo(vec3 B, vec3 N, float roughness) {
	float k = pow(roughness + 1.0, 2.0) / 8;
	float calculated = dot(N,B) / (dot(N,B) * (1 - k) + k);
	return calculated;

}

float getGF(vec3 L, vec3 V, vec3 N, float roughness) {
	float GL = getSchlickGeo(L, N, roughness);
	float GV = getSchlickGeo(V, N, roughness);
	return GL*GV;
}




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
	//F0	
	vec3 F0 = getFresnelAtAngleZero(vec3(vertexColor), metallic);
	//Fresnel reflectance
	vec3 F = getFresnel(F0, L, H);
	//specular color
	vec3 kS = F;
	//diffuse color
	vec3 kD = 1.0 - kS;
	kD = kD * (1.0 - metallic);
	kD = kD * vec3(vertexColor);
	kD = kD/PI;

	//specular reflction
	float NDF = getNDF(H, N, roughness);
	float G = getGF(L, V, N, roughness);
	kS = kS * NDF * G;
	kS /= (4.0 * max(0, dot(N,L)) * max(0, dot(N,V))) + 0.0001;

	//final color
	vec3 finalColor = (kD + kS)*vec3(light.color)*max(0, dot(N,L));

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
	vec3 specColor = spec * tempColor * tempLight;

	

	// Just output int erpolated color
	//out_color = vec4(diffuseColor + specColor, 1.0);
	out_color = vec4(finalColor, 1.0);
}
