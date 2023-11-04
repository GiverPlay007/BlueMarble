cmake_minimum_required(VERSION 3.12)

project(BlueMarble)

add_executable(BlueMarble "src/main.cpp")

if(WIN32)
  target_include_directories(BlueMarble PRIVATE
    libs/glm
    libs/stb
    libs/glfw/include
    libs/glew/include
  )

  target_link_directories(BlueMarble PRIVATE
    libs/glfw/lib-vc2019
    libs/glew/lib/Release/x64
  )

  target_link_libraries(BlueMarble PRIVATE
    glfw3.lib
    glew32.lib
    opengl32.lib
  )

  add_custom_command(TARGET BlueMarble POST_BUILD COMMAND 
    ${CMAKE_COMMAND} -E copy 
      "${CMAKE_SOURCE_DIR}/libs/glew/bin/Release/x64/glew32.dll"
      "${CMAKE_CURRENT_BINARY_DIR}/glew32.dll"
  )
else()
  find_package(glfw3 REQUIRED)
  find_package(GLEW REQUIRED)
  find_package(OpenGL REQUIRED)

  target_include_directories(BlueMarble PRIVATE 
    libs/stb
    libs/glm
  )

  target_link_libraries(BlueMarble
    GLEW::glew
    glfw
    OpenGL::OpenGL
  )
endif()

add_custom_command(TARGET BlueMarble POST_BUILD COMMAND
  ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/shaders"
    "${CMAKE_CURRENT_BINARY_DIR}/shaders"
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_custom_command(TARGET BlueMarble POST_BUILD COMMAND
    ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_SOURCE_DIR}/shaders"
      "${CMAKE_CURRENT_BINARY_DIR}/Debug/shaders"
    )
  
  if(WIN32)
    add_custom_command(TARGET BlueMarble POST_BUILD COMMAND 
      ${CMAKE_COMMAND} -E copy 
        "${CMAKE_SOURCE_DIR}/libs/glew/bin/Release/x64/glew32.dll"
        "${CMAKE_CURRENT_BINARY_DIR}/Debug/glew32.dll"
    )
  endif()
endif()