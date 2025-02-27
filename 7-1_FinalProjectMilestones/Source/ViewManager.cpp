///////////////////////////////////////////////////////////////////////////////
// ViewManager.cpp
// ============
// Manage the viewing of 3D objects within the viewport.
//
// AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
// Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Declarations for the global variables and defines
namespace
{
    // Window size
    const int WINDOW_WIDTH = 1000;
    const int WINDOW_HEIGHT = 800;
    const char* g_ViewName = "view";
    const char* g_ProjectionName = "projection";

    // Camera object for scene interaction
    Camera* g_pCamera = nullptr;

    // Mouse movement variables
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // Frame timing variables
    float gDeltaTime = 0.0f;
    float gLastFrame = 0.0f;

    // Projection mode toggle (false = perspective, true = orthographic)
    bool bOrthographicProjection = false;

    // Adjustable movement speed (modified via scroll wheel)
    float gMovementSpeed = 2.5f;
}

/***********************************************************
 *  Constructor: ViewManager()
 *
 *  Initializes the view manager and camera settings.
 ***********************************************************/
ViewManager::ViewManager(ShaderManager* pShaderManager)
{
    m_pShaderManager = pShaderManager;
    m_pWindow = NULL;
    g_pCamera = new Camera();

    // Default camera position and orientation
    g_pCamera->Position = glm::vec3(0.0f, 5.5f, 8.0f);
    g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
    g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
    g_pCamera->Zoom = 80;
}

/***********************************************************
 *  Destructor: ~ViewManager()
 *
 *  Cleans up allocated resources.
 ***********************************************************/
ViewManager::~ViewManager()
{
    m_pShaderManager = NULL;
    m_pWindow = NULL;
    if (g_pCamera != NULL)
    {
        delete g_pCamera;
        g_pCamera = NULL;
    }
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  Creates the main OpenGL display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, windowTitle, NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return NULL;
    }
    glfwMakeContextCurrent(window);

    // Register mouse movement and scroll callbacks
    glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);
    glfwSetScrollCallback(window, &ViewManager::Mouse_Scroll_Callback);

    // Capture all mouse events
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Enable blending for transparency support
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_pWindow = window;
    return window;
}

/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  Handles mouse movement input for camera control.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
    if (gFirstMouse)
    {
        gLastX = xMousePos;
        gLastY = yMousePos;
        gFirstMouse = false;
    }

    float xOffset = xMousePos - gLastX;
    float yOffset = gLastY - yMousePos; // Reversed

    gLastX = xMousePos;
    gLastY = yMousePos;

    g_pCamera->ProcessMouseMovement(xOffset, yOffset);
}

/***********************************************************
 *  Mouse_Scroll_Callback()
 *
 *  Handles mouse scroll input for movement speed adjustment.
 ***********************************************************/
void ViewManager::Mouse_Scroll_Callback(GLFWwindow* window, double xOffset, double yOffset)
{
    if (yOffset > 0)  // Scroll up → Increase speed
        gMovementSpeed += 0.5f;
    else if (yOffset < 0) // Scroll down → Decrease speed
        gMovementSpeed = std::max(0.5f, gMovementSpeed - 0.5f); // Keep minimum value
}

/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  Processes keyboard input for camera movement.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
    if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(m_pWindow, true);
    }

    if (g_pCamera == NULL)
        return;

    // Apply movement speed factor to camera movements
    if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
        g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime * gMovementSpeed);

    if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
        g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime * gMovementSpeed);

    if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
        g_pCamera->ProcessKeyboard(LEFT, gDeltaTime * gMovementSpeed);

    if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
        g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime * gMovementSpeed);

    if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
        g_pCamera->ProcessKeyboard(UP, gDeltaTime * gMovementSpeed);

    if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
        g_pCamera->ProcessKeyboard(DOWN, gDeltaTime * gMovementSpeed);

    if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS)
    {
        bOrthographicProjection = false;
        std::cout << "Switched to Perspective View" << std::endl;

        // Adjust camera for a standard 3D view
        g_pCamera->Position = glm::vec3(0.0f, 5.5f, 8.0f);
        g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
        g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
        g_pCamera->Zoom = 80.0f;
    }

    // Toggle to Orthographic View
    if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS)
    {
        bOrthographicProjection = true;
        std::cout << "Switched to Orthographic View" << std::endl;

        // Adjust camera to look directly at the object (flat view)
        g_pCamera->Position = glm::vec3(0.0f, 5.0f, 10.0f);
        g_pCamera->Front = glm::vec3(0.0f, 0.0f, -1.0f);
        g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
        g_pCamera->Zoom = 1.0f;
    }
}

/***********************************************************
 *  PrepareSceneView()
 *
 *  Sets up the camera's view and projection matrices.
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
    float currentFrame = glfwGetTime();
    gDeltaTime = currentFrame - gLastFrame;
    gLastFrame = currentFrame;

    ProcessKeyboardEvents(); // Process keyboard input

    glm::mat4 view = g_pCamera->GetViewMatrix();
    glm::mat4 projection;

    if (!bOrthographicProjection)
    {
        // Perspective Projection (3D View)
        projection = glm::perspective(glm::radians(g_pCamera->Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }
    else
    {
        // Orthographic Projection (2D View)
        float orthoSize = 5.0f;
        float aspectRatio = (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT;

        if (aspectRatio >= 1.0f)
        {
            projection = glm::ortho(-orthoSize * aspectRatio, orthoSize * aspectRatio, -orthoSize, orthoSize, 0.1f, 100.0f);
        }
        else
        {
            projection = glm::ortho(-orthoSize, orthoSize, -orthoSize / aspectRatio, orthoSize / aspectRatio, 0.1f, 100.0f);
        }
    }

    if (m_pShaderManager != NULL)
    {
        m_pShaderManager->setMat4Value(g_ViewName, view);
        m_pShaderManager->setMat4Value(g_ProjectionName, projection);
        m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
    }
}
