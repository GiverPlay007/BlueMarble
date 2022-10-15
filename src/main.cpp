#include <iostream>

#include <GLFW/glfw3.h>
#include <cassert>

const int width = 1280;
const int height = 720;

int main()
{
  assert(glfwInit( ));

  GLFWwindow* window = glfwCreateWindow(width, height, "Blue Marble", nullptr, nullptr);
  assert(window);

  while(!glfwWindowShouldClose(window))
  {
    glfwPollEvents();
    glfwSwapBuffers(window);
  }

  glfwTerminate();
  return 0;
}