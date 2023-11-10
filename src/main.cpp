#include <iostream>
#include <cassert>
#include <array>
#include <vector>
#include <fstream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int width = 800;
int height = 600;

struct vertex_t
{
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec3 color;
  glm::vec2 UV;
};

struct DirectionalLight
{
  glm::vec3 direction;
  GLfloat intensity;
};

void mouseButtonCallback(GLFWwindow* window, int button, int action, int modifiers);

void mouseMotionCallback(GLFWwindow* window, double x, double y);

void resize(GLFWwindow* window, int newWidth, int newHeight);

void printGlVersion();

void checkShaderErrors(GLuint shaderId);

GLuint compileShaderProgram(const char* shadersPath);

GLuint loadTexture(const char* texturePath);

GLuint generateVao();

GLuint generateSphereVao(GLuint& numVertices, GLuint& numIndexes);

void generateSphereMesh(GLuint resolution, std::vector<vertex_t>& vertices, std::vector<glm::ivec3>& indexes);

class FlyCamera
{
public:
  // View matrix
  glm::vec3 location  { 0.0f, 0.0f,  5.0f };
  glm::vec3 direction { 0.0f, 0.0f, -1.0f };
  glm::vec3 up        { 0.0f, 1.0f,  0.0f };

  // Projection matrix
  float aspectRatio = static_cast<float> (width) / height;
  float fov = glm::radians(45.0f);
  float near = 0.01f;
  float far = 1000.0f;

  // Camera movement
  float speed = 2.0f;
  float sensitivity = 0.1f;

  glm::mat4 getViewMatrix() const
  {
    return glm::lookAt(location, location + direction, up);
  }

  glm::mat4 getViewProjection() const
  {
    glm::mat4 projection = glm::perspective(fov, aspectRatio, near, far);
    return projection * getViewMatrix();
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
  glfwSetFramebufferSizeCallback(window, resize);

  // Configure window
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // V-Sync

  // Check if glew has been initialized
  assert(glewInit() == GLEW_OK);

  printGlVersion();

  resize(window, width, height);

  // Compile the shader program
  GLuint shaderProgramId =  compileShaderProgram("shaders/triangle");

  // Load Earth texture (NASA/public domain)
  GLuint textureId = loadTexture("textures/earth.jpg");

  // Generate quad VAO
  GLuint vaoId = generateVao();

  GLuint sphereNumVertices = 0;
  GLuint sphereNumIndexes = 0;
  GLuint sphereVaoId = generateSphereVao(sphereNumVertices, sphereNumIndexes);

  // Generate model matrix
  glm::mat4 modelMatrix = glm::rotate(glm::identity<glm::mat4>(), glm::radians(90.0f), glm::vec3{ 1.0f, 0.0f, 0.0f });

  // Enable back face culling
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  // Enable depth test
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  // Create directional light source
  DirectionalLight light { glm::vec3 { 0.0f, 0.0f, -1.0f }, 1.0f };

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
    glm::mat4 viewMatrix = camera.getViewMatrix();
    glm::mat4 normalMatrix = glm::inverse(glm::transpose(viewMatrix * modelMatrix));
    glm::mat4 viewProjectionMatrix = camera.getViewProjection();
    glm::mat4 modelViewProjectionMatrix = viewProjectionMatrix * modelMatrix;

    // Clear the screen
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Activate shader program
    glUseProgram(shaderProgramId);

    // Send the normal matrix to the shader program
    GLuint normalMatrixLoc = glGetUniformLocation(shaderProgramId, "normalMatrix");
    glUniformMatrix4fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // Send the model view projection matrix to the shader program
    GLint modelViewProjectionLocation = glGetUniformLocation(shaderProgramId, "modelViewProjection");
    glUniformMatrix4fv(modelViewProjectionLocation, 1, GL_FALSE, glm::value_ptr(modelViewProjectionMatrix));

    // Use the earth texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    GLint textureSamplerLocation = glGetUniformLocation(shaderProgramId, "textureSampler");
    glUniform1i(textureSamplerLocation, 0);

    // Directional light direction
    GLint lightDirectionLocation = glGetUniformLocation(shaderProgramId, "lightDirection");
    glUniform3fv(lightDirectionLocation, 1, glm::value_ptr(viewMatrix * glm::vec4{ light.direction, 0.0f }));

    // Directional light intensity
    GLint lightIntensityLocation = glGetUniformLocation(shaderProgramId, "lightIntensity");
    glUniform1f(lightIntensityLocation, light.intensity);

    // Bind VAO
    // glBindVertexArray(vaoId);
    glBindVertexArray(sphereVaoId);

    // Draw the sphere
    glDrawElements(GL_TRIANGLES, sphereNumIndexes, GL_UNSIGNED_INT, nullptr);

    // Unbind VAO
    glBindVertexArray(0);

    // Disable shader program
    glUseProgram(0);

    // Copy draw buffers to the screen
    glfwSwapBuffers(window);
  }

  glDeleteVertexArrays(1, &vaoId);
  glfwTerminate();
  
  return 0;
}

void generateSphereMesh(GLuint resolution, std::vector<vertex_t>& vertices, std::vector<glm::ivec3>& indexes)
{
  vertices.clear();
  indexes.clear();

  constexpr float pi = glm::pi<float>();
  constexpr float twoPi = glm::two_pi<float>();

  const float inverseResolution = 1.0f / static_cast<float>(resolution - 1);

  for(GLuint uIndex = 0; uIndex < resolution; ++uIndex)
  {
    const float u = uIndex * inverseResolution;
    const float phi = glm::mix(0.0f, twoPi, u);

    for(GLuint vIndex = 0; vIndex < resolution; ++vIndex)
    {
      const float v = vIndex * inverseResolution;
      const float theta = glm::mix(0.0f, pi, v);

      glm::vec3 vertexPosition = {
        glm::sin(theta) * glm::cos(phi),
        glm::sin(theta) * glm::sin(phi),
        glm::cos(theta)
      };

      vertex_t vertex {
        vertexPosition,
        glm::normalize(vertexPosition),
        glm::vec3{ 1.0f, 1.0f, 1.0f },
        glm::vec2 { 1.0f - u, v }
      };

      vertices.push_back(vertex);
    }
  }

  for(GLuint u = 0; u < resolution -1; ++u)
  {
    for(GLuint v = 0; v < resolution -1; ++v)
    {
      GLuint p0 = u + v * resolution;
      GLuint p1 = (u + 1) + v * resolution;
      GLuint p2 = (u + 1) + (v + 1) * resolution;
      GLuint p3 = u + (v + 1) * resolution;

      indexes.push_back(glm::ivec3{ p0, p1, p3 });
      indexes.push_back(glm::ivec3{ p3, p1, p2 });
    }
  }
}

GLuint generateSphereVao(GLuint& numVertices, GLuint& numIndexes)
{
  std::vector<vertex_t> vertices;
  std::vector<glm::ivec3> triangles;

  generateSphereMesh(50, vertices, triangles);

  numVertices = static_cast<GLsizei>(vertices.size());
  numIndexes = static_cast<GLsizei>(triangles.size()) * 3;

  // Generate sphere VBO and send data to the GPU
  GLuint vertexBuffer;
  glGenBuffers(1, &vertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex_t), vertices.data(), GL_STATIC_DRAW);

  GLuint elementBuffer;
  glGenBuffers(1, &elementBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndexes * sizeof(GLuint), triangles.data(), GL_STATIC_DRAW);

  GLuint vaoId;
  glGenVertexArrays(1, &vaoId);
  glBindVertexArray(vaoId);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glEnableVertexAttribArray(3);

  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), nullptr); // Position
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(vertex_t), reinterpret_cast<void*>(offsetof(vertex_t, normal))); // Normal
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_TRUE, sizeof(vertex_t), reinterpret_cast<void*>(offsetof(vertex_t, color))); // Color
  glVertexAttribPointer(3, 2, GL_FLOAT, GL_TRUE, sizeof(vertex_t), reinterpret_cast<void*>(offsetof(vertex_t, UV))); // Texture UV

  glBindVertexArray(0);

  return vaoId;
}

GLuint generateVao()
{
  // Define quad
  std::array<vertex_t, 4> quad = {
    vertex_t { glm::vec3 { -1.0f, -1.0f,  0.0f },
               glm::vec3 {  0.0f,  0.0f,  1.0f },
               glm::vec3 {  1.0f,  0.0f,  0.0f },
               glm::vec2 {  0.0f,  0.0f }, },

    vertex_t { glm::vec3 {  1.0f, -1.0f,  0.0f },
               glm::vec3 {  0.0f,  0.0f,  1.0f },
               glm::vec3 {  0.0f,  1.0f,  0.0f },
               glm::vec2 {  1.0f,  0.0f }, },

    vertex_t { glm::vec3 {  1.0f,  1.0f,  0.0f },
               glm::vec3 {  0.0f,  0.0f,  1.0f },
               glm::vec3 {  1.0f,  0.0f,  0.0f },
               glm::vec2 {  1.0f,  1.0f }, },

    vertex_t { glm::vec3 { -1.0f,  1.0f,  0.0f },
               glm::vec3 {  0.0f,  0.0f,  1.0f },
               glm::vec3 {  0.0f,  0.0f,  1.0f },
               glm::vec2 {  0.0f,  1.0f }, }
  };

  // Define EBO indexes
  std::array<glm::ivec3, 2> indexes = {
    glm::ivec3 { 0, 1, 3 },
    glm::ivec3 { 3, 1, 2 }
  };

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

  // Generate vertex array object (VAO)
  GLuint vaoId;
  glGenVertexArrays(1, &vaoId);

  // Bind VAO
  glBindVertexArray(vaoId);

  // Enable vertex attributes
  glEnableVertexAttribArray(0); // Position
  glEnableVertexAttribArray(1); // Normal
  glEnableVertexAttribArray(2); // Color
  glEnableVertexAttribArray(3); // Texture UV

  // Bind the quad buffers
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);

  // Send quad vertices attributes to the shader program
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), nullptr); // Position
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(vertex_t), reinterpret_cast<void*>(offsetof(vertex_t, normal))); // Normal
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_TRUE, sizeof(vertex_t), reinterpret_cast<void*>(offsetof(vertex_t, color))); // Color
  glVertexAttribPointer(3, 2, GL_FLOAT, GL_TRUE, sizeof(vertex_t), reinterpret_cast<void*>(offsetof(vertex_t, UV))); // Texture UV

  // Unbind VAO
  glBindVertexArray(0);
  
  return vaoId;
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

void resize(GLFWwindow* window, int newWidth, int newHeight)
{
  width = newWidth;
  height = newHeight;

  camera.aspectRatio = static_cast<float> (width) / height;
  glViewport(0, 0, width, height);
}
