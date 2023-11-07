#include <iostream>
#include <cassert>
#include <array>
#include <fstream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

const int width = 800;
const int height = 600;

void mouseButtonCallback(GLFWwindow* window, int button, int action, int modifiers);

void mouseMotionCallback(GLFWwindow* window, double x, double y);

void printGlVersion();

void checkShaderErrors(GLuint shaderId);

GLuint compileShaderProgram(const char* shadersPath);

GLuint loadTexture(const char* texturePath);

struct vertex_t
{
  glm::vec3 position;
  glm::vec3 color;
  glm::vec2 UV;
};

class FlyCamera
{
public:
  // View matrix
  glm::vec3 location  { 0.0f, 0.0f,  5.0f };
  glm::vec3 direction { 0.0f, 0.0f, -1.0f };
  glm::vec3 up        { 0.0f, 1.0f,  0.0f };

  // Projection matrix
  float aspectRatio = width / height;
  float fov = glm::radians(45.0f);
  float near = 0.01f;
  float far = 1000.0f;

  // Camera movement
  float speed = 2.0f;
  float sensitivity = 0.1f;

  glm::mat4 getViewProjection() const
  {
    glm::mat4 view = glm::lookAt(location, location + direction, up);
    glm::mat4 projection = glm::perspective(fov, aspectRatio, near, far);

    return projection * view;
  }

  void look(float yaw, float pitch)
  {
    yaw *= sensitivity;
    pitch *= sensitivity;

    const glm::vec3 right = glm::normalize(glm::cross(direction, up));
    const glm::mat4 identity = glm::identity<glm::mat4>();

    glm::mat4 pitchRotation = glm::rotate(identity, glm::radians(pitch), right);
    glm::mat4 yawRotation = glm::rotate(identity, glm::radians(yaw), up);

    up = pitchRotation * glm::vec4 { up, 0.0f };
    direction = yawRotation * pitchRotation * glm::vec4 { direction, 0.0f };
  }

  void moveForward(float amount)
  {
    location += glm::normalize(direction) * amount * speed;
  }

  void moveRight(float amount)
  {
    glm::vec3 right = glm::normalize(glm::cross(direction, up));
    location += right * amount * speed;
  }
};

FlyCamera camera;
bool enableMouseMovement;

glm::vec2 previousCursor { 0.0f, 0.0f };

int main()
{
  // Create window and OpenGL context
  assert(glfwInit() == GLFW_TRUE);

  // Create GLFW window
  GLFWwindow* window = glfwCreateWindow(width, height, "Blue Marble", nullptr, nullptr);
  assert(window);

  // Register window callbacks
  glfwSetMouseButtonCallback(window, mouseButtonCallback);
  glfwSetCursorPosCallback(window, mouseMotionCallback);

  // Configure window
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // V-Sync

  // Check if glew has been initialized
  assert(glewInit() == GLEW_OK);

  printGlVersion();

  // Compile the shader program
  GLuint shaderProgramId =  compileShaderProgram("shaders/triangle");

  // Load Earth texture (NASA/public domain)
  GLuint textureId = loadTexture("textures/earth.jpg");

  // Define quad
  std::array<vertex_t, 4> quad = {
    vertex_t { glm::vec3 { -1.0f, -1.0f,  0.0f },
               glm::vec3 {  1.0f,  0.0f,  0.0f },
               glm::vec2 {  0.0f,  0.0f }, },

    vertex_t { glm::vec3 {  1.0f, -1.0f,  0.0f },
               glm::vec3 {  0.0f,  1.0f,  0.0f },
               glm::vec2 {  1.0f,  0.0f }, },

    vertex_t { glm::vec3 {  1.0f,  1.0f,  0.0f },
               glm::vec3 {  1.0f,  0.0f,  0.0f },
               glm::vec2 {  1.0f,  1.0f }, },

    vertex_t { glm::vec3 { -1.0f,  1.0f,  0.0f },
               glm::vec3 {  0.0f,  0.0f,  1.0f },
               glm::vec2 {  0.0f,  1.0f }, }
  };

  // Define EBO indexes
  std::array<glm::ivec3, 2> indexes = {
    glm::ivec3 { 0, 1, 3 },
    glm::ivec3 { 3, 1, 2 }
  };

  // Generate model matrix
  glm::mat4 modelMatrix = glm::identity<glm::mat4>();

  // Generate VBO identifier and send data to GPU
  GLuint vertexBuffer;
  glGenBuffers(1, &vertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad.data(), GL_STATIC_DRAW);

  // Generate EBO identifier and send data to GPU
  GLuint elementBuffer;
  glGenBuffers(1, &elementBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexes), indexes.data(), GL_STATIC_DRAW);
  

  // Enable back face culling
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  // Last frame time
  double previousTime = glfwGetTime();

  while(!glfwWindowShouldClose(window))
  {
    // Calculate delta time
    double currentTime = glfwGetTime();
    float deltaTime = (float) (currentTime - previousTime);
    previousTime = currentTime;

    // Process the window events
    glfwPollEvents();

    // Process keyboard input
    char moveForward = 0;
    char moveRight = 0;

    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveForward++;
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveForward--;
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveRight--;
    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveRight++;

    if(moveForward) camera.moveForward(moveForward * deltaTime);
    if(moveRight) camera.moveRight(moveRight * deltaTime);

    // Generate the model view projection matrix
    glm::mat4 viewProjectionMatrix = camera.getViewProjection();
    glm::mat4 modelViewProjectionMatrix = viewProjectionMatrix * modelMatrix;

    // Clear the screen
    glClearColor(0.1f, 0.7f, 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Activate shader program
    glUseProgram(shaderProgramId);

    // Send the model view projection matrix to the shader program
    GLint modelViewProjectionLocation = glGetUniformLocation(shaderProgramId, "modelViewProjection");
    glUniformMatrix4fv(modelViewProjectionLocation, 1, GL_FALSE, glm::value_ptr(modelViewProjectionMatrix));

    // Use the earth texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    GLint textureSamplerLocation = glGetUniformLocation(shaderProgramId, "textureSampler");
    glUniform1i(textureSamplerLocation, 0);

    // Enable vertex attributes
    glEnableVertexAttribArray(0); // Position
    glEnableVertexAttribArray(1); // Color
    glEnableVertexAttribArray(2); // Texture UV

    // Bind the quad buffers
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
    
    // Send quad vertices attributes to the shader program
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), nullptr); // Position
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(vertex_t), reinterpret_cast<void*>(offsetof(vertex_t, color))); // Color
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_TRUE, sizeof(vertex_t), reinterpret_cast<void*>(offsetof(vertex_t, UV))); // Texture UV

    // Draw the quad
    glDrawElements(GL_TRIANGLES, (GLsizei) (indexes.size() * 3), GL_UNSIGNED_INT, nullptr);

    // Unbind quad buffers
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Disable shader attributes
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    // Disable shader program
    glUseProgram(0);

    // Copy draw buffers to the screen
    glfwSwapBuffers(window);
  }

  glDeleteBuffers(1, &vertexBuffer);
  glfwTerminate();
  
  return 0;
}

GLuint loadTexture(const char* texturePath)
{
  std::cout << "Loading texture " << texturePath << std::endl;

  stbi_set_flip_vertically_on_load(true);
  
  int textureWidth = 0;
  int textureHeight = 0;
  int numberOfComponents = 0;

  unsigned char* textureData = stbi_load(texturePath, &textureWidth, &textureHeight, &numberOfComponents, 3);
  assert(textureData);

  // Generate texture identifier
  GLuint textureId;
  glGenTextures(1, &textureId);

  // Bind texture to modify
  glBindTexture(GL_TEXTURE_2D, textureId);

  // Copy texture to the GPU
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureWidth, textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData);

  // Configure texture filters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

  // Configure texture wrapping
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // Generate texture mipmap
  glGenerateMipmap(GL_TEXTURE_2D);

  // Unbind the texture and free memory
  glBindTexture(GL_TEXTURE_2D, 0);
  stbi_image_free(textureData);

  return textureId;
}

std::string readFile(const char* filePath)
{
  std::string fileContent;

  if(std::ifstream fileStream { filePath, std::ios::in })
  {
    fileContent.assign(std::istreambuf_iterator<char>(fileStream), std::istreambuf_iterator<char>());
  }

  return fileContent;
}

GLuint compileShaderProgram(const char* shadersPath)
{
  std::string vertexPath = std::string(shadersPath) + ".vert";
  std::string fragmentPath = std::string(shadersPath) + ".frag";

  std::string vertexShaderSource = readFile(vertexPath.c_str());
  std::string fragmentShaderSource = readFile(fragmentPath.c_str());

  assert(!vertexShaderSource.empty());
  assert(!fragmentShaderSource.empty());

  // Create shaders identifiers
  GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
  GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

  const char* vertexShaderSourcePtr = vertexShaderSource.c_str();
  const char* fragmentShaderSourcePtr = fragmentShaderSource.c_str();

  // Compile vertex shader
  std::cout << "Compiling vertex shader " << vertexPath << std::endl;
  glShaderSource(vertexShaderId, 1, &vertexShaderSourcePtr, nullptr);
  glCompileShader(vertexShaderId);
  checkShaderErrors(vertexShaderId);

  // Compile fragment shader
  std::cout << "Compiling fragment shader " << fragmentPath << std::endl;
  glShaderSource(fragmentShaderId, 1, &fragmentShaderSourcePtr, nullptr);
  glCompileShader(fragmentShaderId);
  checkShaderErrors(fragmentShaderId);

  // Link shader program
  GLuint shaderProgramId = glCreateProgram();
  glAttachShader(shaderProgramId, vertexShaderId);
  glAttachShader(shaderProgramId, fragmentShaderId);
  glLinkProgram(shaderProgramId);

  // Check for erros
  GLint result = GL_TRUE;
  glGetProgramiv(shaderProgramId, GL_LINK_STATUS, &result);

  if(result == GL_FALSE)
  {
    std::cout << "Failed to link shader program: " << shadersPath << std::endl;

    GLint infoLogLength = 0;
    glGetProgramiv(shaderProgramId, GL_INFO_LOG_LENGTH, &infoLogLength);

    if(infoLogLength > 0)
    {
      std::string programInfoLog(infoLogLength, '\0');
      glGetProgramInfoLog(shaderProgramId, infoLogLength, nullptr, &programInfoLog[0]);
      std::cout << programInfoLog << std::endl;
    }

    assert (false);
  }

  // Free resources
  glDetachShader(shaderProgramId, vertexShaderId);
  glDetachShader(shaderProgramId, fragmentShaderId);

  glDeleteShader(vertexShaderId);
  glDeleteShader(fragmentShaderId);

  return shaderProgramId;
}

void checkShaderErrors(GLuint shaderId)
{
  GLint result = GL_TRUE;
  glGetShaderiv(shaderId, GL_COMPILE_STATUS, &result);

  if(result == GL_FALSE)
  {
    std::cout << "Failed to compile shader" << std::endl;

    GLint infoLogLength = 0;
    glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &infoLogLength);

    if(infoLogLength > 0)
    {
      std::string shaderInfoLog(infoLogLength, '\0');
      glGetShaderInfoLog(shaderId, infoLogLength, nullptr, &shaderInfoLog[0]);

      std::cout << shaderInfoLog << std::endl;
    }

    assert(false);
  }
}

void printGlVersion()
{
  GLint glMajorVersion = 0;
  GLint glMinorVersion = 0;

  glGetIntegerv(GL_MAJOR_VERSION, &glMajorVersion);
  glGetIntegerv(GL_MINOR_VERSION, &glMinorVersion);

  std::cout << "OpenGL Version: " << glMajorVersion << "." << glMajorVersion << std::endl;
  std::cout << "OpenGl Vendor: " << glGetString(GL_VENDOR) << std::endl;
  std::cout << "OpenGl Renderer: " << glGetString(GL_RENDERER) << std::endl;
  std::cout << "OpenGl Version (renderer): " << glGetString(GL_VERSION) << std::endl;
  std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int modifiers)
{
  if(button == GLFW_MOUSE_BUTTON_LEFT)
  {
    if(action == GLFW_PRESS)
    {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      double x;
      double y;

      glfwGetCursorPos(window, &x, &y);
      
      previousCursor = { x, y };
      enableMouseMovement = true;
    }

    if(action == GLFW_RELEASE)
    {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      enableMouseMovement = false;
    }
  }
}

void mouseMotionCallback(GLFWwindow* window, double x, double y)
{
  if(enableMouseMovement)
  {
    glm::vec2 cursorPosition = glm::vec2 { x, y };
    glm::vec2 deltaCursor = previousCursor - cursorPosition;

    camera.look(deltaCursor.x, deltaCursor.y);
    previousCursor = cursorPosition;
  }
}
