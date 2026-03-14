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


void frame_buffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouseMoveEvent(GLFWwindow* window, double posX, double posY);
std::vector<float> drawUnitCircle();
void generateCylinder(std::vector<float>& vertices, std::vector<unsigned int>& indices, int height, float radius);
void sendDataToCard(unsigned int& VAO, const std::vector<float>& vertices);
void sendDataToCard(unsigned int& VAO, const std::vector<float>& vertices, const std::vector<unsigned int>& indices);

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

// Arrow Settings
glm::vec3 initialArrowPos = glm::vec3(1.0f);
glm::vec3 arrowPos;
float projectionAngle = 30.0f;
float initialSpeed = 25.0f;
float lastArrowX;
bool releaseArrow = false;
float speedScale = 0.5f;
float timeScale = 0.5f;
bool arrowReachGround = false;
glm::vec3 velocity = glm::vec3(0.0f);
bool firstTime;

// Motion and Gravity
float g = 9.8;


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

	Shader ourShader("vertexShader.glsl", "fragmentShader.glsl");

	float Ground[] = {
		// Position					// Color					// Texture
		-21.0f, -1.0f, 21.0f,		 0.0f, 1.0f, 0.0f,				0.0f, 21.0f,		// left top
																					
		21.0f, -1.0f, -21.0f,		 0.0f, 1.0f, 0.0f,				21.0f, 0.0f,		// right bottom
																					
		-21.0f, -1.0f, -21.0f,		 0.0f, 1.0f, 0.0f,				0.0f, 0.0f,		// left bottom
																					
		21.0f, -1.0f, 21.0f, 		 0.0f, 1.0f, 0.0f,				21.0f, 21.0f,		// right top

	};

	GLushort corners[] = {
		0, 2, 1,
		0, 1, 3,
	};

	GLuint VBO, VAO, EBO;

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// Vertex Data Sent through vertex buffer
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Ground), Ground, GL_STATIC_DRAW);

	// Vertex Attributes
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), NULL);

	// Color Attributes
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 3));

	// Sending Indices data through the element buffer
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(corners), corners, GL_STATIC_DRAW);

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
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

	stbi_image_free(data);




	glBindVertexArray(0);
	std::vector<float> circleVertices = drawUnitCircle();

	Shader circleShader("circlVertexShader.glsl", "circleFragmentShader.glsl");
	unsigned int circleVAO;
	glGenVertexArrays(1, &circleVAO);
	glBindVertexArray(circleVAO);
	sendDataToCard(circleVAO, circleVertices);
	glBindVertexArray(0);

	std::vector<float> cylinderVertices;
	std::vector<unsigned int> cylinderIndices;
	generateCylinder(cylinderVertices, cylinderIndices, 2, 0.05f);

	Shader cylinderShader("cylinderVertexShader.glsl", "cylinderFragmentShader.glsl");
	unsigned int cylinderVAO;
	glGenVertexArrays(1, &cylinderVAO);
	glBindVertexArray(cylinderVAO);
	sendDataToCard(cylinderVAO, cylinderVertices, cylinderIndices);
	glBindVertexArray(0);

	arrowPos = initialArrowPos;

	while (!glfwWindowShouldClose(window)) {
		glClearColor(0.30f, 0.70f, 0.60f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glfwGetFramebufferSize(window, &width, &height);

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window);

		ourShader.use();
		glBindTexture(GL_TEXTURE_2D, texture);
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, -3.0f));
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), ((float)width / height), 0.1f, 200.0f);
		glm::mat4 view = camera.GetViewMatrix();

		ourShader.setMat4("view", view);
		ourShader.setMat4("projection", projection);
		ourShader.setMat4("model", model);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

		circleShader.use();
		glm::mat4 modelCircle = glm::mat4(1.0f);
		modelCircle = glm::translate(modelCircle, glm::vec3(0.0f, 1.0f, -3.0f));
		glm::mat4 projectionCircle = glm::perspective(glm::radians(45.0f), ((float)width / height), 0.1f, 200.0f);
		glm::mat4 viewCircle = camera.GetViewMatrix();

		circleShader.setMat4("view", viewCircle);
		circleShader.setMat4("projection", projectionCircle);
		circleShader.setMat4("model", modelCircle);

		glBindVertexArray(circleVAO);
		glDrawArrays(GL_TRIANGLE_FAN, 0, circleVertices.size() / 3);

		cylinderShader.use();

		if (releaseArrow && (velocity == glm::vec3(0.0f))) {
			velocity = glm::vec3(
				initialSpeed * cos(glm::radians(projectionAngle)) * speedScale,
				initialSpeed * sin(glm::radians(projectionAngle)) * speedScale,
				0.0f
				);
		}
		
		if (releaseArrow && !arrowReachGround){
			// Update arrow

			arrowPos += velocity * deltaTime * timeScale;
			velocity.y -= g * deltaTime * timeScale;

			if (arrowPos.y < -0.85f) {
				arrowPos.y = -0.85f;
				velocity = glm::vec3(0.0f);
				arrowReachGround = true;
				releaseArrow = false;
			}


		}

// Identity quaternion by default
		static glm::quat lastRot = glm::identity<glm::quat>();

		if (glm::length(velocity) > 0.0001f) {
			// Direction of motion
			glm::vec3 dir = glm::normalize(velocity);

			// Compute quaternion rotation from default arrow orientation (along +Y)
			glm::quat targetRot = glm::rotation(glm::vec3(0.0f, 1.0f, 0.0f), dir);

			// Smooth rotation using slerp (adjust 0.2f for speed)
			lastRot = glm::slerp(lastRot, targetRot, 0.2f);
		}

		glm::mat4 modelCylinder = glm::mat4(1.0f);
		modelCylinder = glm::translate(modelCylinder, arrowPos);
		modelCylinder = glm::rotate(modelCylinder, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelCylinder *= glm::toMat4(lastRot);
		cylinderShader.setMat4("model", modelCylinder);
		
		glm::mat4 projectionCylinder = glm::perspective(glm::radians(45.0f), ((float)width / height), 0.1f, 200.0f);
		glm::mat4 viewCylinder = camera.GetViewMatrix();

		cylinderShader.setMat4("view", viewCylinder);
		cylinderShader.setMat4("projection", projectionCylinder);
		cylinderShader.setMat4("model", modelCylinder);

		glBindVertexArray(cylinderVAO);
		glDrawElements(GL_TRIANGLES, cylinderIndices.size(), GL_UNSIGNED_INT, 0);


		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	glDeleteProgram(ourShader.ID);
	glDeleteVertexArrays(1, &VAO);
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
		releaseArrow = true;

	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
		arrowPos = initialArrowPos;
		releaseArrow = false;
		velocity = glm::vec3(0.0f);
		arrowReachGround = false;
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

std::vector<float> drawUnitCircle() {
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

		vertices.push_back(cos(sectorAngle)); // x
		vertices.push_back(sin(sectorAngle)); // y
		vertices.push_back(0); // z
	}

	return vertices;
}

void generateCylinder(std::vector<float>& vertices, std::vector<unsigned int>& indices, int height, float radius) {
	vertices.clear();
	indices.clear();

	int sectorCount = 36;
	const float PI = 3.14159;
	float sectorStep = 2 * PI / sectorCount;
	float sectorAngle;
	float h = height / 2.0f;

	// For Curve Sides
	for (int i = 0; i <= sectorCount; i++) {
		sectorAngle = sectorStep * i;
		float x = radius * cos(sectorAngle);
		float y = radius * sin(sectorAngle);

		vertices.push_back(x); // x
		vertices.push_back(y); // y
		vertices.push_back(-h); // z

		vertices.push_back(x); // x
		vertices.push_back(y); // y
		vertices.push_back(h); // z
	}

	for (int i = 0; i < sectorCount; i++) {
		unsigned int b1 = i * 2;
		unsigned int t1 = i * 2 + 1;
		unsigned int b2 = (i + 1) * 2;
		unsigned int t2 = (i + 1) * 2 + 1;

		indices.push_back(b1);
		indices.push_back(t1);
		indices.push_back(b2);

		indices.push_back(t1);
		indices.push_back(t2);
		indices.push_back(b2);
	}

	// For Caps
	int centerBottomIdx = vertices.size() / 3;
	int centerTopIdx = centerBottomIdx + 1;

	// bottom center
	vertices.push_back(0.0f);
	vertices.push_back(0.0f);
	vertices.push_back(-h);

	// top center
	vertices.push_back(0.0f);
	vertices.push_back(0.0f);
	vertices.push_back(h);

	for (int i = 0; i <= sectorCount; i++) {
		float sectorAngle = i * sectorStep;
		float x = radius * cos(sectorAngle);
		float y = radius * sin(sectorAngle);

		vertices.push_back(x);
		vertices.push_back(y);
		vertices.push_back(-h);

		vertices.push_back(x);
		vertices.push_back(y);
		vertices.push_back(h);
	}

	int ringStart = centerBottomIdx + 2;
	for (int i = 0; i < sectorCount; i++) {
		int currentRingIdx = ringStart + (i * 2);
		int nextRingIdx = ringStart + ((i + 1) * 2);

		indices.push_back(centerBottomIdx);
		indices.push_back(nextRingIdx);
		indices.push_back(currentRingIdx);

		indices.push_back(centerTopIdx);
		indices.push_back(currentRingIdx + 1);
		indices.push_back(nextRingIdx + 1);
	}

}

void sendDataToCard(unsigned int& VAO, const std::vector<float>& vertices) {
	glBindVertexArray(VAO);
	unsigned int VBO, EBO;

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}

void sendDataToCard(unsigned int& VAO, const std::vector<float>& vertices, const std::vector<unsigned int>& indices) {
	glBindVertexArray(VAO);
	unsigned int VBO, EBO;

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
}