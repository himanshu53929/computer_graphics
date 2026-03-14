#pragma once
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"
#include "Camera.h"

class Arrow {
private:
    std::vector<float>        combinedVertices;
    std::vector<unsigned int> combinedIndices;

    unsigned int VAO = 0, VBO = 0, EBO = 0;

    void generateCylinder(std::vector<float>& verts, std::vector<unsigned int>& idx, float height, float radius, float zOffset = 0.0f)
    {
        verts.clear(); idx.clear();

        const int   sectorCount = 36;
        const float PI = 3.14159265359f;
        float       sectorStep = 2 * PI / sectorCount;
        float       h = height / 2.0f;

        // Side surface
        for (int i = 0; i <= sectorCount; i++) {
            float angle = i * sectorStep;
            float x = cos(angle), y = sin(angle);
            float u = (float)i / sectorCount;

            // Bottom vertex
            verts.push_back(x * radius); verts.push_back(y * radius); verts.push_back(-h + zOffset);
            verts.push_back(x);          verts.push_back(y);          verts.push_back(0.0f);
            verts.push_back(u);          verts.push_back(0.0f);

            // Top vertex
            verts.push_back(x * radius); verts.push_back(y * radius); verts.push_back(h + zOffset);
            verts.push_back(x);          verts.push_back(y);          verts.push_back(0.0f);
            verts.push_back(u);          verts.push_back(1.0f);
        }

        for (int i = 0; i < sectorCount; i++) {
            unsigned int b1 = i * 2, t1 = i * 2 + 1, b2 = (i + 1) * 2, t2 = (i + 1) * 2 + 1;
            idx.push_back(b1); idx.push_back(b2); idx.push_back(t1);
            idx.push_back(t1); idx.push_back(b2); idx.push_back(t2);
        }

        // Caps
        int baseIndex = (int)verts.size() / 8;
        int centerBotIdx = baseIndex;
        int centerTopIdx = baseIndex + 1;
        int ringStart = baseIndex + 2;

        verts.push_back(0.0f); verts.push_back(0.0f); verts.push_back(-h + zOffset);
        verts.push_back(0.0f); verts.push_back(0.0f); verts.push_back(-1.0f);
        verts.push_back(0.5f); verts.push_back(0.5f);

        verts.push_back(0.0f); verts.push_back(0.0f); verts.push_back(h + zOffset);
        verts.push_back(0.0f); verts.push_back(0.0f); verts.push_back(1.0f);
        verts.push_back(0.5f); verts.push_back(0.5f);

        for (int i = 0; i <= sectorCount; i++) {
            float angle = i * sectorStep;
            float x = cos(angle), y = sin(angle);
            float u = x * 0.5f + 0.5f, v = y * 0.5f + 0.5f;

            verts.push_back(x * radius); verts.push_back(y * radius); verts.push_back(-h + zOffset);
            verts.push_back(0.0f);     verts.push_back(0.0f);     verts.push_back(-1.0f);
            verts.push_back(u);        verts.push_back(v);

            verts.push_back(x * radius); verts.push_back(y * radius); verts.push_back(h + zOffset);
            verts.push_back(0.0f);     verts.push_back(0.0f);     verts.push_back(1.0f);
            verts.push_back(u);        verts.push_back(v);
        }

        for (int i = 0; i < sectorCount; i++) {
            int cur = ringStart + i * 2;
            int next = ringStart + (i + 1) * 2;

            idx.push_back(centerBotIdx); idx.push_back(next);     idx.push_back(cur);
            idx.push_back(centerTopIdx); idx.push_back(cur + 1);  idx.push_back(next + 1);
        }
    }

    void generateCone(std::vector<float>& verts, std::vector<unsigned int>& idx, float height, float radius, float zOffset = 0.0f)
    {
        verts.clear(); idx.clear();

        const int   sectorCount = 36;
        const float PI = 3.14159265359f;
        float       sectorStep = 2 * PI / sectorCount;
        float       h = height / 2.0f;
        float       slantLen = sqrt(radius * radius + height * height);

        // Side surface — base ring + apex pairs
        for (int i = 0; i <= sectorCount; i++) {
            float angle = i * sectorStep;
            float x = cos(angle), y = sin(angle);
            float nx = x * (height / slantLen);
            float ny = y * (height / slantLen);
            float nz = radius / slantLen;
            float u = (float)i / sectorCount;

            // Base ring vertex
            verts.push_back(x * radius); verts.push_back(y * radius); verts.push_back(-h + zOffset);
            verts.push_back(nx);         verts.push_back(ny);         verts.push_back(nz);
            verts.push_back(u);          verts.push_back(0.0f);

            // Apex vertex
            verts.push_back(0.0f);       verts.push_back(0.0f);       verts.push_back(h + zOffset);
            verts.push_back(nx);         verts.push_back(ny);         verts.push_back(nz);
            verts.push_back(u);          verts.push_back(1.0f);
        }

        for (int i = 0; i < sectorCount; i++) {
            unsigned int b1 = i * 2, t1 = i * 2 + 1, b2 = (i + 1) * 2;
            idx.push_back(b1); idx.push_back(b2); idx.push_back(t1);
        }

        // Base cap
        int baseIndex = (int)verts.size() / 8;
        int centerIdx = baseIndex;
        int ringStart = baseIndex + 1;

        verts.push_back(0.0f); verts.push_back(0.0f); verts.push_back(-h + zOffset);
        verts.push_back(0.0f); verts.push_back(0.0f); verts.push_back(-1.0f);
        verts.push_back(0.5f); verts.push_back(0.5f);

        for (int i = 0; i <= sectorCount; i++) {
            float angle = i * sectorStep;
            float x = cos(angle), y = sin(angle);
            float u = x * 0.5f + 0.5f, v = y * 0.5f + 0.5f;

            verts.push_back(x * radius); verts.push_back(y * radius); verts.push_back(-h + zOffset);
            verts.push_back(0.0f);     verts.push_back(0.0f);     verts.push_back(-1.0f);
            verts.push_back(u);        verts.push_back(v);
        }

        for (int i = 0; i < sectorCount; i++) {
            idx.push_back(centerIdx);
            idx.push_back(ringStart + i + 1);
            idx.push_back(ringStart + i);
        }
    }

    void setupGPU() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER,
            combinedVertices.size() * sizeof(float),
            combinedVertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            combinedIndices.size() * sizeof(unsigned int),
            combinedIndices.data(), GL_STATIC_DRAW);

        const int stride = 8 * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

        glBindVertexArray(0);
    }

public:
    Arrow(float cylinderHeight, float cylinderRadius, float coneHeight, float coneRadius)
    {
        std::vector<float>        cylVerts, coneVerts;
        std::vector<unsigned int> cylIdx, coneIdx;

        float coneZOffset = cylinderHeight / 2.0f + coneHeight / 2.0f;

        generateCylinder(cylVerts, cylIdx, cylinderHeight, cylinderRadius, 0.0f);
        generateCone(coneVerts, coneIdx, coneHeight, coneRadius, coneZOffset);

        unsigned int vertexOffset = (unsigned int)(cylVerts.size() / 8);

        combinedVertices = cylVerts;
        combinedVertices.insert(combinedVertices.end(), coneVerts.begin(), coneVerts.end());

        combinedIndices = cylIdx;
        for (unsigned int i : coneIdx)
            combinedIndices.push_back(i + vertexOffset);

        setupGPU();
    }

    void draw(const glm::mat4& modelMatrix, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const {
        Shader shader("cylinderVertexShader.glsl", "cylinderFragmentShader.glsl");
        shader.use();

        shader.setMat4("view", viewMatrix);
        shader.setMat4("projection", projectionMatrix);
        shader.setMat4("model", modelMatrix);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)combinedIndices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    ~Arrow() {
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteVertexArrays(1, &VAO);
    }

    Arrow(const Arrow&) = delete;
    Arrow& operator=(const Arrow&) = delete;
};
