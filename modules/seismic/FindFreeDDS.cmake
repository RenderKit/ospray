# - Try to find FreeDDS
# Once done, this will define
#
#  FreeDDS_FOUND - system has FreeDDS
#  FreeDDS_INCLUDE_DIRS - the FreeDDS include directories
#  FreeDDS_LIBRARIES - link these to use FreeDDS
#
# this file is modeled after http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

MACRO(SUBDIRLIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
        LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()


# Use pkg-config to get hints about paths
libfind_pkg_check_modules(FreeDDS_PKGCONF FreeDDS)

# Root directory for FreeDDS installation
find_path(FreeDDS_ROOT_DIR
  NAMES bin/dds-set-path
  PATHS $ENV{DDSROOT} ENV PATH
  PATH_SUFFIXES ..
)

# Include directory
find_path(FreeDDS_INCLUDE_DIR
  NAMES cdds.h
  PATHS ${FreeDDS_PKGCONF_INCLUDE_DIRS} ${FreeDDS_ROOT_DIR}/include
)

# Include directory for host includes
SUBDIRLIST(SUBDIRS ${FreeDDS_ROOT_DIR}/include)

find_path(FreeDDS_INCLUDE_DIR_HOST
  NAMES chost.h
  PATHS ${FreeDDS_PKGCONF_INCLUDE_DIRS} ${FreeDDS_ROOT_DIR}/include
  PATH_SUFFIXES ${SUBDIRS}
)

# Library directory
SUBDIRLIST(SUBDIRS ${FreeDDS_ROOT_DIR}/lib)

find_path(FreeDDS_LIBRARY_DIR
  NAMES libdds_r3.a
  PATHS ${FreeDDS_PKGCONF_LIBRARY_DIRS} ${FreeDDS_ROOT_DIR}/lib
  PATH_SUFFIXES ${SUBDIRS}
)

# And the libraries themselves
find_library(FreeDDS_LIBRARY
  NAMES dds_r3
  PATHS ${FreeDDS_LIBRARY_DIR}
)

find_library(FreeDDS_CHOST_LIBRARY
  NAMES chost
  PATHS ${FreeDDS_LIBRARY_DIR}
)

find_library(FreeDDS_GIO_LIBRARY
  NAMES gio
  PATHS ${FreeDDS_LIBRARY_DIR}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(FreeDDS_PROCESS_INCLUDES FreeDDS_INCLUDE_DIR FreeDDS_INCLUDE_DIR_HOST)
set(FreeDDS_PROCESS_LIBS FreeDDS_LIBRARY FreeDDS_CHOST_LIBRARY FreeDDS_GIO_LIBRARY)
libfind_process(FreeDDS)
