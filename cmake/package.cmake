## ======================================================================== ##
## Copyright 2009-2018 Intel Corporation                                    ##
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

INCLUDE(GNUInstallDirs)

SET(CMAKE_INSTALL_SCRIPTDIR scripts)
IF (OSPRAY_ZIP_MODE)
  # in tgz / zip let's have relative RPath
  SET(CMAKE_SKIP_INSTALL_RPATH OFF)
  IF (APPLE)
    SET(CMAKE_MACOSX_RPATH ON)
    SET(CMAKE_INSTALL_RPATH "@executable_path/" "@executable_path/../lib")
  ELSE()
    SET(CMAKE_INSTALL_RPATH "\$ORIGIN:\$ORIGIN/../lib")
    # on per target basis:
    #SET_TARGET_PROPERTIES(apps INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    #SET_TARGET_PROPERTIES(libs INSTALL_RPATH "$ORIGIN")
  ENDIF()
ELSE()
  SET(CMAKE_INSTALL_NAME_DIR ${CMAKE_INSTALL_FULL_LIBDIR})
  IF (APPLE)
    # use RPath on OSX
    SET(CMAKE_SKIP_INSTALL_RPATH OFF)
  ELSE()
    # we do not want any RPath for installed binaries
    SET(CMAKE_SKIP_INSTALL_RPATH ON)
  ENDIF()
  IF (NOT WIN32)
    # for RPMs install docu in versioned folder
    SET(CMAKE_INSTALL_DOCDIR ${CMAKE_INSTALL_DOCDIR}-${OSPRAY_VERSION})
    SET(CMAKE_INSTALL_SCRIPTDIR ${CMAKE_INSTALL_DATAROOTDIR}/OSPRay-${OSPRAY_VERSION}/scripts)
  ENDIF()
ENDIF()

##############################################################
# install headers
##############################################################

INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/ospray/include/ospray
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  COMPONENT devel
  FILES_MATCHING PATTERN "*.h"
)

##############################################################
# install documentation
##############################################################

INSTALL(FILES ${PROJECT_SOURCE_DIR}/LICENSE.txt DESTINATION ${CMAKE_INSTALL_DOCDIR} COMPONENT lib)
INSTALL(FILES ${PROJECT_SOURCE_DIR}/CHANGELOG.md DESTINATION ${CMAKE_INSTALL_DOCDIR} COMPONENT lib)
INSTALL(FILES ${PROJECT_SOURCE_DIR}/README.md DESTINATION ${CMAKE_INSTALL_DOCDIR} COMPONENT lib)
INSTALL(FILES ${PROJECT_SOURCE_DIR}/readme.pdf DESTINATION ${CMAKE_INSTALL_DOCDIR} COMPONENT lib OPTIONAL)

##############################################################
# install bench script
##############################################################

INSTALL(PROGRAMS ${PROJECT_SOURCE_DIR}/scripts/bench/run_benchmark.py DESTINATION ${CMAKE_INSTALL_SCRIPTDIR} COMPONENT apps)

##############################################################
# CPack specific stuff
##############################################################

SET(CPACK_PACKAGE_NAME "OSPRay")
SET(CPACK_PACKAGE_FILE_NAME "ospray-${OSPRAY_VERSION}")
#SET(CPACK_PACKAGE_ICON ${PROJECT_SOURCE_DIR}/ospray-doc/images/icon.png)
#SET(CPACK_PACKAGE_RELOCATABLE TRUE)
SET(CPACK_STRIP_FILES TRUE) # do not disable, stripping symbols is important for security reasons
SET(CMAKE_STRIP "${PROJECT_SOURCE_DIR}/scripts/strip.sh") # needs this to properly strip under MacOSX

SET(CPACK_PACKAGE_VERSION_MAJOR ${OSPRAY_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${OSPRAY_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${OSPRAY_VERSION_PATCH})
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OSPRay: A Ray Tracing Based Rendering Engine for High-Fidelity Visualization")
SET(CPACK_PACKAGE_VENDOR "Intel Corporation")
SET(CPACK_PACKAGE_CONTACT ospray@googlegroups.com)

SET(CPACK_COMPONENT_LIB_DISPLAY_NAME "Library")
SET(CPACK_COMPONENT_LIB_DESCRIPTION "The OSPRay library including documentation.")

SET(CPACK_COMPONENT_DEVEL_DISPLAY_NAME "Development")
SET(CPACK_COMPONENT_DEVEL_DESCRIPTION "Header files for C and C++ required to develop applications with OSPRay.")

SET(CPACK_COMPONENT_APPS_DISPLAY_NAME "Applications")
SET(CPACK_COMPONENT_APPS_DESCRIPTION "Example and viewer applications and tutorials demonstrating how to use OSPRay.")

SET(CPACK_COMPONENT_MPI_DISPLAY_NAME "MPI Module")
SET(CPACK_COMPONENT_MPI_DESCRIPTION "OSPRay module for MPI-based distributed rendering.")

SET(CPACK_COMPONENT_REDIST_DISPLAY_NAME "Redistributables")
SET(CPACK_COMPONENT_REDIST_DESCRIPTION "Dependencies of OSPRay (such as Embree, TBB, imgui) that may or may not be already installed on your system.")

SET(CPACK_COMPONENT_TEST_DISPLAY_NAME "Test Suite")
SET(CPACK_COMPONENT_TEST_DESCRIPTION "Tools for testing the correctness of various aspects of OSPRay.")

# dependencies between components
SET(CPACK_COMPONENT_DEVEL_DEPENDS lib)
SET(CPACK_COMPONENT_APPS_DEPENDS lib)
SET(CPACK_COMPONENT_MPI_DEPENDS lib)
SET(CPACK_COMPONENT_LIB_REQUIRED ON) # always install the libs
SET(CPACK_COMPONENT_TEST_DEPENDS lib)

# point to readme and license files
SET(CPACK_RESOURCE_FILE_README ${PROJECT_SOURCE_DIR}/README.md)
SET(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/LICENSE.txt)

IF (OSPRAY_ZIP_MODE)
  SET(CPACK_MONOLITHIC_INSTALL ON)
ENDIF()


IF (WIN32) # Windows specific settings

  IF (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    MESSAGE(FATAL_ERROR "Only 64bit architecture supported.")
  ENDIF()

  IF (OSPRAY_ZIP_MODE)
    SET(CPACK_GENERATOR ZIP)
    SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}.windows")
  ELSE()
    SET(CPACK_GENERATOR WIX)
    SET(CPACK_WIX_ROOT_FEATURE_DESCRIPTION "OSPRay is an open source, scalable, and portable ray tracing engine for high-performance, high-fidelity visualization.")
    SET(CPACK_WIX_PROPERTY_ARPURLINFOABOUT http://www.ospray.org/)
    SET(CPACK_PACKAGE_NAME "OSPRay v${OSPRAY_VERSION}")
    SET(CPACK_COMPONENTS_ALL lib devel apps mpi redist)
    SET(CPACK_PACKAGE_INSTALL_DIRECTORY "Intel\\\\OSPRay v${OSPRAY_VERSION_MAJOR}")
    MATH(EXPR OSPRAY_VERSION_NUMBER "10000*${OSPRAY_VERSION_MAJOR} + 100*${OSPRAY_VERSION_MINOR} + ${OSPRAY_VERSION_PATCH}")
    SET(CPACK_WIX_PRODUCT_GUID "9D64D525-2603-4E8C-9108-845A146${OSPRAY_VERSION_NUMBER}")
    SET(CPACK_WIX_UPGRADE_GUID "9D64D525-2603-4E8C-9108-845A146${OSPRAY_VERSION_MAJOR}0000") # upgrade as long as major version is the same
    SET(CPACK_WIX_CMAKE_PACKAGE_REGISTRY TRUE)
  ENDIF()


ELSEIF(APPLE) # MacOSX specific settings

  CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/README.md ${PROJECT_BINARY_DIR}/ReadMe.txt COPYONLY)
  SET(CPACK_RESOURCE_FILE_README ${PROJECT_BINARY_DIR}/ReadMe.txt)

  IF (OSPRAY_ZIP_MODE)
    SET(CPACK_GENERATOR TGZ)
    SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}.x86_64.macosx")
  ELSE()
    SET(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
    SET(CPACK_GENERATOR PackageMaker)
    SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}.x86_64")
    #SET(CPACK_COMPONENTS_ALL lib devel apps)
    SET(CPACK_MONOLITHIC_INSTALL ON)
    SET(CPACK_PACKAGE_NAME ospray-${OSPRAY_VERSION})
    SET(CPACK_PACKAGE_VENDOR "intel") # creates short name com.intel.ospray.xxx in pkgutil
    SET(CPACK_OSX_PACKAGE_VERSION 10.7)
  ENDIF()


ELSE() # Linux specific settings

  IF (OSPRAY_ZIP_MODE)
    SET(CPACK_GENERATOR TGZ)
    SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}.x86_64.linux")
  ELSE()
    SET(CPACK_GENERATOR RPM)
    SET(CPACK_COMPONENTS_ALL lib devel apps mpi)
    IF (OSPRAY_ENABLE_TESTING)
      LIST(APPEND CPACK_COMPONENTS_ALL test)
    ENDIF()
    SET(CPACK_RPM_COMPONENT_INSTALL ON)

    # dependencies
    SET(OSPLIB_REQS "embree3-lib >= ${EMBREE_VERSION_REQUIRED}")
    IF (CMAKE_VERSION VERSION_LESS "3.4.0")
      OSPRAY_WARN_ONCE(RPM_PACKAGING "You need at least v3.4.0 of CMake for generating RPMs")
      SET(CPACK_RPM_PACKAGE_REQUIRES ${OSPLIB_REQS})
    ELSE()
      INCLUDE(ispc) # for ISPC_VERSION_REQUIRED
      # needs to use COMPONENT names in original capitalization (i.e. lowercase)
      SET(CPACK_RPM_lib_PACKAGE_REQUIRES ${OSPLIB_REQS})
      SET(CPACK_RPM_apps_PACKAGE_REQUIRES "ospray-lib >= ${OSPRAY_VERSION}")
      SET(CPACK_RPM_devel_PACKAGE_REQUIRES "ospray-lib = ${OSPRAY_VERSION}, ispc >= ${ISPC_VERSION_REQUIRED}")
      SET(CPACK_RPM_mpi_PACKAGE_REQUIRES "ospray-lib = ${OSPRAY_VERSION}")
      SET(CPACK_RPM_test_PACKAGE_REQUIRES "ospray-lib = ${OSPRAY_VERSION}")
    ENDIF()

    SET(CPACK_RPM_PACKAGE_RELEASE 1)
    SET(CPACK_RPM_FILE_NAME RPM-DEFAULT)
    SET(CPACK_RPM_devel_PACKAGE_ARCHITECTURE noarch)
    SET(CPACK_RPM_PACKAGE_LICENSE "ASL 2.0") # Apache Software License, Version 2.0
    SET(CPACK_RPM_PACKAGE_GROUP "Development/Libraries")

    SET(CPACK_RPM_CHANGELOG_FILE ${CMAKE_BINARY_DIR}/rpm_changelog.txt) # ChangeLog of the RPM
    IF (CMAKE_VERSION VERSION_LESS "3.7.0")
      EXECUTE_PROCESS(COMMAND date "+%a %b %d %Y" OUTPUT_VARIABLE CHANGELOG_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)
    ELSE()
      STRING(TIMESTAMP CHANGELOG_DATE "%a %b %d %Y")
    ENDIF()
    SET(RPM_CHANGELOG "* ${CHANGELOG_DATE} Johannes Günther <johannes.guenther@intel.com> - ${OSPRAY_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}\n- First package")
    FILE(WRITE ${CPACK_RPM_CHANGELOG_FILE} ${RPM_CHANGELOG})

    SET(CPACK_RPM_PACKAGE_URL http://www.ospray.org/)
    SET(CPACK_RPM_DEFAULT_DIR_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

    # post install and uninstall scripts
    SET(SCRIPT_FILE_LDCONFIG ${CMAKE_BINARY_DIR}/rpm_ldconfig.sh)
    FILE(WRITE ${SCRIPT_FILE_LDCONFIG} "/sbin/ldconfig")
    SET(CPACK_RPM_lib_POST_INSTALL_SCRIPT_FILE ${SCRIPT_FILE_LDCONFIG})
    SET(CPACK_RPM_lib_POST_UNINSTALL_SCRIPT_FILE ${SCRIPT_FILE_LDCONFIG})
    SET(CPACK_RPM_mpi_POST_INSTALL_SCRIPT_FILE ${SCRIPT_FILE_LDCONFIG})
    SET(CPACK_RPM_mpi_POST_UNINSTALL_SCRIPT_FILE ${SCRIPT_FILE_LDCONFIG})
    SET(CPACK_RPM_apps_POST_INSTALL_SCRIPT_FILE ${SCRIPT_FILE_LDCONFIG})
    SET(CPACK_RPM_apps_POST_UNINSTALL_SCRIPT_FILE ${SCRIPT_FILE_LDCONFIG})
  ENDIF()
  
ENDIF()
