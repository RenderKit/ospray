## ======================================================================== ##
## Copyright 2018-2019 Intel Corporation                                    ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

set(OSPRAY_DEFAULT_COMPONENT apps)

# prefer libGL over libOpenGl for better compatibility with SWR
set(OpenGL_GL_PREFERENCE "LEGACY")
find_package(OpenGL 2 REQUIRED)

find_package(glfw3 QUIET)
if (TARGET glfw)
  get_property(glfw3_INCLUDE_DIRS TARGET glfw
    PROPERTY INTERFACE_INCLUDE_DIRECTORIES
  )
endif()

# build ospTutorial, for demonstration and API testing
add_executable(ospTutorial ospTutorial.c)
target_link_libraries(ospTutorial PRIVATE ospray)
# C++ version
add_executable(ospTutorialCpp ospTutorial.cpp)
target_link_libraries(ospTutorialCpp PRIVATE ospray)

ospray_create_application(ospTutorialSpheres
  ArcballCamera.cpp
  GLFWOSPRayWindow.cpp
  ospTutorialSpheres.cpp
  imgui/imgui_impl_glfw_gl3.cpp
)

target_link_libraries(ospTutorialSpheres
PRIVATE
  ospray_testing
  ospray_imgui
  glfw
  ${OPENGL_LIBRARIES}
)

ospray_create_application(ospTutorialBouncingSpheres
  ArcballCamera.cpp
  GLFWOSPRayWindow.cpp
  ospTutorialBouncingSpheres.cpp
  imgui/imgui_impl_glfw_gl3.cpp
)

target_link_libraries(ospTutorialBouncingSpheres
PRIVATE
  ospray
  ospray_common
  ospray_imgui
  glfw
  ${OPENGL_LIBRARIES}
)

ospray_create_application(ospTutorialSubdivision
  ArcballCamera.cpp
  GLFWOSPRayWindow.cpp
  ospTutorialSubdivision.cpp
  imgui/imgui_impl_glfw_gl3.cpp
)

target_link_libraries(ospTutorialSubdivision
PRIVATE
  ospray_testing
  ospray_imgui
  glfw
  ${OPENGL_LIBRARIES}
)

ospray_create_application(ospTutorialStructuredVolume
  ArcballCamera.cpp
  GLFWOSPRayWindow.cpp
  ospTutorialStructuredVolume.cpp
  imgui/imgui_impl_glfw_gl3.cpp
)

target_link_libraries(ospTutorialStructuredVolume
PRIVATE
  ospray_testing
  ospray_imgui
  glfw
  ${OPENGL_LIBRARIES}
)

ospray_create_application(ospTutorialUnstructuredVolume
  ArcballCamera.cpp
  GLFWOSPRayWindow.cpp
  ospTutorialUnstructuredVolume.cpp
  imgui/imgui_impl_glfw_gl3.cpp
)

target_link_libraries(ospTutorialUnstructuredVolume
PRIVATE
  ospray_testing
  ospray_imgui
  glfw
  ${OPENGL_LIBRARIES}
)
