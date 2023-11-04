#include <iostream>
#include <cassert>
#include <array>
#include <fstream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

const int width = 1280;
const int height = 720;

void printGlVersion();

void checkShaderErrors(GLuint shaderId);

GLuint compileShaderProgram(const char* shadersPath);

int main()
{
  // Create window and OpenGL context
  assert(glfwInit() == GLFW_TRUE);

  GLFWwindow* window = glfwCreateWindow(width, height, "Blue Marble", nullptr, nullptr);
  assert(window);

  glfwMakeContextCurrent(window);
  assert(glewInit() == GLEW_OK);

  printGlVersion();

  // Compile the shader program
  GLuint shaderProgramId =  compileShaderProgram("shaders/triangle");

  // Define triangle
  std::array<glm::vec3, 6> triangle = {
    glm::vec3 { -0.5f, -0.5f, 0.0f },
    glm::vec3 { 0.5f, -0.5f, 0.0f },
    glm::vec3 { 0.0f, 0.5f, 0.0f },
  };

  // Generate model matrix
  glm::mat4 modelMatrix = glm::identity<glm::mat4>();

  // Generate view matrix
  glm::vec3 eye { 0.0f, 0.0f, 10.0f };
  glm::vec3 center { 0.0f, 0.0f, 0.0f };
  glm::vec3 up { 0.0f, 1.0f, 0.0f };
  glm::mat4 viewMatrix = glm::lookAt(eye, center, up);

  // Generate projection matrix
  constexpr float fov = glm::radians(45.0f);
  constexpr float aspectRatio = width / height;
  const float near = 0.001f;
  const float far = 1000.0f;
  glm::mat4 projectionMatrix = glm::perspective(fov, aspectRatio, near, far);

  // Generate model view projection matrix
  glm::mat4 modelViewProjectionMatrix = projectionMatrix * viewMatrix * modelMatrix;

  // Apply model view projection to the triangle vertices
  for(glm::vec3& vertex : triangle)
  {
    glm::vec4 projectedVertex = modelViewProjectionMatrix * glm::vec4 { vertex, 1.0f };
    projectedVertex /= projectedVertex.w;
    vertex = projectedVertex;
  }

  // Generate triangle vertex buffer
  GLuint vertexBuffer;

  glGenBuffers(1, &vertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle.data(), GL_STATIC_DRAW);

  while(!glfwWindowShouldClose(window))
  {
    // Process the screen events
    glfwPollEvents();

    // Clear the screen
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Activate shader program
    glUseProgram(shaderProgramId);

    // Bind triangle vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr); 

    // Draw the triangle
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Unbind triangle vertices
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(0);

    // Disable shader program
    glUseProgram(0);

    // Copy draw buffers to the screen
    glfwSwapBuffers(window);
  }

  glDeleteBuffers(1, &vertexBuffer);
  glfwTerminate();
  
  return 0;
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
