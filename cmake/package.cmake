## ======================================================================== ##
## Copyright 2009-2017 Intel Corporation                                    ##
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
INSTALL(FILES ${CMAKE_BINARY_DIR}/readme.pdf DESTINATION ${CMAKE_INSTALL_DOCDIR} COMPONENT lib OPTIONAL)

LIST(APPEND CPACK_NSIS_MENU_LINKS "${CMAKE_INSTALL_DOCDIR}/LICENSE.txt" "LICENSE")
LIST(APPEND CPACK_NSIS_MENU_LINKS "${CMAKE_INSTALL_DOCDIR}/CHANGELOG.md" "CHANGELOG")
LIST(APPEND CPACK_NSIS_MENU_LINKS "${CMAKE_INSTALL_DOCDIR}/README.md" "README.md")
LIST(APPEND CPACK_NSIS_MENU_LINKS "${CMAKE_INSTALL_DOCDIR}/readme.pdf" "readme.pdf")

##############################################################
# CPack specific stuff
##############################################################

SET(CPACK_PACKAGE_NAME "OSPRay")
SET(CPACK_PACKAGE_FILE_NAME "ospray-${OSPRAY_VERSION}")
#SET(CPACK_PACKAGE_ICON ${PROJECT_SOURCE_DIR}/ospray-doc/images/icon.png)
#SET(CPACK_PACKAGE_RELOCATABLE TRUE)
#SET(CPACK_STRIP_FILES TRUE) # keep symbols, useful for bug reports

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
SET(CPACK_COMPONENT_APPS_DESCRIPTION "Viewer applications and tutorials demonstrating how to use OSPRay.")

SET(CPACK_COMPONENT_MPI_DISPLAY_NAME "MPI Module")
SET(CPACK_COMPONENT_MPI_DESCRIPTION "OSPRay module for MPI-based distributed rendering.")

SET(CPACK_COMPONENT_REDIST_DISPLAY_NAME "Redistributables")
SET(CPACK_COMPONENT_REDIST_DESCRIPTION "Dependencies of OSPRay (such as Embree, TBB, Qt, Glut) that may or may not be already installed on your system.")

# dependencies between components
SET(CPACK_COMPONENT_DEVEL_DEPENDS lib)
SET(CPACK_COMPONENT_APPS_DEPENDS lib)
SET(CPACK_COMPONENT_MPI_DEPENDS lib)
SET(CPACK_COMPONENT_LIB_REQUIRED ON) # always install the libs

# point to readme and license files
SET(CPACK_RESOURCE_FILE_README ${PROJECT_SOURCE_DIR}/README.md)
SET(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/LICENSE.txt)

IF (OSPRAY_ZIP_MODE)
  SET(CPACK_MONOLITHIC_INSTALL 1)
ENDIF()


IF(WIN32) # Windows specific settings

  IF (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    MESSAGE(FATAL_ERROR "Only 64bit architecture supported.")
  ENDIF()

  IF (OSPRAY_ZIP_MODE)
    SET(CPACK_GENERATOR ZIP)
    SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}.windows")
  ELSE() # NSIS specific settings
    SET(CPACK_GENERATOR NSIS)
    SET(CPACK_COMPONENTS_ALL lib devel apps mpi)
    SET(CPACK_NSIS_INSTALL_ROOT "\$PROGRAMFILES64\\\\Intel")
    SET(CPACK_PACKAGE_INSTALL_DIRECTORY "OSPRay v${OSPRAY_VERSION}")
    SET(CPACK_NSIS_DISPLAY_NAME "OSPRay v${OSPRAY_VERSION}")
    SET(CPACK_NSIS_PACKAGE_NAME "OSPRay v${OSPRAY_VERSION}")
    SET(CPACK_NSIS_URL_INFO_ABOUT http://www.ospray.org/)
    #SET(CPACK_NSIS_HELP_LINK http://www.ospray.org/getting_ospray.html)
    #SET(CPACK_NSIS_MUI_ICON ${PROJECT_SOURCE_DIR}/scripts/install_windows/icon32.ico)
    SET(CPACK_NSIS_CONTACT ${CPACK_PACKAGE_CONTACT})
    #SET(CPACK_NSIS_EXTRA_PREINSTALL_COMMANDS ${CPACK_NSIS_EXTRA_PREINSTALL_COMMANDS} "\n CreateDirectory \\\"$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\"")
    #SET(CPACK_NSIS_EXTRA_PREINSTALL_COMMANDS ${CPACK_NSIS_EXTRA_PREINSTALL_COMMANDS} "\n CreateDirectory \\\"$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\documentation\\\"")
    #SET(CPACK_NSIS_EXTRA_PREINSTALL_COMMANDS ${CPACK_NSIS_EXTRA_PREINSTALL_COMMANDS} "\n CreateDirectory \\\"$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\tutorials\\\" \n")
    #SET(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS  ${CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS} "\n ${UNINSTALL_LIST}\n RMDir \\\"$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\documentation\\\"")
    #SET(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS  ${CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS} "\n ${UNINSTALL_LIST}\n RMDir \\\"$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\tutorials\\\"")
    #SET(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS  ${CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS} "\n ${UNINSTALL_LIST}\n RMDir \\\"$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\"\n")
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
    SET(CPACK_MONOLITHIC_INSTALL 1)
    SET(CPACK_PACKAGE_NAME ospray-${OSPRAY_VERSION})
    SET(CPACK_PACKAGE_VENDOR "intel") # creates short name com.intel.ospray.xxx in pkgutil
    SET(CPACK_OSX_PACKAGE_VERSION 10.7)
  ENDIF()


ELSE() # Linux specific settings

  IF (OSPRAY_ZIP_MODE)
    SET(CPACK_GENERATOR TGZ)
    SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}.x86_64.linux")
  ELSE()
    SET(OSPLIB_REQS "embree-lib >= ${EMBREE_VERSION_REQUIRED}")
    OSPRAY_CONFIGURE_TASKING_SYSTEM()# NOTE(jda) - OSPRAY_TASKING_SYSTEM wasn't visible
    IF(${OSPRAY_TASKING_SYSTEM} STREQUAL "TBB")
      SET(OSPLIB_REQS "${OSPLIB_REQS}, tbb >= 3.0")
    ENDIF()
    IF(CMAKE_VERSION VERSION_LESS "3.4.0")
      OSPRAY_WARN_ONCE(RPM_PACKAGING "You need at least v3.4.0 of CMake for generating RPMs")
      SET(CPACK_RPM_PACKAGE_REQUIRES ${OSPLIB_REQS})
    ELSE()
      # needs to use COMPONENT names in original capitalization (i.e. lowercase)
      SET(CPACK_RPM_lib_PACKAGE_REQUIRES ${OSPLIB_REQS})
      SET(CPACK_RPM_apps_PACKAGE_REQUIRES "ospray-lib >= ${OSPRAY_VERSION}, qt = 4")
      SET(CPACK_RPM_devel_PACKAGE_REQUIRES "ospray-lib = ${OSPRAY_VERSION}")
      SET(CPACK_RPM_mpi_PACKAGE_REQUIRES "ospray-lib = ${OSPRAY_VERSION}")
      #SET(CPACK_RPM_PACKAGE_DEBUG ON)
    ENDIF()
    SET(CPACK_GENERATOR RPM)
    SET(CPACK_RPM_PACKAGE_RELEASE 1) # scripts/release_linux.sh assumes "1"
    SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}-${CPACK_RPM_PACKAGE_RELEASE}.x86_64")
    SET(CPACK_COMPONENTS_ALL lib devel apps mpi)
    SET(CPACK_RPM_COMPONENT_INSTALL ON)
    SET(CPACK_RPM_PACKAGE_LICENSE "ASL 2.0") # Apache Software License, Version 2.0
    SET(CPACK_RPM_PACKAGE_GROUP "Development/Libraries")
    #SET(CPACK_RPM_CHANGELOG_FILE "") # ChangeLog of the RPM; also CHANGELOG.md is not in the required format
    SET(CPACK_RPM_PACKAGE_ARCHITECTURE x86_64)
    SET(CPACK_RPM_PACKAGE_URL http://www.ospray.org/)
    SET(CPACK_RPM_DEFAULT_DIR_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

    # post install and uninstall scripts
    SET(CPACK_RPM_lib_POST_INSTALL_SCRIPT_FILE ${PROJECT_SOURCE_DIR}/scripts/rpm_ldconfig.sh)
    SET(CPACK_RPM_lib_POST_UNINSTALL_SCRIPT_FILE ${PROJECT_SOURCE_DIR}/scripts/rpm_ldconfig.sh)
  ENDIF()
  
ENDIF()
