#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Camera.h"
#include "Shader.h"
#include "Arrow.h"


// Collidable Attributes
struct Collidable
{
	glm::vec3 normal;
	float d;
	std::string name;
};

void frame_buffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouseMoveEvent(GLFWwindow* window, double posX, double posY);
void mouseButtonHandler(GLFWwindow* window, int button, int action, int modes);
void mouseScrollEvent(GLFWwindow* window, double xOffset, double yOffset);
std::vector<float> drawUnitCircle(float radius);
void generateSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices, float radius);
void checkImageFormat(const int& channel);
void sendDataToCard(unsigned int& VAO, const std::vector<float>& vertices, int stride);
void sendDataToCard(unsigned int& VAO, const std::vector<float>& vertices, const std::vector<unsigned int>& indices, int stride);
static int computeDartboardScore(float distanceFromCenter, float radius);
bool intersectSegmentPlane(glm::vec3 start, glm::vec3 end, glm::vec3 normal, float d, glm::vec3& hitPoint);
void setCollidables(std::vector<Collidable>& collidables);
bool sweptPointAABB(glm::vec3 start, glm::vec3 velocity, glm::vec3 minB, glm::vec3 maxB, float& tHit, glm::vec3& hitNormal);

// Screen Size Settings
int width = 1000;
int height = 800;

// Camera Settings
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
bool firstMouse = true;
bool mouseLock = false;
float lastX = width / 2.0f;
float lastY = height / 2.0f;

// Timing Settings
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ground attributes
const float groundSize = 21.0f;

// wall attributes
const float wallOffset = groundSize - 2.5f;
const float wallSize = groundSize;

// Arrow Settings
glm::vec3 initialArrowPos = camera.Position;
glm::vec3 arrowPos;
float projectionAngle;
float initialSpeed = 100.0f;
float lastArrowX;
bool releaseArrow = false;
float speedScale = 1.0f;
float timeScale = 1.0f;
bool arrowReachGround = false;
bool arrowReachWall = false;
bool arrowStuck = false;
glm::vec3 velocity = glm::vec3(0.0f);
bool firstTime;
float speedFactor = 0.2;
bool arrowStuckInTarget = false;


// Motion and Gravity
float g = 9.8;


// Light Attributes
glm::vec3 lightOffset = glm::vec3(0.0f, 15.0f, 0.0f);
glm::vec3 lightPos;
glm::vec3 lightColor = glm::vec3(1.0f);
glm::vec3 diffuseColor = lightColor * glm::vec3(0.7f);
glm::vec3 ambientColor = diffuseColor * glm::vec3(0.2f);
glm::vec3 specularColor = glm::vec3(1.0f);


// Dartboard size (world units)
float dartboardRadius = groundSize-20;
// scoring
glm::vec3 prevArrowPos = glm::vec3(0.0f);
int lastThrowScore = 0;
bool throwing = false;


// Image format
unsigned int format;

// Target Attributes
glm::vec3 targetOffset = glm::vec3(0.0f, 7.0f, -10.0f);
glm::vec3 targetPos = glm::vec3(0.0f, 0.0f, 0.0f);

// Level Settings
bool level1 = true;
bool level2 = false;
bool level3 = false;
bool level4 = false;

// Target Path Attributes
const float phi1 = glm::radians(60.0f);
const float phi2 = glm::radians(5.0f);
const float phi3 = glm::radians(30.0f);
const float r1 = 5.0f;
const float r2 = 6.0f;
const float r3 = 7.0f;


int main() {
	if (!glfwInit()) {
		std::cout << "Failed to initialize glfw!!!" << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(width, height, "Jamie King", NULL, NULL);

	if (window == NULL) {
		std::cout << "Window was not initialized!!!" << std::endl;
		glfwTerminate();
		exit(1);
	}


	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, frame_buffer_size_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialzie glad!!!" << std::endl;
		return -1;
	}
	glfwSetCursorPosCallback(window, mouseMoveEvent);
	glfwSetMouseButtonCallback(window, mouseButtonHandler);
	glfwSetScrollCallback(window, mouseScrollEvent);
	glEnable(GL_DEPTH_TEST);

	Shader groundShader("vertexShader.glsl", "fragmentShader.glsl");
	groundShader.use();

	// Setting the material properties that react with light
	groundShader.setInt("material.diffuse", 0);

	// Setting the light properties -> often called energy of light, also no specular componet needed cause our ground is not shiny
	groundShader.setVec3("light.ambient", ambientColor);
	groundShader.setVec3("light.diffuse", diffuseColor);

	// make sure shader uses texture unit 0 for its sampler
	groundShader.use();
	groundShader.setInt("ourTexture", 0);

	// enable alpha blending for textured quads (dartboard PNG may have transparency)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	std::vector<float> Ground = {
		// Position								// Normal					// Texture
		-groundSize, -1.0f, groundSize,		 0.0f, 1.0f, 0.0f,				0.0f, groundSize,			// left top
																					
		groundSize, -1.0f, -groundSize,		 0.0f, 1.0f, 0.0f,				groundSize, 0.0f,			// right bottom
																					
		-groundSize, -1.0f, -groundSize,	 0.0f, 1.0f, 0.0f,				0.0f, 0.0f,					// left bottom
																					
		groundSize, -1.0f, groundSize, 		 0.0f, 1.0f, 0.0f,				groundSize, groundSize,		// right top

	};

	std::vector <unsigned int> corners = {
		0, 2, 1,
		0, 1, 3,
	};

	GLuint groundVAO;

	glGenVertexArrays(1, &groundVAO);
	glBindVertexArray(groundVAO);

	sendDataToCard(groundVAO, Ground, corners, 8);

	GLint textureWidth, textureHeight, channel;
	GLuint groundTexture;
	glGenTextures(1, &groundTexture);
	glBindTexture(GL_TEXTURE_2D, groundTexture);

	// Let's define wrapping and filtering parameters for texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


	unsigned char* data = stbi_load("./resources/floor.jpg", &textureWidth, &textureHeight, &channel, 0);
	if (data) {
		checkImageFormat(channel);
		glTexImage2D(GL_TEXTURE_2D, 0, format, textureWidth, textureHeight, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else {
		std::cout << "Failed to read texture file..." << std::endl;
	}

	stbi_image_free(data);

	GLuint textureWall;
	glGenTextures(1, &textureWall);
	glBindTexture(GL_TEXTURE_2D, textureWall);

	// Let's define wrapping and filtering parameters for texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


	data = stbi_load("./resources/wall.jpg", &textureWidth, &textureHeight, &channel, 0);
	if (data) {
		checkImageFormat(channel);
		glTexImage2D(GL_TEXTURE_2D, 0, format, textureWidth, textureHeight, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else {
		std::cout << "Failed to read texture file..." << std::endl;
	}


	stbi_image_free(data);

	GLuint textureRoof;
	glGenTextures(1, &textureRoof);
	glBindTexture(GL_TEXTURE_2D, textureRoof);

	// Let's define wrapping and filtering parameters for texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


	data = stbi_load("./resources/metal_grate_rusty_diff_4k.jpg", &textureWidth, &textureHeight, &channel, 0);
	if (data) {
		checkImageFormat(channel);
		glTexImage2D(GL_TEXTURE_2D, 0, format, textureWidth, textureHeight, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	else {
		std::cout << "Unable to load roof" << std::endl;
	}
	stbi_image_free(data);

	glBindVertexArray(0);

	std::vector<float> indicatorVertices = drawUnitCircle(0.03f);

	Shader indicatorShader("circlVertexShader.glsl", "circleFragmentShader.glsl");
	unsigned int indicatorVAO;
	glGenVertexArrays(1, &indicatorVAO);
	glBindVertexArray(indicatorVAO);
	sendDataToCard(indicatorVAO, indicatorVertices, 3);
	glBindVertexArray(0);

	Shader sourceShader("sourceVertexShader.glsl", "sourceFragmentShader.glsl");
	unsigned int sourceVAO;
	glGenVertexArrays(1, &sourceVAO);
	std::vector<float> sourceVertices;
	std::vector<unsigned int> sourceIndices;
	generateSphere(sourceVertices, sourceIndices, 0.5);
	sendDataToCard(sourceVAO, sourceVertices, sourceIndices, 8);

	arrowPos = initialArrowPos;

	glm::vec3 wallPositions[] = {
		glm::vec3(wallOffset, 0.0f, 0.0f), // Right Wall
		glm::vec3(-wallOffset, 0.0f, 0.0f), // Left Wall
		glm::vec3(0.0f, 0.0f, wallOffset), // Back Wall
		glm::vec3(0.0f, 0.0f, -wallOffset) // Front Wall
	};

	glm::vec3 wallRotations[] = {
		glm::vec3(0.0f, 0.0f, 1.0f), // Right Wall
		glm::vec3(0.0f, 0.0f, -1.0f), // Left Wall
		glm::vec3(-1.0f, 0.0f, 0.0f), // Back Wall
		glm::vec3(1.0f, 0.0f, 0.0f), // Front Wall
	};

	Arrow arrow(2.0f, 0.02f, 0.5f, 0.02f);

	std::vector<Collidable> collidables;

	setCollidables(collidables);

	Shader targetShader("cubeVertexShader.glsl", "cubeFragmentShader.glsl");

	std::vector<float> vertices = {
		// positions          // normals           // texture coords
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
	};
	unsigned int targetVAO;
	glGenVertexArrays(1, &targetVAO);
	sendDataToCard(targetVAO, vertices, 8);

	targetShader.use();
	targetShader.setFloat("material.shininess", 32.0f);
	targetShader.setInt("material.specular", 1);
	targetShader.setInt("material.diffuse", 0);

	targetShader.setVec3("light.diffuse", diffuseColor);
	targetShader.setVec3("light.ambient", ambientColor);
	targetShader.setVec3("light.specular", specularColor);

	unsigned int diffuseMap;
	glGenTextures(1, &diffuseMap);
	glBindTexture(GL_TEXTURE_2D, diffuseMap);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	stbi_set_flip_vertically_on_load(true);
	data = stbi_load("./resources/container2.png", &textureWidth, &textureHeight, &channel, 0);
	if (data) {
		checkImageFormat(channel);
		glTexImage2D(GL_TEXTURE_2D, 0, format, textureWidth, textureHeight, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else {
		std::cout << "Faild to read texture..." << std::endl;
	}
	stbi_image_free(data);

	unsigned int specularMap;
	glGenTextures(1, &specularMap);
	glBindTexture(GL_TEXTURE_2D, specularMap);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	data = stbi_load("./resources/container2_specular.png", &textureWidth, &textureHeight, &channel, 0);
	if (data) {
		checkImageFormat(channel);
		glTexImage2D(GL_TEXTURE_2D, 0, format, textureWidth, textureHeight, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	stbi_image_free(data);

	while (!glfwWindowShouldClose(window)) {
		glClearColor(0.30f, 0.70f, 0.60f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glfwGetFramebufferSize(window, &width, &height);

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window);

		// For light Source Moving in elliptical path
		lightPos = lightOffset + glm::vec3(7 * sin(currentFrame), 5 * cos(currentFrame), 9 * cos(currentFrame));

		// remember arrow previous position before physics update (used for collision interpolation)
		prevArrowPos = arrowPos;

		// For target
		glm::vec3 previousTargetPos = targetPos;

		// Simple Pendulum Kind of Motion
		if (!arrowStuckInTarget && level1) {
			targetPos = targetOffset + glm::vec3(r1 * sin(currentFrame), 0.0f, 0.0f);
		}

		// Circular Motion
		else if (!arrowStuckInTarget && level2) {
			targetPos = targetOffset + glm::vec3(r1 * sin(currentFrame), r1 * cos(currentFrame), 0.0f);
		}

		// Infinity Shape
		else if (!arrowStuckInTarget && level3) {
			targetPos = targetOffset + glm::vec3(r1 * cos(2 * currentFrame + glm::radians(90.0f)), r1 * sin(currentFrame), 0.0f);
		}

		// Random Variation Motion
		else if (!arrowStuckInTarget && level4) {
			targetPos = targetOffset + glm::vec3(r1 * cos(1.0f * currentFrame + phi1),
				r2 * sin(1.6180339f * currentFrame + phi2),
				r3 * cos(1.4142135f * currentFrame + phi3));
		}

		// Definign projection and view matrices which will be constant for all object we render
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), ((float)width / height), 0.1f, 200.0f);
		glm::mat4 view = camera.GetViewMatrix();

		// Rendering Source
		sourceShader.use();
		glm::mat4 sourceModel = glm::mat4(1.0f);
		sourceModel = glm::translate(sourceModel, lightPos);
		sourceShader.setMat4("model", sourceModel);
		sourceShader.setMat4("view", view);
		sourceShader.setMat4("projection", projection);
		glBindVertexArray(sourceVAO);
		glDrawElements(GL_TRIANGLES, sourceIndices.size(), GL_UNSIGNED_INT, 0);

		// Now its turn for target
		targetShader.use();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, diffuseMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, specularMap);

		targetShader.setVec3("light.position", lightPos);
		targetShader.setVec3("viewPos", camera.Position);

		glm::mat4 targetModel = glm::mat4(1.0f);
		targetModel = glm::translate(targetModel, targetPos);
		targetShader.setMat4("model", targetModel);
		targetShader.setMat4("view", view);
		targetShader.setMat4("projection", projection);

		glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(targetModel)));
		targetShader.setMat3("normalMatrix", normalMatrix);

		glBindVertexArray(targetVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);


		groundShader.use();
		groundShader.setVec3("light.position", lightPos);


		// Ground
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, groundTexture);
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		

		groundShader.setMat4("view", view);
		groundShader.setMat4("projection", projection);
		groundShader.setMat4("model", model);
		normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));
		groundShader.setMat3("normalMatrix", normalMatrix);

		glBindVertexArray(groundVAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);



		// Roof
		glBindTexture(GL_TEXTURE_2D, textureRoof);
		glm::mat4 roofModel = glm::mat4(1.0f);
		roofModel = glm::translate(roofModel, glm::vec3(0.0f, 20.0f, 0.0f));
		roofModel = glm::rotate(roofModel, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));

		groundShader.setMat4("view", view);
		groundShader.setMat4("projection", projection);
		groundShader.setMat4("model", roofModel);
		normalMatrix = glm::mat3(glm::transpose(glm::inverse(roofModel)));
		groundShader.setMat3("normalMatrix", normalMatrix);


		glBindVertexArray(groundVAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// Walls
    for (int i = 0; i < 4; i++) {
		// Always use the wall texture for the room walls. The dartboard is drawn
		// separately as a textured quad later so the wall texture must remain unchanged.
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureWall);
			glm::mat4 wallModel = glm::mat4(1.0f);
			wallModel = glm::translate(wallModel, wallPositions[i]);
			wallModel = glm::rotate(wallModel, glm::radians(90.0f), wallRotations[i]);


			groundShader.setMat4("view",view);
			groundShader.setMat4("projection", projection);
			groundShader.setMat4("model", wallModel);
			normalMatrix = glm::mat3(glm::transpose(glm::inverse(wallModel)));
			groundShader.setMat3("normalMatrix", normalMatrix);

			glBindVertexArray(groundVAO);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}

	indicatorShader.use();
	indicatorShader.setFloat("someColor", speedFactor);
	glm::vec3 indicatorOffset =
		camera.Front * 0.5f +
		camera.Right * -0.2f +
		camera.Up * -0.1f;

	glm::vec3 indicatorPos = camera.Position + indicatorOffset;
	glm::vec3 dir = glm::normalize(camera.Front);
	glm::quat indicatorRotation = glm::rotation(glm::vec3(0.0f, 0.0f, 1.0f), dir);

	glm::mat4 modelIndicator = glm::mat4(1.0f);
	modelIndicator = glm::translate(modelIndicator, indicatorPos);
	modelIndicator *= glm::toMat4(indicatorRotation);


	indicatorShader.setMat4("view",view);
	indicatorShader.setMat4("projection", projection);
	indicatorShader.setMat4("model", modelIndicator);

	glBindVertexArray(indicatorVAO);
	glDrawArrays(GL_TRIANGLE_FAN, 0, indicatorVertices.size() / 3);

	//static glm::quat lastRot = glm::identity<glm::quat>();
	static glm::quat lastRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

	// if the arrow isn't released yet calculate its position according to the camera's postion and orientation
	if (!releaseArrow && !arrowStuck)
		{
			glm::vec3 offset =
				camera.Front * 0.5f +
				camera.Right * 0.2f +
				camera.Up * -0.1f;

			arrowPos = camera.Position + offset;
			glm::vec3 dir = glm::normalize(camera.Front);

			lastRot = glm::rotation(glm::vec3(0.0f, 0.0f, 1.0f), dir);
		}

	// if the arrow is set to release and it is not released yet i.e velocity = 0 , assign it a velocity
	if (releaseArrow && (velocity == glm::vec3(0.0f))) {
			glm::vec3 dir = glm::normalize(camera.Front);
			float pitch = asin(dir.y);
			projectionAngle = glm::degrees(pitch);

			// starting a new throw: reset score state
			lastThrowScore = 0;
			throwing = true;
			// record initial previous position for interpolation
			prevArrowPos = arrowPos;

			velocity = camera.Front * initialSpeed * speedFactor;
		}
	
	// if the arrow is released calculate its position based on physics
	if (releaseArrow && !arrowReachGround){
		// Update arrow
		arrowPos += velocity * deltaTime * timeScale;
		velocity.y -= g * deltaTime * timeScale;
		glm::vec3 arrowSize = glm::vec3(0.2f, 0.2f, 0.6f);
			
		glm::vec3 hit;
		bool collided = false;

		glm::vec3 targetVelocity = (targetPos - previousTargetPos) / deltaTime;
		glm::vec3 relativeVelocity = velocity - targetVelocity;

		glm::vec3 halfSize = glm::vec3(0.5f);

		glm::vec3 minB = targetPos - halfSize;
		glm::vec3 maxB = targetPos + halfSize;

		float tHit;
		glm::vec3 hitNormal;

		bool hitTarget = sweptPointAABB(prevArrowPos,	relativeVelocity * deltaTime, minB,	maxB, tHit, hitNormal);

		if (hitTarget) {
			arrowPos = prevArrowPos + (relativeVelocity * deltaTime) * tHit;

			velocity = glm::vec3(0.0f);
			arrowStuck = true;
			arrowStuckInTarget = true;
			releaseArrow = false;

			std::cout << "Hit Cube!" << std::endl;

			if (level1) {
				level1 = false;
				level2 = true;
			}

			else if (level2) {
				level2 = false;
				level3 = true;
			}

			else if (level3) {
				level3 = false;
				level4 = true;
			}

		}

		for (auto& collidable : collidables) {
			if (intersectSegmentPlane(prevArrowPos, arrowPos, collidable.normal, collidable.d, hit)) {
				arrowPos = hit;
				velocity = glm::vec3(0.0f);
				arrowStuck = true;
				releaseArrow = false;
				if (collidable.normal == glm::vec3(0, 1, 0)) {
					arrowReachGround = true;
				}

				std::cout << "Arrow collided with " << collidable.name << std::endl;

				collided = true;
				break;
			}
		}

		}

	// if arrow has some velocity then let's make its orientation towards the velocity
	if (glm::length(velocity) > 0.0001f) {
			// Direction of motion
			glm::vec3 dir = glm::normalize(velocity);

			// Compute quaternion rotation from default arrow orientation (along +Z)
			glm::quat targetRot = glm::rotation(glm::vec3(0.0f, 0.0f, 1.0f), dir);

			// Smooth rotation using slerp (adjust 0.2f for speed)
			lastRot = glm::slerp(lastRot, targetRot, 1.0f);
		}

	// let's reposition arrow if it reached ground
	if (arrowReachGround) {
			releaseArrow = false;
			arrowReachGround = false;
		}

	glm::mat4 modelArrow = glm::mat4(1.0f);
	modelArrow = glm::translate(modelArrow, arrowPos);
	modelArrow *= glm::toMat4(lastRot);		

	arrow.draw(modelArrow, view, projection);

	


	// update window title with last throw score (resets on each throw)
	{
			std::string title = "Jamie King - Last Score: " + std::to_string(lastThrowScore);
			glfwSetWindowTitle(window, title.c_str());
		}

	glfwPollEvents();
	glfwSwapBuffers(window);
	}

	glDeleteProgram(groundShader.ID);
	glDeleteProgram(indicatorShader.ID);
	glDeleteProgram(sourceShader.ID);
	glDeleteProgram(targetShader.ID);

	glDeleteVertexArrays(1, &groundVAO);
	glDeleteVertexArrays(1, &indicatorVAO);
	glDeleteVertexArrays(1, &sourceVAO);
	glDeleteVertexArrays(1, &targetVAO);

	glDeleteTextures(1, &groundTexture);
	glDeleteTextures(1, &textureWall);
	glDeleteTextures(1, &textureRoof);
	glDeleteTextures(1, &diffuseMap);
	glDeleteTextures(1, &specularMap);

	glfwTerminate();
	return 0;
}

void frame_buffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);

	return;
}


void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}

	static bool previousF1 = false;
	bool currentF1 = glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS;
	if (currentF1 && !previousF1) {
		mouseLock = not mouseLock;
		glfwSetInputMode(window, GLFW_CURSOR,
			mouseLock ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
	}
	previousF1 = currentF1;

	static bool previousF10 = false;
	bool currentF10 = glfwGetKey(window, GLFW_KEY_F10) == GLFW_PRESS;
	if (currentF10 && !previousF10) {
		firstPersonView = not firstPersonView;
	}
	previousF10 = currentF10;


	float speed = 3.0f * deltaTime;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);

	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);

	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);

	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
		camera.ProcessKeyboard(UP, deltaTime);

	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
		camera.ProcessKeyboard(DOWN, deltaTime);

	if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
		if (!arrowStuck)
			releaseArrow = true;

	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
		arrowStuck = false;
		arrowStuckInTarget = false;
	}

	static bool previousUp = false;
	bool currentUp = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
	if (currentUp && !previousUp) {
		speedFactor += 0.05f;
	}
	previousUp = currentUp;

	static bool previousDown = false;
	bool currentDown = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
	if (currentDown && !previousDown) {
		speedFactor -= 0.05f;
		if (speedFactor <= 0) {
			speedFactor = 0.0f;
		}
	}
	previousDown = currentDown;

	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

}

void mouseButtonHandler(GLFWwindow* window, int button, int action, int modes) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		if (!arrowStuck)
			releaseArrow = true;
	}

	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		arrowStuck = false;
		arrowStuckInTarget = false;
	}
}


void mouseMoveEvent(GLFWwindow* window, double posX, double posY) {
	if (mouseLock) {
		float xpos = static_cast<float> (posX);
		float ypos = static_cast<float> (posY);

		if (firstMouse) {
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}

		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos;
		lastX = xpos;
		lastY = ypos;

		camera.ProcessMouseMovement(xoffset, yoffset);
	}
	return;
}

void mouseScrollEvent(GLFWwindow* window, double xOffset, double yOffset) {
	// Scrool up
	if (yOffset > 0) {
		speedFactor += 0.05f;
	}
	// Scroll down
	else if (yOffset < 0) {
		speedFactor -= 0.05f;
	}
}

std::vector<float> drawUnitCircle(float radius) {
	int sectorCount = 36;
	const float PI = 3.14159;
	float sectorStep = 2 * PI / sectorCount;
	float sectorAngle;

	std::vector<float> vertices;
	vertices.push_back(0.0f);
	vertices.push_back(0.0f);
	vertices.push_back(0.0f);

	for (int i = 0; i <= sectorCount; i++) {
		sectorAngle = sectorStep * i;

		vertices.push_back(radius * cos(sectorAngle)); // x
		vertices.push_back(radius * sin(sectorAngle)); // y
		vertices.push_back(radius * 0); // z
	}

	return vertices;
}

void generateSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices, float radius)
{
	// constants inside function
	const int stacks = 50;
	const int sectors = 50;
	const float PI = 3.14159265359f;

	float x, y, z, xy;
	float nx, ny, nz;
	float u, v;

	float sectorStep = 2 * PI / sectors;
	float stackStep = PI / stacks;

	for (int i = 0; i <= stacks; ++i)
	{
		float stackAngle = PI / 2 - i * stackStep; // from +pi/2 to -pi/2
		xy = radius * cos(stackAngle);
		y = radius * sin(stackAngle);

		for (int j = 0; j <= sectors; ++j)
		{
			float sectorAngle = j * sectorStep;

			x = xy * cos(sectorAngle);
			z = xy * sin(sectorAngle);

			// position
			vertices.push_back(x);
			vertices.push_back(y);
			vertices.push_back(z);

			// normal
			nx = x / radius;
			ny = y / radius;
			nz = z / radius;
			vertices.push_back(nx);
			vertices.push_back(ny);
			vertices.push_back(nz);

			// texture coords
			u = (float)j / sectors;
			v = (float)i / stacks;
			vertices.push_back(u);
			vertices.push_back(v);
		}
	}

	// indices
	int k1, k2;
	for (int i = 0; i < stacks; ++i)
	{
		k1 = i * (sectors + 1);
		k2 = k1 + sectors + 1;

		for (int j = 0; j < sectors; ++j, ++k1, ++k2)
		{
			if (i != 0)
			{
				indices.push_back(k1);
				indices.push_back(k2);
				indices.push_back(k1 + 1);
			}

			if (i != (stacks - 1))
			{
				indices.push_back(k1 + 1);
				indices.push_back(k2);
				indices.push_back(k2 + 1);
			}
		}
	}
}

void checkImageFormat(const int& channel) {
	if (channel == 1) {
		format = GL_RED;
	}

	else if (channel == 3){
		format = GL_RGB;
	}

	else if (channel == 4) {
		format = GL_RGBA;
	}
}

void sendDataToCard(unsigned int& VAO, const std::vector<float>& vertices, int stride) {
	glBindVertexArray(VAO);
	unsigned int VBO, EBO;

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

	// Position Attirb
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)0);

	// Normal Attrib
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(3 * sizeof(float)));

	// Texture Attrib
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(6 * sizeof(float)));

	glBindVertexArray(0);
}

void sendDataToCard(unsigned int& VAO, const std::vector<float>& vertices, const std::vector<unsigned int>& indices, int stride) {
	glBindVertexArray(VAO);
	unsigned int VBO, EBO;

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

	// Position Attirb
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)0);

	// Normal Attrib
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(3 * sizeof(float)));

	// Texture Attrib
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(6 * sizeof(float)));

	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
	
	glBindVertexArray(0);
}

static int computeDartboardScore(float distanceFromCenter, float radius) {
	if (distanceFromCenter > radius) return 0;

	float r = radius;
	// simple ring scoring (adjust thresholds as you like)
	if (distanceFromCenter <= 0.1f * r) return 50; // inner bull
	if (distanceFromCenter <= 0.25f * r) return 25; // outer bull
	if (distanceFromCenter <= 0.5f * r) return 10; // inner ring
	if (distanceFromCenter <= r) return 5; // outer area
	return 0;
}

bool intersectSegmentPlane(glm::vec3 start, glm::vec3 end ,glm::vec3 normal, float d, glm::vec3& hitPoint) {
	glm::vec3 dir = end - start;
	float denom = glm::dot(normal, dir);

	if (fabs(denom) < 1e-6f)
		return false;

	float t = -(glm::dot(normal, start) + d) / denom;

	if (t >= 0.0f && t <= 1.0f) {
		hitPoint = start + t * dir;
		return true;
	}

	return false;
}

void setCollidables(std::vector<Collidable>& collidables) {
	Collidable ground;
	ground.normal = glm::vec3(0.0f, 1.0f, 0.0f);
	ground.d = 0.85f;
	ground.name = "ground";
	collidables.push_back(ground);

	Collidable roof;
	roof.normal = glm::vec3(0.0f, 1.0f, 0.0f);
	roof.d = -20.0f;
	roof.name = "roof";
	collidables.push_back(roof);

	Collidable rightWall;
	rightWall.normal = glm::vec3(1.0f, 0.0f, 0.0f);
	rightWall.d = -wallOffset;
	rightWall.name = "rightWall";
	collidables.push_back(rightWall);

	Collidable leftWall;
	leftWall.normal = glm::vec3(-1.0f, 0.0f, 0.0f);
	leftWall.d = -wallOffset;
	leftWall.name = "leftWall";
	collidables.push_back(leftWall);

	Collidable frontWall;
	frontWall.normal = glm::vec3(0.0f, 0.0f, -1.0f);
	frontWall.d = -wallOffset;
	frontWall.name = "frontWall";
	collidables.push_back(frontWall);

	Collidable backWall;
	backWall.normal = glm::vec3(0.0f, 0.0f, 1.0f);
	backWall.d = -wallOffset;
	backWall.name = "backWall";
	collidables.push_back(backWall);
}

bool sweptPointAABB(glm::vec3 start, glm::vec3 velocity, glm::vec3 minB, glm::vec3 maxB, float& tHit, glm::vec3& hitNormal) {
	float tEnter = 0.0f;
	float tExit = 1.0f;

	glm::vec3 normal(0.0f);

	for (int i = 0; i < 3; i++) {
		if (fabs(velocity[i]) < 1e-6f) {
			if (start[i] < minB[i] || start[i] > maxB[i])
				return false;
		}
		else {
			float t1 = (minB[i] - start[i]) / velocity[i];
			float t2 = (maxB[i] - start[i]) / velocity[i];

			float tMin = std::min(t1, t2);
			float tMax = std::max(t1, t2);

			if (tMin > tEnter) {
				tEnter = tMin;
				normal = glm::vec3(0.0f);
				normal[i] = (t1 > t2) ? 1.0f : -1.0f;
			}

			tExit = std::min(tExit, tMax);

			if (tEnter > tExit)
				return false;
		}
	}

	tHit = tEnter;
	hitNormal = normal;

	return (tEnter >= 0.0f && tEnter <= 1.0f);
}
