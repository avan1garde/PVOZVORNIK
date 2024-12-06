//Nikola Eric RA157/2021 

#define _CRT_SECURE_NO_WARNINGS
#define CRES 30
#define REMAINING_ROCKETS 10
#define STB_IMAGE_IMPLEMENTATION

// OpenGL and GLFW
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Math and transformations
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// File I/O and string operations
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

// Timer and random
#include <ctime>
#include <chrono>
#include <random>

// External libraries
#include "stb_image.h"
#include "Shaders.h"
#include "TextRendering.h"

// Math utilities
#include <cmath>
#include <math.h> // Some systems may prefer this

// Data structures
#include <map>

// Handling the standard library
#include <thread>

struct Location {
    float x;
    float y;
};

struct Rocket {
    float x, y;
    float dirX, dirY;
    bool isInbound;
    int heliTarget;
};

bool spacePressedCurrently = false;
bool spacePressedPreviously = false;
bool isMapHidden = false;

float pvoCenterX = 0.0f;
float pvoCenterY = 0.0f;
bool pvoCenterSet = false;
float pvoCenterRad = 0.04;
bool cityCenterSet = false;
float cityCenterX = 0.0f;
float cityCenterY = 0.0f;
float cityCenterRad = 0.04;
float helicopterRad = 0.03;
float rocketRad = 0.03;

Location heliPositions[5];
int remainingHelis = 5;
float rocketVelocity = 0.004f;
Rocket rockets[10];
auto startTime = std::chrono::high_resolution_clock::now();
bool initWait = false;
std::string heliNames[5] = { "Super 6-3","Super 6-4","AH-1 Cobra","AH-64 Apache","VH-92 Patriot" };
int hits = 0;
int selectedHeli = -1;
bool simulationOver = false;
int simulationCounter = 0;

void setCircle(float circle[64], float r, float xTranslation, float yTranslation)       //ChatGPT generated function
{
    float centerX = 0.0;
    float centerY = 0.0;
    circle[0] = centerX + xTranslation;
    circle[1] = centerY + yTranslation;

    for (int i = 0; i <= CRES; i++) {
        circle[2 + 2 * i] = centerX + xTranslation + r * cos((3.141592 / 180) * (i * 360 / CRES));
        circle[2 + 2 * i + 1] = centerY + yTranslation + r * sin((3.141592 / 180) * (i * 360 / CRES));
    }
}

void createHelis(int number) {
    srand(static_cast<unsigned>(time(nullptr)));

    for (int i = 0; i < number; ++i) {
        int side = rand() % 4;
        if (side == 0) {
            heliPositions[i].x = 1.1;
            std::string randomFloat = "0.";
            randomFloat.append(std::to_string(rand() % 10));
            randomFloat.append(std::to_string(rand() % 10));
            heliPositions[i].y = std::stof(randomFloat);
        }
        else if (side == 1) {
            std::string randomFloat = "0.";
            randomFloat.append(std::to_string(rand() % 10));
            randomFloat.append(std::to_string(rand() % 10));
            heliPositions[i].x = std::stof(randomFloat);
            heliPositions[i].y = 1.1;
        }
        else if (side == 2) {
            heliPositions[i].x = -1.1;
            std::string randomFloat = "0.";
            randomFloat.append(std::to_string(rand() % 10));
            randomFloat.append(std::to_string(rand() % 10));
            heliPositions[i].y = std::stof(randomFloat);
        }
        else {
            std::string randomFloat = "0.";
            randomFloat.append(std::to_string(rand() % 10));
            randomFloat.append(std::to_string(rand() % 10));
            heliPositions[i].x = std::stof(randomFloat);
            heliPositions[i].y = -1.1;
        }
    }
}

void moveHelis(float cityCenterX, float cityCenterY, float speed) {
    for (int i = 0; i < 5; i++) {
        float dirX = cityCenterX - heliPositions[i].x;
        float dirY = cityCenterY - heliPositions[i].y;

        // Normalize
        float distance = sqrt(dirX * dirX + dirY * dirY);
        dirX /= distance;
        dirY /= distance;

        heliPositions[i].x += dirX * speed;
        heliPositions[i].y += dirY * speed;
    }
}

void selectHeli(int heliIndex) {
    if (heliPositions[heliIndex].x < 101.0f) {
        selectedHeli = heliIndex;
    }
}

bool collisionTest(float object1X, float object1Y, float object1Radius, float object2X, float object2Y, float object2Radius) {
    float distance = std::sqrt(std::pow(object2X - object1X, 2) + std::pow(object2Y - object1Y, 2));
    return distance < (object1Radius + object2Radius);
}

void mouseCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        if (!pvoCenterSet) {
            pvoCenterX = (float)(2.0 * mouseX / width - 1.0);
            pvoCenterY = (float)(1.0 - 2.0 * mouseY / height);
            pvoCenterSet = true;
        }

        else if (!cityCenterSet) {
            cityCenterX = (float)(2.0 * mouseX / width - 1.0);
            cityCenterY = (float)(1.0 - 2.0 * mouseY / height);
            cityCenterSet = true;
        }
    }
}

void normalizeVector(float& x, float& y) {
    float length = sqrt(x * x + y * y);
    if (length != 0) {
        x /= length;
        y /= length;
    }
}

static unsigned loadImageToTexture(const char* filePath) {
    int TextureWidth;
    int TextureHeight;
    int TextureChannels;
    unsigned char* ImageData = stbi_load(filePath, &TextureWidth, &TextureHeight, &TextureChannels, 0);

    if (ImageData != NULL)
    {
        stbi__vertical_flip(ImageData, TextureWidth, TextureHeight, TextureChannels);
        GLint InternalFormat = -1;
        switch (TextureChannels) {
        case 1: InternalFormat = GL_RED; break;
        case 3: InternalFormat = GL_RGB; break;
        case 4: InternalFormat = GL_RGBA; break;
        default: InternalFormat = GL_RGB; break;
        }

        unsigned int Texture;
        glGenTextures(1, &Texture);
        glBindTexture(GL_TEXTURE_2D, Texture);
        glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, TextureWidth, TextureHeight, 0, InternalFormat, GL_UNSIGNED_BYTE, ImageData);
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(ImageData);
        return Texture;
    }
    else
    {
        std::cout << "Fatal loading error!" << std::endl;
        stbi_image_free(ImageData);
        return 0;
    }
}

int main(void)
{
    // Pattern, from PDF
    if (!glfwInit())
    {
        std::cout << "GLFW LOAD ERROR!\n";
        return 1;
    }

    createHelis(5);     // Added 5 helis

    for (int i = 0; i < 10; ++i) {      // Rockets loaded, 10 units
        rockets[i] = { pvoCenterX, pvoCenterY, 0.0f, 0.0f, false, -1 };
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window;
    unsigned int wWidth = 1500;
    unsigned int wHeight = 1200;
    const char wTitle[] = "PVO SISTEM, ZVORNIK";
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    int xPos = (mode->width - wWidth) / 2;
    int yPos = (mode->height - wHeight) / 2;
    window = glfwCreateWindow(wWidth, wHeight, wTitle, NULL, NULL);
    glfwSetWindowPos(window, xPos, yPos);

    if (window == NULL)
    {
        std::cout << "WINDOW CREATION ERROR!\n";
        glfwTerminate();
        return 2;
    }

    glfwMakeContextCurrent(window);

    // OpenGL state, pattern
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (glewInit() != GLEW_OK)
    {
        std::cout << "GLEW LOAD ERROR!\n";
        return 3;
    }

    glfwSetMouseButtonCallback(window, mouseCallback);

    unsigned int unifiedShader = createShader("texture.vert", "texture.frag");
    unsigned int baseShader = createShader("base.vert", "base.frag");
    unsigned int cityShader = createShader("city.vert", "city.frag");
    unsigned int rocketShader = createShader("rocket.vert", "rocket.frag");
    unsigned int textShader = createShader("text.vert", "text.frag");
    unsigned int greenShader = createShader("green.vert", "green.frag");
    int colorLoc = glGetUniformLocation(unifiedShader, "color");

    loadFont("fonts/ariali.ttf");
    initTextRendering();

    float vertices[] = {
    -1.0, -1.0,  0.0, 0.0,
     1.0, -1.0,  1.0, 0.0,
    -1.0,  1.0,  0.0, 1.0,

     1.0, -1.0,  1.0, 0.0,
     1.0,  1.0,  1.0, 1.0
    };

    unsigned int stride = (2 + 2) * sizeof(float);
    unsigned int VAO[3];
    glGenVertexArrays(3, VAO);
    unsigned int VBO[3];
    glGenBuffers(3, VBO);

    // VAO, VBO
    glGenVertexArrays(1, &VAO[0]);
    glBindVertexArray(VAO[0]);
    glGenBuffers(1, &VBO[0]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Render
    unsigned mapTexture = loadImageToTexture("res/zvornik.png");
    glBindTexture(GL_TEXTURE_2D, mapTexture);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    unsigned uTexLoc = glGetUniformLocation(unifiedShader, "uTex");
    glUniform1i(uTexLoc, 0);

    float centarZv[CRES * 2 + 4];

    float blueCircle[CRES * 2 + 4];
    setCircle(blueCircle, 0.03, 0.0, 0.0);

    // VAO, VBO, rockets
    unsigned int VAOBlue, VBOBlue;
    glGenVertexArrays(1, &VAOBlue);
    glGenBuffers(1, &VBOBlue);
    glBindVertexArray(VAOBlue);
    glBindBuffer(GL_ARRAY_BUFFER, VBOBlue);
    glBufferData(GL_ARRAY_BUFFER, sizeof(blueCircle), blueCircle, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    int rocketsLeft = REMAINING_ROCKETS;

    // VAO, VBO, remaining rockets
    unsigned int VAOrocketLeft[REMAINING_ROCKETS];
    unsigned int VBOrocketLeft[REMAINING_ROCKETS];
    float rocketLeftCircle[CRES * 2 + 4];
    for (int i = 0; i < rocketsLeft; ++i) {

        setCircle(rocketLeftCircle, 0.02, 0.6 + 0.04 * i, -0.8);

        glGenVertexArrays(1, &VAOrocketLeft[i]);
        glGenBuffers(1, &VBOrocketLeft[i]);
        glBindVertexArray(VAOrocketLeft[i]);
        glBindBuffer(GL_ARRAY_BUFFER, VBOrocketLeft[i]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(rocketLeftCircle), rocketLeftCircle, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    // VAO, VBO, remaining helis
    float helicoptersLeftCircle[CRES * 2 + 4];
    unsigned int VAOhelicoptersLeft[5];
    unsigned int VBOhelicoptersLeft[5];
    for (int i = 0; i < 5; ++i) {

        setCircle(helicoptersLeftCircle, 0.02, 0.6 + 0.04 * i, -0.7);

        glGenVertexArrays(1, &VAOhelicoptersLeft[i]);
        glGenBuffers(1, &VBOhelicoptersLeft[i]);
        glBindVertexArray(VAOhelicoptersLeft[i]);
        glBindBuffer(GL_ARRAY_BUFFER, VBOhelicoptersLeft[i]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(helicoptersLeftCircle), helicoptersLeftCircle, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    // VAO, VBO, city status indicator
    float cityStatusCircle[CRES * 2 + 4];
    unsigned int VAOcityStatus;
    unsigned int VBOcityStatus;
    setCircle(cityStatusCircle, 0.02, 0.6, -0.6);

    glGenVertexArrays(1, &VAOcityStatus);
    glGenBuffers(1, &VBOcityStatus);
    glBindVertexArray(VAOcityStatus);
    glBindBuffer(GL_ARRAY_BUFFER, VBOcityStatus);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cityStatusCircle), cityStatusCircle, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    int applyTranslationLoc = glGetUniformLocation(baseShader, "applyTranslation");

    float baseCircle[CRES * 2 + 4];
    setCircle(baseCircle, pvoCenterRad, 0.0, 0.0);

    // VAO, VBO, PVO
    glBindVertexArray(VAO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(baseCircle), baseCircle, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    int translationLocB = glGetUniformLocation(baseShader, "translation");

    // VAO, VBO, city
    setCircle(centarZv, cityCenterRad, 0.0, 0.0);

    glBindVertexArray(VAO[2]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(centarZv), centarZv, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    int translationLocC = glGetUniformLocation(cityShader, "uTranslation");
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(wWidth),
        0.0f, static_cast<float>(wHeight));
    glUseProgram(textShader);
    glUniformMatrix4fv(glGetUniformLocation(textShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));


    float greenVertices[] = {
        -1.0f, 1.0f,
        -1.0f, -1.0f,
        1.0f, -1.0f,
        1.0f, 1.0f
    };

    unsigned int greenIndices[] = {
    0, 1, 2,
    2, 3, 0
    };

    // VAO, VBO, EBO
    unsigned int VAOgreen, VBOgreen, EBOgreen;
    glGenVertexArrays(1, &VAOgreen);
    glGenBuffers(1, &VBOgreen);
    glGenBuffers(1, &EBOgreen);

    glBindVertexArray(VAOgreen);

    glBindBuffer(GL_ARRAY_BUFFER, VBOgreen);
    glBufferData(GL_ARRAY_BUFFER, sizeof(greenVertices), greenVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOgreen);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(greenIndices), greenIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    const double frameDuration = 1000.0 / 60.0;

    while (!glfwWindowShouldClose(window))
    {
        //renderBottomRightText(textShader, "Nikola Eric RA157/2021", 0.1f, glm::vec3(0.0f, 0.0f, 0.0f)); method deleted, found an easier way

        auto frameStart = std::chrono::high_resolution_clock::now();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }

        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        {
            isMapHidden = true;
        }

        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        {
            isMapHidden = false;
        }

        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
        {
            selectHeli(0);
        }

        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
        {
            selectHeli(1);
        }

        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
        {
            selectHeli(2);
        }

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            selectHeli(3);
        }

        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        {
            selectHeli(4);
        }

        glUseProgram(unifiedShader);
        glBindVertexArray(VAO[0]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mapTexture);

        if (!isMapHidden)
        {
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 5);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        glUseProgram(greenShader);
        int colorLoc = glGetUniformLocation(greenShader, "overlayColor");
        glUniform4f(colorLoc, 0.0f, 0.7f, 0.0f, 0.6f);
        glBindVertexArray(VAOgreen);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glUseProgram(baseShader);

        // Draw, remaining rockets
        for (int i = 0; i < rocketsLeft; ++i) {
            glUniform1i(applyTranslationLoc, false);
            glBindVertexArray(VAOrocketLeft[i]);
            colorLoc = glGetUniformLocation(baseShader, "color");
            glUniform3f(colorLoc, 0.0, 0.0, 1.0);
            glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(rocketLeftCircle) / (2 * sizeof(float)));
        }

        // Draw, remaining helis
        for (int i = 0; i < remainingHelis; ++i) {
            glUniform1i(applyTranslationLoc, false);
            glBindVertexArray(VAOhelicoptersLeft[i]);
            colorLoc = glGetUniformLocation(baseShader, "color");
            glUniform3f(colorLoc, 1.0, 0.0, 0.0);
            glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(helicoptersLeftCircle) / (2 * sizeof(float)));
        }
        // Draw, hit helis
        for (int i = remainingHelis; i < 5; ++i) {
            glUniform1i(applyTranslationLoc, false);
            glBindVertexArray(VAOhelicoptersLeft[i]);
            colorLoc = glGetUniformLocation(baseShader, "color");
            glUniform3f(colorLoc, 0.0, 1.0, 0.0);
            glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(helicoptersLeftCircle) / (2 * sizeof(float)));
        }

        // Draw, city status
        glUniform1i(applyTranslationLoc, false);
        glBindVertexArray(VAOcityStatus);
        colorLoc = glGetUniformLocation(baseShader, "color");

        if (hits == 0) {
            glUniform3f(colorLoc, 0.0, 1.0, 0.0);       // Green
        }
        else if (hits == 1) {
            glUniform3f(colorLoc, 1.0, 1.0, 0.0);       // Yellow
        }
        else {
            glUniform3f(colorLoc, 1.0, 0.0, 0.0);       // Red
        }

        glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(cityStatusCircle) / (2 * sizeof(float)));

        // Translation
        if (pvoCenterSet) {
            glUseProgram(baseShader);
            glUniform1i(applyTranslationLoc, true);
            glUniform2f(translationLocB, pvoCenterX, pvoCenterY);

            glBindVertexArray(VAO[1]);
            colorLoc = glGetUniformLocation(baseShader, "color");
            glUniform3f(colorLoc, 0.0, 0.0, 1.0);
            glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(baseCircle) / (2 * sizeof(float)));
        }

        if (cityCenterSet) {
            glUseProgram(cityShader);
            glUniform2f(translationLocC, cityCenterX, cityCenterY);

            glBindVertexArray(VAO[2]);
            colorLoc = glGetUniformLocation(cityShader, "color");
            glUniform3f(colorLoc, 1.0, 1.0, 0.0);
            glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(centarZv) / (2 * sizeof(float)));
        }

        if (simulationOver && hits >= 2) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            float offset = 0.5f;                                                                           // Thickness of the bold effect
            renderText(textShader, "MISSION FAILED!", 50 - offset, 450, 1.5, glm::vec3(0.0f, 0.0f, 0.0f)); // Left
            renderText(textShader, "MISSION FAILED!", 50 + offset, 450, 1.5, glm::vec3(0.0f, 0.0f, 0.0f)); // Right
            renderText(textShader, "MISSION FAILED!", 50, 450 - offset, 1.5, glm::vec3(0.0f, 0.0f, 0.0f)); // Down
            renderText(textShader, "MISSION FAILED!", 50, 450 + offset, 1.5, glm::vec3(0.0f, 0.0f, 0.0f)); // Up
            renderText(textShader, "MISSION FAILED!", 50, 450, 1.5, glm::vec3(0.0f, 0.0f, 0.0f));

            simulationCounter++;
            if (simulationCounter >= 4) {
                std::exit(EXIT_FAILURE);
            }

        }
        else if (simulationOver && remainingHelis <= 0) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            float offset = 0.5f;
            renderText(textShader, "DEFENSE SUCCESSFUL!", 50 - offset, 450, 1.5, glm::vec3(0.0f, 0.0f, 0.0f)); // Left
            renderText(textShader, "DEFENSE SUCCESSFUL!", 50 + offset, 450, 1.5, glm::vec3(0.0f, 0.0f, 0.0f)); // Right
            renderText(textShader, "DEFENSE SUCCESSFUL!", 50, 450 - offset, 1.5, glm::vec3(0.0f, 0.0f, 0.0f)); // Down
            renderText(textShader, "DEFENSE SUCCESSFUL!", 50, 450 + offset, 1.5, glm::vec3(0.0f, 0.0f, 0.0f)); // Up
            renderText(textShader, "DEFENSE SUCCESSFUL!", 50, 450, 1.5, glm::vec3(0.0f, 0.0f, 0.0f));

            simulationCounter++;
            if (simulationCounter >= 4) {
                std::exit(EXIT_FAILURE);
            }
        }

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !spacePressedCurrently) {
            if (selectedHeli != -1 && rocketsLeft > 0 && !rockets[rocketsLeft - 1].isInbound) {
                rocketsLeft--;

                rockets[rocketsLeft].x = pvoCenterX;
                rockets[rocketsLeft].y = pvoCenterY;
                rockets[rocketsLeft].isInbound = true;
                rockets[rocketsLeft].heliTarget = selectedHeli;

                float targetX = heliPositions[selectedHeli].x;
                float targetY = heliPositions[selectedHeli].y;
                rockets[rocketsLeft].dirX = targetX - pvoCenterX;
                rockets[rocketsLeft].dirY = targetY - pvoCenterY;
                normalizeVector(rockets[rocketsLeft].dirX, rockets[rocketsLeft].dirY);
            }

            spacePressedCurrently = true;
        }

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
            spacePressedCurrently = false;
        }

        // Time elapsed
        auto currentTime = std::chrono::high_resolution_clock::now();
        float elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;


        for (int i = 0; i < 10; ++i) {
            if (rockets[i].isInbound) {
                int targetHel = rockets[i].heliTarget;      // Target index, update direction

                float currentHelX = heliPositions[targetHel].x;
                float currentHelY = heliPositions[targetHel].y;

                rockets[i].dirX = currentHelX - rockets[i].x;
                rockets[i].dirY = currentHelY - rockets[i].y;
                normalizeVector(rockets[i].dirX, rockets[i].dirY);

                rockets[i].x += rockets[i].dirX * rocketVelocity;       // Update rocket
                rockets[i].y += rockets[i].dirY * rocketVelocity;

                float distance = std::sqrt(std::pow(rockets[i].x - currentHelX, 2) + std::pow(rockets[i].y - currentHelY, 2));
                renderText(textShader, std::to_string((int)(distance * 1000)) + "m", rockets[i].x * wWidth / 2 + wWidth / 2, rockets[i].y * wHeight / 2 + wHeight / 2 - 30, 0.3, glm::vec3(0.0f, 0.0f, 0.0f));

                if (collisionTest(rockets[i].x, rockets[i].y, rocketRad,
                    currentHelX, currentHelY, helicopterRad)) {

                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::uniform_real_distribution<> dist(0.0, 1.0);

                    double randomValue = dist(gen);

                    if (randomValue <= 0.75) {
                        rockets[i].x = 1000.0;
                        rockets[i].y = 1000.0;
                        heliPositions[targetHel].x = 1000.0;
                        heliPositions[targetHel].y = 1000.0;
                        rockets[i].isInbound = false;
                        rockets[i].heliTarget = -1;
                        selectedHeli = -1;

                        for (int j = 0; j < 10; j++) {
                            if (rockets[j].isInbound && rockets[j].heliTarget == targetHel) {
                                rockets[j].x = 1000.0;
                                rockets[j].y = 1000.0;
                                rockets[j].isInbound = false;
                                rockets[j].heliTarget = -1;                             
                            }
                        }

                        remainingHelis--;
                    }
                    else {
                        rockets[i].x = 1000.0;
                        rockets[i].y = 1000.0;
                        rockets[i].isInbound = false;
                        rockets[i].heliTarget = -1;
                    }
                }

                glUseProgram(rocketShader);
                glBindVertexArray(VAOBlue);
                GLint translationLoc = glGetUniformLocation(rocketShader, "uTranslation");
                glUniform2f(translationLoc, rockets[i].x, rockets[i].y);
                colorLoc = glGetUniformLocation(rocketShader, "color");
                glUniform3f(colorLoc, 0.0, 1.0, 0.0);
                glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(blueCircle) / (2 * sizeof(float)));
            }
        }

        for (int i = 0; i < 5; i++) {
            float dirX = cityCenterX - heliPositions[i].x;
            float dirY = cityCenterY - heliPositions[i].y;

            float distance = sqrt(dirX * dirX + dirY * dirY);
            dirX /= distance;
            dirY /= distance;

            float pulseSpeed = 5.0f + 10.0f * (1.0f - distance);        // Pulsating, adjusting
            float pulseFactor = 0.5f + 0.5f * sin(elapsedTime * pulseSpeed);
            float redIntensity = 1.0;
            float greenIntensity = 1.0 - pulseFactor;
            float blueIntensity = 1.0 - pulseFactor;

            if (i == selectedHeli) {        // If not shot at, color
                redIntensity = 1.0f;
                blueIntensity = 1.0f;
                greenIntensity = 0.0f;
            }

            glUseProgram(rocketShader);
            glBindVertexArray(VAOBlue);
            GLint translationLoc = glGetUniformLocation(rocketShader, "uTranslation");
            glUniform2f(translationLoc, heliPositions[i].x, heliPositions[i].y);
            colorLoc = glGetUniformLocation(rocketShader, "color");
            glUniform3f(colorLoc, redIntensity, greenIntensity, blueIntensity);
            glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(blueCircle) / (2 * sizeof(float)));
            renderText(textShader, heliNames[i], heliPositions[i].x * wWidth / 2 + wWidth / 2 - 5, heliPositions[i].y * wHeight / 2 + wHeight / 2 - 5, 0.3, glm::vec3(0.0f, 0.0f, 0.0f));

            if (collisionTest(heliPositions[i].x, heliPositions[i].y, helicopterRad, cityCenterX, cityCenterY, cityCenterRad)) {
                heliPositions[i].x = 1000.0f;
                heliPositions[i].y = 1000.0f;
                hits++;
                remainingHelis--;

                for (int j = 0; j < 10; j++) {
                    if (rockets[j].isInbound && rockets[j].heliTarget == i) {
                        rockets[j].isInbound = false;
                        rockets[j].x = 1000.0;
                        rockets[j].y = 1000.0;
                    }
                }

                selectedHeli = -1;

                if (hits >= 2) {
                    simulationOver = true;
                }
            }

            if (remainingHelis <= 0) {
                simulationOver = true;
            }
        }

        if (cityCenterSet) {
            if (!initWait) {
                std::this_thread::sleep_for(std::chrono::seconds(3));
                initWait = true;
            }

            moveHelis(cityCenterX, cityCenterY, rocketVelocity / 2);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
        auto frameEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = frameEnd - frameStart;

        if (elapsed.count() < frameDuration) {
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(frameDuration - elapsed.count())));
        }
    }

    glDeleteTextures(1, &mapTexture);
    glDeleteBuffers(3, VBO);
    glDeleteVertexArrays(3, VAO);
    glDeleteBuffers(1, &VBOBlue);
    glDeleteVertexArrays(1, &VAOBlue);
    glDeleteProgram(unifiedShader);
    glDeleteProgram(baseShader);

    for (int i = 0; i < REMAINING_ROCKETS; i++) {
        glDeleteVertexArrays(1, &VAOrocketLeft[i]);
        glDeleteBuffers(1, &VBOrocketLeft[i]);
    }

    for (int i = 0; i < 5; i++) {
        glDeleteVertexArrays(1, &VAOhelicoptersLeft[i]);
        glDeleteBuffers(1, &VBOhelicoptersLeft[i]);
    }

    glfwTerminate();
    return 0;
}