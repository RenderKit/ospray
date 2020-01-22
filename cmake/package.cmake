## Copyright 2009-2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set(CMAKE_INSTALL_SCRIPTDIR scripts)
if (OSPRAY_ZIP_MODE)
  # in tgz / zip let's have relative RPath
  set(CMAKE_SKIP_INSTALL_RPATH OFF)
  if (APPLE)
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_INSTALL_RPATH "@executable_path/" "@executable_path/../${CMAKE_INSTALL_LIBDIR}")
  else()
    set(CMAKE_INSTALL_RPATH "\$ORIGIN:\$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
    # on per target basis:
    #set_TARGET_PROPERTIES(apps INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    #set_TARGET_PROPERTIES(libs INSTALL_RPATH "$ORIGIN")
  endif()
else()
  set(CMAKE_INSTALL_NAME_DIR ${CMAKE_INSTALL_FULL_LIBDIR})
  if (APPLE)
    # use RPath on OSX
    set(CMAKE_SKIP_INSTALL_RPATH OFF)
  else()
    # we do not want any RPath for installed binaries
    set(CMAKE_SKIP_INSTALL_RPATH ON)
  endif()
  if (NOT WIN32)
    # for RPMs install docu in versioned folder
    set(CMAKE_INSTALL_DOCDIR ${CMAKE_INSTALL_DOCDIR}-${OSPRAY_VERSION})
    set(CMAKE_INSTALL_SCRIPTDIR ${CMAKE_INSTALL_DATAROOTDIR}/OSPRay-${OSPRAY_VERSION}/scripts)
  endif()
endif()

##############################################################
# install headers
##############################################################

install(DIRECTORY ${PROJECT_SOURCE_DIR}/ospray/include/ospray
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  COMPONENT devel
  FILES_MATCHING PATTERN "*.h"
)

##############################################################
# install documentation
##############################################################

install(FILES ${PROJECT_SOURCE_DIR}/LICENSE.txt DESTINATION ${CMAKE_INSTALL_DOCDIR} COMPONENT lib)
install(FILES ${PROJECT_SOURCE_DIR}/CHANGELOG.md DESTINATION ${CMAKE_INSTALL_DOCDIR} COMPONENT lib)
install(FILES ${PROJECT_SOURCE_DIR}/README.md DESTINATION ${CMAKE_INSTALL_DOCDIR} COMPONENT lib)
install(FILES ${PROJECT_SOURCE_DIR}/readme.pdf DESTINATION ${CMAKE_INSTALL_DOCDIR} COMPONENT lib OPTIONAL)

##############################################################
# CPack specific stuff
##############################################################

set(CPACK_PACKAGE_NAME "OSPRay")
set(CPACK_PACKAGE_FILE_NAME "ospray-${OSPRAY_VERSION}.x86_64")
#set(CPACK_PACKAGE_ICON ${PROJECT_SOURCE_DIR}/ospray-doc/images/icon.png)
#set(CPACK_PACKAGE_RELOCATABLE TRUE)
set(CPACK_STRIP_FILES TRUE) # do not disable, stripping symbols is important for security reasons
if (APPLE)
  # needs this to properly strip and sign under MacOSX
  set(CMAKE_STRIP "${PROJECT_SOURCE_DIR}/scripts/release/macosx_strip+sign.sh")
endif()

set(CPACK_PACKAGE_VERSION_MAJOR ${OSPRAY_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${OSPRAY_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${OSPRAY_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OSPRay: A Ray Tracing Based Rendering Engine for High-Fidelity Visualization")
set(CPACK_PACKAGE_VENDOR "Intel Corporation")
set(CPACK_PACKAGE_CONTACT ospray@googlegroups.com)

set(CPACK_COMPONENT_LIB_DISPLAY_NAME "Library")
set(CPACK_COMPONENT_LIB_DESCRIPTION "The OSPRay library including documentation.")

set(CPACK_COMPONENT_DEVEL_DISPLAY_NAME "Development")
set(CPACK_COMPONENT_DEVEL_DESCRIPTION "Header files for C and C++ required to develop applications with OSPRay.")

set(CPACK_COMPONENT_APPS_DISPLAY_NAME "Applications")
set(CPACK_COMPONENT_APPS_DESCRIPTION "Example and viewer applications and tutorials demonstrating how to use OSPRay.")

set(CPACK_COMPONENT_REDIST_DISPLAY_NAME "Redistributables")
set(CPACK_COMPONENT_REDIST_DESCRIPTION "Dependencies of OSPRay (such as Embree, TBB, imgui) that may or may not be already installed on your system.")

set(CPACK_COMPONENT_TEST_DISPLAY_NAME "Test Suite")
set(CPACK_COMPONENT_TEST_DESCRIPTION "Tools for testing the correctness of various aspects of OSPRay.")

# dependencies between components
set(CPACK_COMPONENT_DEVEL_DEPENDS lib)
set(CPACK_COMPONENT_APPS_DEPENDS lib)
set(CPACK_COMPONENT_LIB_REQUIRED ON) # always install the libs
set(CPACK_COMPONENT_TEST_DEPENDS lib)

# point to readme and license files
set(CPACK_RESOURCE_FILE_README ${PROJECT_SOURCE_DIR}/README.md)
set(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/LICENSE.txt)

if (OSPRAY_ZIP_MODE)
  set(CPACK_MONOLITHIC_INSTALL ON)
else()
  set(CPACK_COMPONENTS_ALL lib devel apps)
  if (OSPRAY_ENABLE_TESTING)
    list(APPEND CPACK_COMPONENTS_ALL test)
  endif()
endif()


if (WIN32) # Windows specific settings

  if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Only 64bit architecture supported.")
  endif()

  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}.windows")

  if (OSPRAY_ZIP_MODE)
    set(CPACK_GENERATOR ZIP)
  else()
    set(CPACK_GENERATOR WIX)
    set(CPACK_WIX_ROOT_FEATURE_DESCRIPTION "OSPRay is an open source, scalable, and portable ray tracing engine for high-performance, high-fidelity visualization.")
    set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT http://www.ospray.org/)
    set(CPACK_PACKAGE_NAME "OSPRay v${OSPRAY_VERSION}")
    list(APPEND CPACK_COMPONENTS_ALL redist)
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "Intel\\\\OSPRay v${OSPRAY_VERSION_MAJOR}")
    math(EXPR OSPRAY_VERSION_NUMBER "10000*${OSPRAY_VERSION_MAJOR} + 100*${OSPRAY_VERSION_MINOR} + ${OSPRAY_VERSION_PATCH}")
    set(CPACK_WIX_PRODUCT_GUID "9D64D525-2603-4E8C-9108-845A146${OSPRAY_VERSION_NUMBER}")
    set(CPACK_WIX_UPGRADE_GUID "9D64D525-2603-4E8C-9108-845A146${OSPRAY_VERSION_MAJOR}0000") # upgrade as long as major version is the same
    set(CPACK_WIX_CMAKE_PACKAGE_REGISTRY TRUE)
  endif()


elseif(APPLE) # MacOSX specific settings

  configure_file(${PROJECT_SOURCE_DIR}/README.md ${PROJECT_BINARY_DIR}/ReadMe.txt COPYONLY)
  set(CPACK_RESOURCE_FILE_README ${PROJECT_BINARY_DIR}/ReadMe.txt)
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}.macosx")

  if (OSPRAY_ZIP_MODE)
    set(CPACK_GENERATOR ZIP)
  else()
    set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
    set(CPACK_GENERATOR productbuild)
    set(CPACK_PACKAGE_NAME ospray-${OSPRAY_VERSION})
    set(CPACK_PACKAGE_VENDOR "intel") # creates short name com.intel.ospray.xxx in pkgutil
  endif()


else() # Linux specific settings

  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}.linux")

  if (OSPRAY_ZIP_MODE)
    set(CPACK_GENERATOR TGZ)
  else()
    set(CPACK_GENERATOR RPM)
    set(CPACK_RPM_COMPONENT_INSTALL ON)

    # dependencies
    set(OSPLIB_REQS "embree3-lib >= ${EMBREE_VERSION_REQUIRED}")
    if (CMAKE_VERSION VERSION_LESS "3.4.0")
      OSPRAY_WARN_ONCE(RPM_PACKAGING "You need at least v3.4.0 of CMake for generating RPMs")
      set(CPACK_RPM_PACKAGE_REQUIRES ${OSPLIB_REQS})
    else()
      include(ispc) # for ISPC_VERSION_REQUIRED
      # needs to use COMPONENT names in original capitalization (i.e. lowercase)
      set(CPACK_RPM_lib_PACKAGE_REQUIRES ${OSPLIB_REQS})
      set(CPACK_RPM_apps_PACKAGE_REQUIRES "ospray-lib >= ${OSPRAY_VERSION}")
      set(CPACK_RPM_devel_PACKAGE_REQUIRES "ospray-lib = ${OSPRAY_VERSION}, ispc >= ${ISPC_VERSION_REQUIRED}")
      set(CPACK_RPM_test_PACKAGE_REQUIRES "ospray-lib = ${OSPRAY_VERSION}")
    endif()

    set(CPACK_RPM_PACKAGE_RELEASE 1)
    set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
    set(CPACK_RPM_devel_PACKAGE_ARCHITECTURE noarch)
    set(CPACK_RPM_PACKAGE_LICENSE "ASL 2.0") # Apache Software License, Version 2.0
    set(CPACK_RPM_PACKAGE_GROUP "Development/Libraries")

    set(CPACK_RPM_CHANGELOG_FILE ${CMAKE_BINARY_DIR}/rpm_changelog.txt) # ChangeLog of the RPM
    if (CMAKE_VERSION VERSION_LESS "3.7.0")
      execute_process(COMMAND date "+%a %b %d %Y" OUTPUT_VARIABLE CHANGELOG_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)
    else()
      string(TIMESTAMP CHANGELOG_DATE "%a %b %d %Y")
    endif()
    set(RPM_CHANGELOG "* ${CHANGELOG_DATE} Johannes Günther <johannes.guenther@intel.com> - ${OSPRAY_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}\n- First package")
    file(WRITE ${CPACK_RPM_CHANGELOG_FILE} ${RPM_CHANGELOG})

    set(CPACK_RPM_PACKAGE_URL http://www.ospray.org/)
    set(CPACK_RPM_DEFAULT_DIR_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

    # post install and uninstall scripts
    set(SCRIPT_FILE_LDCONFIG ${CMAKE_BINARY_DIR}/rpm_ldconfig.sh)
    file(WRITE ${SCRIPT_FILE_LDCONFIG} "/sbin/ldconfig")
    set(CPACK_RPM_lib_POST_INSTALL_SCRIPT_FILE ${SCRIPT_FILE_LDCONFIG})
    set(CPACK_RPM_lib_POST_UNINSTALL_SCRIPT_FILE ${SCRIPT_FILE_LDCONFIG})
    set(CPACK_RPM_apps_POST_INSTALL_SCRIPT_FILE ${SCRIPT_FILE_LDCONFIG})
    set(CPACK_RPM_apps_POST_UNINSTALL_SCRIPT_FILE ${SCRIPT_FILE_LDCONFIG})
  endif()

endif()
