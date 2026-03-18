#include <iostream>
#include <fstream>
#include <vector>

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Camera.h"
#include "Shader.h"
#include "Arrow.h"


void frame_buffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouseMoveEvent(GLFWwindow* window, double posX, double posY);
std::vector<float> drawUnitCircle(float radius);
void generateSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices, float radius);
void sendDataToCard(unsigned int& VAO, const std::vector<float>& vertices, int stride);
void sendDataToCard(unsigned int& VAO, const std::vector<float>& vertices, const std::vector<unsigned int>& indices, int stride);


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
float groundSize = 21.0f;

// wall attributes
float wallHeight = 14.0f;
float wallOffset = groundSize - 2.5f;
float wallSize = 21.0f;

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

// Motion and Gravity
float g = 9.8;

// Light Attributes
glm::vec3 lightPos = glm::vec3(-15.0f, 17.0f, -15.0f);


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
	glEnable(GL_DEPTH_TEST);

	Shader groundShader("vertexShader.glsl", "fragmentShader.glsl");

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
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	// Let's define wrapping and filtering parameters for texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


	unsigned char* data = stbi_load("./resources/grass.jpg", &textureWidth, &textureHeight, &channel, 0);
	if (data) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureWidth, textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
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


	data = stbi_load("./resources/some.jpg", &textureWidth, &textureHeight, &channel, 0);
	if (data) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureWidth, textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
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


	data = stbi_load("./resources/roof.jpg", &textureWidth, &textureHeight, &channel, 0);
	if (data) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureWidth, textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else {
		std::cout << "Failed to read texture file..." << std::endl;
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
	generateSphere(sourceVertices, sourceIndices, 1);
	sendDataToCard(sourceVAO, sourceVertices, sourceIndices, 8);

	arrowPos = initialArrowPos;

	glm::vec3 wallPositions[] = {
		glm::vec3(wallOffset, 0.0f, 0.0f),
		glm::vec3(-wallOffset, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, wallOffset),
		glm::vec3(0.0f, 0.0f, -wallOffset)
	};

	glm::vec3 wallRotations[] = {
		glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(1.0f, 0.0f, 0.0f),
	};

	Arrow arrow(2.0f, 0.05f, 0.5f, 0.05f);

	while (!glfwWindowShouldClose(window)) {
		glClearColor(0.30f, 0.70f, 0.60f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glfwGetFramebufferSize(window, &width, &height);

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window);

		glm::mat4 projection = glm::perspective(glm::radians(45.0f), ((float)width / height), 0.1f, 200.0f);
		glm::mat4 view = camera.GetViewMatrix();

		sourceShader.use();
		glm::mat4 sourceModel = glm::mat4(1.0f);
		sourceModel = glm::translate(sourceModel, lightPos);
		sourceShader.setMat4("model", sourceModel);
		sourceShader.setMat4("view", view);
		sourceShader.setMat4("projection", projection);
		glBindVertexArray(sourceVAO);
		glDrawElements(GL_TRIANGLES, sourceIndices.size(), GL_UNSIGNED_INT, 0);


		groundShader.use();
		// Ground
		glBindTexture(GL_TEXTURE_2D, texture);
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		

		groundShader.setMat4("view", view);
		groundShader.setMat4("projection", projection);
		groundShader.setMat4("model", model);

		glBindVertexArray(groundVAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// Roof
		glBindTexture(GL_TEXTURE_2D, textureRoof);
		glm::mat4 roofModel = glm::mat4(1.0f);
		roofModel = glm::translate(roofModel, glm::vec3(0.0f, 22.0f, 0.0f));

		groundShader.setMat4("view", view);
		groundShader.setMat4("projection", projection);
		groundShader.setMat4("model", roofModel);

		glBindVertexArray(groundVAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// Walls
		for (int i = 0; i < 4; i++) {
			glBindTexture(GL_TEXTURE_2D, textureWall);
			glm::mat4 wallModel = glm::mat4(1.0f);
			wallModel = glm::translate(wallModel, wallPositions[i]);
			wallModel = glm::rotate(wallModel, glm::radians(90.0f), wallRotations[i]);


			groundShader.setMat4("view",view);
			groundShader.setMat4("projection", projection);
			groundShader.setMat4("model", wallModel);

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

			velocity = camera.Front * initialSpeed * speedFactor;
		}
		
		// if the arrow is released calculate its position based on physics
		if (releaseArrow && !arrowReachGround){
			// Update arrow

			arrowPos += velocity * deltaTime * timeScale;
			velocity.y -= g * deltaTime * timeScale;

			if (arrowPos.y < -0.85f) {
				arrowPos.y = -0.85f;
				velocity = glm::vec3(0.0f);
				arrowReachGround = true;
				releaseArrow = false;
				arrowStuck = true;
			}

			if (abs(arrowPos.z) > wallOffset) {
				velocity = glm::vec3(0.0f);
				arrowReachWall = true;
				releaseArrow = false;
				arrowStuck = true;
			}

			if (abs(arrowPos.x) >= wallOffset) {
				velocity = glm::vec3(0.0f);
				arrowReachWall = true;
				releaseArrow = false;
				arrowStuck = true;
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

		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	glDeleteProgram(groundShader.ID);
	glDeleteVertexArrays(1, &groundVAO);
	glDeleteProgram(indicatorShader.ID);
	glDeleteVertexArrays(1, &indicatorVAO);
	glDeleteProgram(sourceShader.ID);
	glDeleteVertexArrays(1, &sourceVAO);

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

	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
		arrowStuck = false;

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