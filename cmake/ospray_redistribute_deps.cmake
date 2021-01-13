## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

macro(ospray_install_namelink NAME)
  get_filename_component(TARGET_NAME ${NAME} NAME) # strip path
  set(LIB_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
  # strip version and lib suffix
  if(APPLE)
    set(LIBREGEX "(.+)[.]([0-9]+)([.][0-9]+[.][0-9]+)${LIB_SUFFIX}")
  else()
    set(LIBREGEX "(.+)${LIB_SUFFIX}[.]([0-9]+)([.][0-9]+[.][0-9]+)")
  endif()
  string(REGEX REPLACE ${LIBREGEX} "\\1" BASE_LIB_NAME ${TARGET_NAME})
  if (CMAKE_MATCH_COUNT GREATER 2)
    if(APPLE)
      set(SYMLINK ${BASE_LIB_NAME}.${CMAKE_MATCH_2}${LIB_SUFFIX})
    else()
      set(SYMLINK ${BASE_LIB_NAME}${LIB_SUFFIX}.${CMAKE_MATCH_2})
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" -E
        create_symlink ${TARGET_NAME} ${CMAKE_CURRENT_BINARY_DIR}/${SYMLINK})
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${SYMLINK} ${ARGN}
        DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT redist)
    set(TARGET_NAME ${SYMLINK})
  endif()
   
  # also create a major version suffixed symlink
  if(APPLE)
    set(LIBREGEX "(.+)[.]([0-9]+)${LIB_SUFFIX}")
  else()
    set(LIBREGEX "(.+)${LIB_SUFFIX}[.]([0-9]+)")
  endif()
  string(REGEX REPLACE ${LIBREGEX} "\\1" BASE_LIB_NAME ${TARGET_NAME})
  if (CMAKE_MATCH_COUNT)
    set(SYMLINK ${CMAKE_CURRENT_BINARY_DIR}/${BASE_LIB_NAME}${LIB_SUFFIX})
    execute_process(COMMAND "${CMAKE_COMMAND}" -E
        create_symlink ${TARGET_NAME} ${SYMLINK})
    install(PROGRAMS ${SYMLINK} ${ARGN} DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT redist)
  endif()
endmacro()

macro(ospray_add_dependent_lib TARGET_NAME)
  if (TARGET ${TARGET_NAME})
    get_target_property(CONFIGURATIONS ${TARGET_NAME} IMPORTED_CONFIGURATIONS)
    # first/default configuration
    list(GET CONFIGURATIONS 0 CONFIGURATION)
    get_target_property(LIBRARY ${TARGET_NAME} IMPORTED_LOCATION_${CONFIGURATION})
    list(APPEND DEPENDENT_LIBS ${LIBRARY})
    ospray_install_namelink(${LIBRARY})
    # potentially Debug configuration
    list(FIND CONFIGURATIONS "DEBUG" FOUND)
    if(FOUND GREATER 0)
      get_target_property(LIBRARY ${TARGET_NAME} IMPORTED_LOCATION_DEBUG)
      list(APPEND DEPENDENT_LIBS_DEBUG ${LIBRARY})
      ospray_install_namelink(${LIBRARY} CONFIGURATIONS Debug)
    endif()
  else()
    message(STATUS "Skipping target '${TARGET_NAME}")
  endif()
endmacro()

ospray_add_dependent_lib(rkcommon::rkcommon)
if (RKCOMMON_TASKING_TBB)
  ospray_add_dependent_lib(TBB::tbb)
  ospray_add_dependent_lib(TBB::tbbmalloc)
endif()
ospray_add_dependent_lib(openvkl::openvkl)
ospray_add_dependent_lib(openvkl::openvkl_module_ispc_driver)
ospray_add_dependent_lib(openvkl::openvkl_module_ispc_driver_4)
ospray_add_dependent_lib(openvkl::openvkl_module_ispc_driver_8)
ospray_add_dependent_lib(openvkl::openvkl_module_ispc_driver_16)
if (OSPRAY_MODULE_DENOISER)
  ospray_add_dependent_lib(OpenImageDenoise)
endif()

if (WIN32)
  get_filename_component(EMBREE_LIB_DIR ${EMBREE_LIBRARY} PATH)
  set(EMBREE_DLL_HINTS
    ${EMBREE_LIB_DIR}
    ${EMBREE_LIB_DIR}/../bin
    ${embree_DIR}/../../../bin
    ${embree_DIR}/../bin
  )
  find_file(EMBREE_DLL embree3.dll HINTS ${EMBREE_DLL_HINTS})
  mark_as_advanced(EMBREE_DLL)
  list(APPEND DEPENDENT_LIBS ${EMBREE_DLL})
  set(INSTALL_DIR ${CMAKE_INSTALL_BINDIR})
else()
  # TODO use ospray_add_dependent_lib(embree) when v3.10 (with targets) is minimum
  list(APPEND DEPENDENT_LIBS ${EMBREE_LIBRARY})
  ospray_install_namelink(${EMBREE_LIBRARY})
  set(INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})
endif()

if (OSPRAY_SIGN_FILE)
  add_custom_target(sign_files
    COMMAND ${OSPRAY_SIGN_FILE} ${OSPRAY_SIGN_FILE_ARGS} ${DEPENDENT_LIBS} ${DEPENDENT_LIBS_DEBUG}
    COMMENT "Signing files"
    VERBATIM
  )
else()
  add_custom_target(sign_files COMMENT "Not signing files")
endif()

install(PROGRAMS ${DEPENDENT_LIBS} DESTINATION ${INSTALL_DIR} COMPONENT redist)
if (DEPENDENT_LIBS_DEBUG)
  install(PROGRAMS ${DEPENDENT_LIBS_DEBUG} CONFIGURATIONS Debug
          DESTINATION ${INSTALL_DIR} COMPONENT redist)
endif()

# Install MSVC runtime
if (WIN32)
  set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
  include(InstallRequiredSystemLibraries)
  list(FILTER CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS INCLUDE REGEX
      ".*msvcp[0-9]+\.dll|.*vcruntime[0-9]+\.dll|.*vcruntime[0-9]+_[0-9]+\.dll")
  install(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
      DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT redist)
endif()
