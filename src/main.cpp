#include <iostream>
#include <cassert>
#include <array>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

const int width = 1280;
const int height = 720;

void print_gl_version();

int main()
{
  assert(glfwInit() == GLFW_TRUE);

  GLFWwindow* window = glfwCreateWindow(width, height, "Blue Marble", nullptr, nullptr);
  assert(window);

  glfwMakeContextCurrent(window);
  assert(glewInit() == GLEW_OK);

  print_gl_version();

  std::array<glm::vec3, 6> triangle = {
    glm::vec3 { -0.5f, -0.5f, 0.0f },
    glm::vec3 { 0.5f, -0.5f, 0.0f  },
    glm::vec3 { 0.0f, 0.5f, 0.0f },
  };

  GLuint vertexBuffer;

  glGenBuffers(1, &vertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle.data(), GL_STATIC_DRAW);

  while(!glfwWindowShouldClose(window))
  {
    glfwPollEvents();
    
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr); 

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(0);

    glfwSwapBuffers(window);
  }

  glDeleteBuffers(1, &vertexBuffer);
  glfwTerminate();
  
  return 0;
}

void print_gl_version()
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