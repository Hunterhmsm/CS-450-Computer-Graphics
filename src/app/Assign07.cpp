#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <GL/glew.h>					
#include <GLFW/glfw3.h>
#include "glm/glm.hpp"
#include "MeshData.hpp"
#include "MeshGLData.hpp"
#include "GLSetup.hpp"
#include "Shader.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "glm/gtc/matrix_transform.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/transform.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "Utility.hpp"
using namespace std;


struct PointLight {
	glm::vec4 pos;
	glm::vec4 color;
};

PointLight light;

//global variables
float rotAngle = 0.0f;
glm::vec3 eye = glm::vec3(0,0,1);
glm::vec3 lookAt = glm::vec3(0,0,0);
glm::vec2 mousePos;
float metallic = 0.0;
float roughness = 0.1;

//function to generate a transformation to rotate around an arbitrary point and axis
glm::mat4 makeLocalRotate(glm::vec3 offset, glm::vec3 axis, float angle){
	//create transform matrix
	glm::mat4 transformationMatrix = glm::mat4(1.0f);
	//translate by negative offset
	glm::mat4 negOffset = glm::translate(transformationMatrix, -offset);
	//angle to radians
	float angleRadians = glm::radians(angle);
	//rotate angle around axis
	glm::mat4 rotation = glm::rotate(transformationMatrix, angleRadians, axis);
	//translate by offset
	glm::mat4 posOffset = glm::translate(transformationMatrix, offset);
	//combine
	glm::mat4 combine = posOffset * rotation * negOffset;
	return combine;
}
//mouse cursor callback
//lots of code used from ProfExercises
static void mouse_position_callback( GLFWwindow* window, double xpos, double ypos){
	//get mouse position
	glm::vec2 mousePosition = glm::vec2(xpos, ypos);
	//relative mouse motion
    glm::vec2 relMouse = mousePos - mousePosition;
	int fw, fh;
	//use frame buffer size to get width and height
    glfwGetFramebufferSize(window, &fw, &fh);
	if (fw > 0 && fh > 0){	
		float scaledx = (float)relMouse.x / fw;
		float scaledy = (float)relMouse.y / fh;
		//get camera direction
		glm::vec3 cameraDirection = glm::vec3(lookAt - eye);
		//get relative x, convert lookAt to vec4 to multiply then back to vec3
		glm::mat4 relativeX = makeLocalRotate(eye, glm::vec3(0,1,0), 30.0f * scaledx);
		//cross product of global y and camera direction
		lookAt = glm::vec3(relativeX * glm::vec4(lookAt, 1.0));
		glm::vec3 crossProduct = (glm::cross(cameraDirection, glm::vec3(0,1,0)));
		//relative y
		glm::mat4 relativeY = makeLocalRotate(eye, crossProduct, 30.0f * scaledy);
		lookAt = glm::vec3(relativeY * glm::vec4(lookAt, 1.0));
	}
	mousePos = glm::vec2(xpos, ypos);

}

glm::mat4 makeRotateZ(glm::vec3 offset){
	//create transform matrix
	glm::mat4 transformationMatrix = glm::mat4(1.0f);
	//translate by negative offset
	glm::mat4 negOffset = glm::translate(transformationMatrix, -offset);
	//translate to radians
	float rotAngleRadian = glm::radians(rotAngle);
	//rotates around z
	glm::mat4 rotation = glm::rotate(transformationMatrix, rotAngleRadian, glm::vec3(0.0f, 0.0f, 1.0f));
	//translate by offset
	glm::mat4 posOffset = glm::translate(transformationMatrix, offset);
	//composite
	glm::mat4 composite = posOffset * rotation * negOffset;
	//returns composite
	return composite;
}

void renderScene( vector<MeshGL> &allMeshes, aiNode *node, glm::mat4 parentMat, GLint modelMatLoc, int level, GLint normMatLoc, glm::mat4 viewMat) {
	//gets transformation for current node
	aiMatrix4x4 temp = node->mTransformation;
	//creates nodeT
	glm::mat4 nodeT;
	//convert to glm mat4 nodeT
	aiMatToGLM4(temp, nodeT);
	//compute current model matrix
	glm::mat4 modelMat = parentMat*nodeT;
	//gets location
	glm::vec4 currentPos = modelMat[3];
	//convert to vec3
	glm::vec3 currentPosVec3 = glm::vec3(currentPos);
	//make rotate to get proper local z rotation
	glm::mat4 R = makeRotateZ(currentPosVec3);
	//generate a temporary model matrix
	glm::mat4 tmpModel = R * modelMat;
	//pass in tmpmodel	
	glUniformMatrix4fv(modelMatLoc, 1, GL_FALSE,  glm::value_ptr(tmpModel));
	glm::mat3 normal = glm::transpose(glm::inverse(glm::mat3(viewMat * tmpModel)));
	glUniformMatrix3fv(normMatLoc, 1, GL_FALSE, glm::value_ptr(normal));
	//for each mesh in NODE
	for(int i = 0; i < node->mNumMeshes; i++){
		//get index of each mesh
		int index = node->mMeshes[i];
		//draw mesh
		drawMesh(allMeshes.at(index));
	}
	//for each child of the node
	for(int i = 0; i<node->mNumChildren; i++){
		//render scene
		renderScene(allMeshes, node->mChildren[i], modelMat, modelMatLoc, level + 1, normMatLoc, viewMat);
	}
}
//key press function, some code taken from ProfExercises
static void key_callback(GLFWwindow *window,
                        int key, int scancode,
                        int action, int mods){
		//speed variable
		float speed = 0.1f;
		//check for key presses	
	    if(action == GLFW_PRESS || action == GLFW_REPEAT) {
			//if escape is pressed close window
        	if(key == GLFW_KEY_ESCAPE) {
				glfwSetWindowShouldClose(window, true);
			}
			//if J is pressed add 1.0 to rot angle
			if(key == GLFW_KEY_J){
				rotAngle = rotAngle +  1.0;
			}
			//if K is pressed subtract 1.0 from rot angle
			if(key == GLFW_KEY_K){
				rotAngle = rotAngle - 1.0;
			}
			//movement keys, math is hard
			if(key == GLFW_KEY_W){
				glm::vec3 direction = glm::normalize(lookAt - eye);
				lookAt += direction * speed;
				eye += direction * speed;
			}
			if(key == GLFW_KEY_S){
				glm::vec3 direction = glm::normalize(lookAt - eye);
				lookAt -= direction * speed;
				eye -= direction * speed;
			}
			if(key == GLFW_KEY_D){
				glm::vec3 direction = glm::normalize(lookAt - eye);
				glm::vec3 localX = glm::normalize(glm::cross(direction, glm::vec3(0.0, 0.1, 0.0)));
				lookAt += localX * speed;
				eye += localX * speed;
			}
			if(key == GLFW_KEY_A){
				glm::vec3 direction = glm::normalize(lookAt - eye);
				glm::vec3 localX = glm::normalize(glm::cross(direction, glm::vec3(0.0, 0.1, 0.0)));
				lookAt -= localX * speed;
				eye -= localX * speed;
			//new keys to change colors
		}
			if(key == GLFW_KEY_1){
				light.color = glm::vec4(1,1,1,1);
			}
			if(key == GLFW_KEY_2){
				light.color = glm::vec4(1,0,0,1);
			}
			if(key == GLFW_KEY_3){
				light.color = glm::vec4(0,1,0,1);
			}
			if(key == GLFW_KEY_4){
				light.color = glm::vec4(0,0,1,1);
			}
			if(key == GLFW_KEY_V){
				metallic -= 0.1f;
				metallic = max(metallic, 0.0f);
				cout<<"metallic"<<metallic;
			}
			if(key == GLFW_KEY_B){
				metallic += 0.1f;
				metallic = min(metallic, 1.0f);
				cout<<"metallic"<<metallic;
			}
			if(key == GLFW_KEY_N){
				roughness -= 0.1f;
				roughness = max(roughness, 0.1f);
				cout<<"<<roughness"<<roughness;
			}
			if(key == GLFW_KEY_M){
				roughness += 0.1f;
				roughness = min(roughness, 0.7f);
				cout<<"<<roughness"<<roughness;
			}
	}
						}

void extractMeshData(aiMesh *mesh, Mesh &m) {
	//clear out vertices and faces
	m.vertices.clear();
	m.indices.clear();

	for (int i=0; i < mesh->mNumVertices; i++){
		//creates a vertex
		Vertex temp;
		//grabs position and converts it to glm vec3 hopefully
		glm::vec3 tempPos(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
		//grabs normals
		glm::vec3 normal(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
		//stores position in vector
		temp.position = tempPos;
		//stores normals in vector
		temp.normal = normal;
		//changes the vector color
		temp.color = glm::vec4(1,1,0,1);
		//adds the vertex to list
		m.vertices.push_back(temp);
		}
	
	for (int i=0; i < mesh->mNumFaces; i++){
		//grabs the AIface from mesh
		aiFace face = mesh->mFaces[i];
		//loops through face indices
		for (int j = 0; j < face.mNumIndices; j++){
			//adds index for each face to list
				m.indices.push_back(face.mIndices[j]);
		}
	}
}

void createSimplePentagon(Mesh &m) {
	// Clear out vertices and elements
	m.vertices.clear();
	m.indices.clear();

	// Create four corners
	Vertex upperLeft, upperRight;
	Vertex lowerLeft, lowerRight;
	Vertex fifth;

	// Set positions of vertices
	// Note: glm::vec3(x, y, z)
	upperLeft.position = glm::vec3(-0.5, 0.5, 0.0);
	upperRight.position = glm::vec3(0.5, 0.5, 0.0);
	lowerLeft.position = glm::vec3(-0.5, -0.5, 0.0);
	lowerRight.position = glm::vec3(0.5, -0.5, 0.0);
	fifth.position = glm::vec3(0.0, 0.9, 0.0);

	// Set vertex colors (red, green, blue, white)
	// Note: glm::vec4(red, green, blue, alpha)
	upperLeft.color = glm::vec4(1.0, 0.0, 0.0, 1.0);
	upperRight.color = glm::vec4(0.0, 1.0, 0.0, 1.0);
	lowerLeft.color = glm::vec4(0.0, 0.0, 1.0, 1.0);
	lowerRight.color = glm::vec4(1.0, 1.0, 1.0, 1.0);
	fifth.color = glm::vec4(0.2, 0.6, 0.3, 1.0);

	// Add to mesh's list of vertices
	m.vertices.push_back(upperLeft);
	m.vertices.push_back(upperRight);	
	m.vertices.push_back(lowerLeft);
	m.vertices.push_back(lowerRight);
	m.vertices.push_back(fifth);
	
	// Add indices for two triangles
	m.indices.push_back(0);
	m.indices.push_back(3);
	m.indices.push_back(1);

	m.indices.push_back(0);
	m.indices.push_back(2);
	m.indices.push_back(3);

	m.indices.push_back(0);
	m.indices.push_back(1);
	m.indices.push_back(4);
}



// Create very simple mesh: a quad (4 vertices, 6 indices, 2 triangles)
void createSimpleQuad(Mesh &m) {
	// Clear out vertices and elements
	m.vertices.clear();
	m.indices.clear();

	// Create four corners
	Vertex upperLeft, upperRight;
	Vertex lowerLeft, lowerRight;

	// Set positions of vertices
	// Note: glm::vec3(x, y, z)
	upperLeft.position = glm::vec3(-0.5, 0.5, 0.0);
	upperRight.position = glm::vec3(0.5, 0.5, 0.0);
	lowerLeft.position = glm::vec3(-0.5, -0.5, 0.0);
	lowerRight.position = glm::vec3(0.5, -0.5, 0.0);

	// Set vertex colors (red, green, blue, white)
	// Note: glm::vec4(red, green, blue, alpha)
	upperLeft.color = glm::vec4(1.0, 0.0, 0.0, 1.0);
	upperRight.color = glm::vec4(0.0, 1.0, 0.0, 1.0);
	lowerLeft.color = glm::vec4(0.0, 0.0, 1.0, 1.0);
	lowerRight.color = glm::vec4(1.0, 1.0, 1.0, 1.0);

	// Add to mesh's list of vertices
	m.vertices.push_back(upperLeft);
	m.vertices.push_back(upperRight);	
	m.vertices.push_back(lowerLeft);
	m.vertices.push_back(lowerRight);
	
	// Add indices for two triangles
	m.indices.push_back(0);
	m.indices.push_back(3);
	m.indices.push_back(1);

	m.indices.push_back(0);
	m.indices.push_back(2);
	m.indices.push_back(3);
}



// Main 
int main(int argc, char **argv) {
	
	// Are we in debugging mode?
	bool DEBUG_MODE = true;

		//sets model path
	string path = "sampleModels/sphere.obj";
	if (argc >= 2){
		path = argv[1];	
	}
	//all this code loads model and checks if it loaded
	Assimp::Importer importer;

	const aiScene *scene = importer.ReadFile(path,
		aiProcess_Triangulate | aiProcess_FlipUVs |
		aiProcess_GenNormals | aiProcess_JoinIdenticalVertices);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		cerr << "Error: " << importer.GetErrorString() << endl;
		exit(1);
			}
	//creates Vector of MeshGLs
	vector<MeshGL> myVector;
	


	// GLFW setup
	// Switch to 4.1 if necessary for macOS
	GLFWwindow* window = setupGLFW("Assign07: mcgiveh", 4, 3, 800, 800, DEBUG_MODE);

	//sets up key callback, taken from profexercises
	glfwSetKeyCallback(window, key_callback);
	// GLEW setup
	setupGLEW(window);

	//mousePos
	double mx, my;
	glfwGetCursorPos(window, &mx, &my);
	mousePos = glm::vec2(mx, my);
	glfwSetCursorPosCallback(window, mouse_position_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Check OpenGL version
	checkOpenGLVersion();

	// Set up debugging (if requested)
	if(DEBUG_MODE) checkAndSetupOpenGLDebugging();

	// Set the background color to a shade of blue
	glClearColor(0.8f, 0.2f, 0.8f, 1.0f);	

	// Create and load shaders
	GLuint programID = 0;
	try {		
		// Load vertex shader code and fragment shader code
		string vertexCode = readFileToString("./shaders/Assign07/Basic.vs");
		string fragCode = readFileToString("./shaders/Assign07/Basic.fs");

		// Print out shader code, just to check
		if(DEBUG_MODE) printShaderCode(vertexCode, fragCode);

		// Create shader program from code
		programID = initShaderProgramFromSource(vertexCode, fragCode);
	}
	catch (exception e) {		
		// Close program
		cleanupGLFW(window);
		exit(EXIT_FAILURE);
	}

	//sets light color and position
	light.pos = glm::vec4(0.5,0.5,0.5,1);
	light.color = glm::vec4(1,1,1,1);


	//gets model matrix location, code format taken from profexercises
	GLint modelMatLoc = glGetUniformLocation(programID, "modelMat");
	//view and projection matrix
	GLint viewMatLoc = glGetUniformLocation(programID, "viewMat");
    GLint projMatLoc = glGetUniformLocation(programID, "projMat");
	GLint lightColorLoc = glGetUniformLocation(programID, "light.color");
	GLint lightPosLoc = glGetUniformLocation(programID, "light.pos");
	GLint normMatLoc = glGetUniformLocation(programID, "normMat");
	GLint metallicMatLoc = glGetUniformLocation(programID, "metallic");
	GLint roughnessMatLoc = glGetUniformLocation(programID, "roughness");

		//loops through the meshes of the scene	
	for(int i = 0; i < scene->mNumMeshes; i++){
		//creates a mesh object
		Mesh tempMesh;
		//creates a MeshGL object
		MeshGL tempGL;
		//grabs mesh data
		extractMeshData(scene->mMeshes[i], tempMesh);
		//grabs MeshGL Data
		createMeshGL(tempMesh, tempGL);
		//adds to vector
		myVector.push_back(tempGL);	

	}
	
	// Create simple quad
	//Mesh m;
	//createSimplePentagon(m);

	// Create OpenGL mesh (VAO) from data
	//MeshGL mgl;
	//createMeshGL(m, mgl);
	
	// Enable depth testing
	glEnable(GL_DEPTH_TEST);

	while (!glfwWindowShouldClose(window)) {
		// Set viewport size
		int fwidth, fheight;
		glfwGetFramebufferSize(window, &fwidth, &fheight);
		glViewport(0, 0, fwidth, fheight);

		// Clear the framebuffer	
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use shader program
		glUseProgram(programID);

		//pass in metallic and roughness
		glUniform1f(metallicMatLoc, metallic);
		glUniform1f(roughnessMatLoc, roughness);

		//view matrix
		glm::mat4 viewMatrix = glm::lookAt(eye, lookAt, glm::vec3(0,1,0));
		//pass view matrix to  shader
		glUniformMatrix4fv(viewMatLoc, 1, false, glm::value_ptr(viewMatrix));
		//frame buffer w and h
		int fw, fh;
		glfwGetFramebufferSize(window, &fw, &fh);
		//aspect ratio = Frame buffer w/h
		float aspectRatio = 1.0f;
		//if w or h are 0 aspect ratio = 0
		if((fw > 0)&&(fh > 0)){
			aspectRatio = ((float)fw)/((float)fh);
		}
		glm::mat4 projMatrix = glm::perspective(glm::radians(90.0f), aspectRatio, 0.01f, 50.0f);
		glUniformMatrix4fv(projMatLoc, 1, false, glm::value_ptr(projMatrix));
		
		// Draw object
		//drawMesh(mgl);
		//for (int i = 0; i < myVector.size(); i++)
			//drawMesh(myVector[i]);
		//calls render scene

		//calculates position of light in eye/view space

		glm::vec4 lightView = viewMatrix * light.pos;

		glUniform4fv(lightPosLoc, 1, glm::value_ptr(lightView));
		glUniform4fv(lightColorLoc, 1, glm::value_ptr(light.color));


		renderScene(myVector, scene->mRootNode, glm::mat4(1.0), modelMatLoc, 0, normMatLoc, viewMatrix);

		// Swap buffers and poll for window events		
		glfwSwapBuffers(window);
		glfwPollEvents();

		// Sleep for 15 ms
		this_thread::sleep_for(chrono::milliseconds(15));
	} 

	// Clean up mesh
	//cleanupMesh(mgl);
	for (int i = 0; i < myVector.size(); i++)
			cleanupMesh(myVector[i]);
	myVector.clear();

	// Clean up shader programs
	glUseProgram(0);
	glDeleteProgram(programID);
		
	// Destroy window and stop GLFW
	cleanupGLFW(window);

	return 0;
}
