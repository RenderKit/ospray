## Copyright 2009-2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

macro(ospray_install_namelink NAME TARGET_NAME)
  set(BASE_LIB_NAME lib${NAME})
  set(LIB_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
  execute_process(COMMAND "${CMAKE_COMMAND}" -E create_symlink
                  ${TARGET_NAME} ${CMAKE_CURRENT_BINARY_DIR}/${BASE_LIB_NAME}${LIB_SUFFIX})
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/lib${NAME}${LIB_SUFFIX}
          DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT redist)

  # If the shared lib we're copying is named with a specific version, also
  # create a major version suffixed symlink
  string(REGEX MATCH "([0-9]+)[.]([0-9]+)[.]([0-9]+)" VERSION_STRING ${TARGET_NAME})
  if (CMAKE_MATCH_0)
    if(APPLE)
      execute_process(COMMAND "${CMAKE_COMMAND}" -E create_symlink
                      ${TARGET_NAME}
                      ${CMAKE_CURRENT_BINARY_DIR}/${BASE_LIB_NAME}.${CMAKE_MATCH_1}${LIB_SUFFIX})
      install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${BASE_LIB_NAME}.${CMAKE_MATCH_1}${LIB_SUFFIX}
              DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT redist)
    else()
      execute_process(COMMAND "${CMAKE_COMMAND}" -E create_symlink
                      ${TARGET_NAME}
                      ${CMAKE_CURRENT_BINARY_DIR}/${BASE_LIB_NAME}${LIB_SUFFIX}.${CMAKE_MATCH_1})
      install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${BASE_LIB_NAME}${LIB_SUFFIX}.${CMAKE_MATCH_1}
              DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT redist)
    endif()
  endif()
endmacro()

if (OSPCOMMON_TASKING_TBB)
  if (WIN32)
    set(TBB_DLL_HINTS
      HINTS
      ${OSPCOMMON_TBB_ROOT}/../redist/${TBB_ARCH}_win/tbb/vc14
      ${OSPCOMMON_TBB_ROOT}/../redist/${TBB_ARCH}/tbb/vc14
      ${OSPCOMMON_TBB_ROOT}/bin/${TBB_ARCH}/vc14
      ${OSPCOMMON_TBB_ROOT}/bin
    )
    find_file(TBB_DLL tbb.dll ${TBB_DLL_HINTS})
    find_file(TBB_DLL_DEBUG tbb_debug.dll ${TBB_DLL_HINTS})
    find_file(TBB_DLL_MALLOC tbbmalloc.dll ${TBB_DLL_HINTS})
    find_file(TBB_DLL_MALLOC_DEBUG tbbmalloc_debug.dll ${TBB_DLL_HINTS})
    mark_as_advanced(TBB_DLL)
    mark_as_advanced(TBB_DLL_DEBUG)
    mark_as_advanced(TBB_DLL_MALLOC)
    mark_as_advanced(TBB_DLL_MALLOC_DEBUG)
    install(PROGRAMS ${TBB_DLL} ${TBB_DLL_MALLOC}
            DESTINATION ${CMAKE_INSTALL_BINDIR}
            CONFIGURATIONS Release RelWithDebInfo COMPONENT redist)
    install(PROGRAMS ${TBB_DLL_DEBUG} ${TBB_DLL_MALLOC_DEBUG}
            DESTINATION ${CMAKE_INSTALL_BINDIR}
            CONFIGURATIONS Debug COMPONENT redist)
  else()
    install(PROGRAMS ${TBB_LIBRARY} ${TBB_LIBRARY_MALLOC}
            DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT redist)
    if (NOT APPLE)
      get_filename_component(TBB_LIBNAME ${TBB_LIBRARY} NAME)
      get_filename_component(TBB_MALLOC_LIBNAME ${TBB_LIBRARY_MALLOC} NAME)
      ospray_install_namelink(tbb ${TBB_LIBNAME})
      ospray_install_namelink(tbbmalloc ${TBB_MALLOC_LIBNAME})
    endif()
  endif()
endif()

macro(ospray_add_dependent_lib TARGET_NAME)
  get_target_property(CONFIGURATIONS ${TARGET_NAME} IMPORTED_CONFIGURATIONS)
  list(GET CONFIGURATIONS 0 CONFIGURATION)
  get_target_property(LIBRARY ${TARGET_NAME} IMPORTED_LOCATION_${CONFIGURATION})
  list(APPEND DEPENDENT_LIBS ${LIBRARY})
endmacro()

ospray_add_dependent_lib(ospcommon::ospcommon)
ospray_add_dependent_lib(openvkl::openvkl)
ospray_add_dependent_lib(openvkl::openvkl_module_ispc_driver)
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
  install(PROGRAMS ${DEPENDENT_LIBS}
          DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT redist)
else()
  list(APPEND DEPENDENT_LIBS ${EMBREE_LIBRARY})
  install(PROGRAMS ${DEPENDENT_LIBS}
          DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT redist)
  if (NOT APPLE)
    get_filename_component(EMBREE_LIBNAME ${EMBREE_LIBRARY} NAME)
    ospray_install_namelink(embree ${EMBREE_LIBNAME})
  endif()
endif()
