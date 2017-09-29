# additional target to run clang-format on all source files

set (EXCLUDE_DIRS
  ${CMAKE_SOURCE_DIR}/apps/bench/pico_bench
  ${CMAKE_SOURCE_DIR}/apps/common/sg/3rdParty
  ${CMAKE_SOURCE_DIR}/apps/exampleViewer/common/gl3w
  ${CMAKE_SOURCE_DIR}/apps/exampleViewer/common/glfw
  ${CMAKE_SOURCE_DIR}/apps/exampleViewer/common/imgui
  ${CMAKE_SOURCE_DIR}/build
  ${CMAKE_SOURCE_DIR}/components/testing
  ${CMAKE_SOURCE_DIR}/ospray-doc
)

# get all project files
file(GLOB_RECURSE ALL_SOURCE_FILES *.cpp *.h *.ih *.ispc)
foreach (SOURCE_FILE ${ALL_SOURCE_FILES})
  foreach (EXCLUDE_DIR ${EXCLUDE_DIRS})
    string(FIND ${SOURCE_FILE} ${EXCLUDE_DIR} EXCLUDE_DIR_FOUND)
    if (NOT ${EXCLUDE_DIR_FOUND} EQUAL -1)
      list(REMOVE_ITEM ALL_SOURCE_FILES ${SOURCE_FILE})
    endif()
  endforeach()
endforeach()

add_custom_target(
  format
  COMMAND clang-format
  -style=file
  -i
  ${ALL_SOURCE_FILES}
)