## Copyright 2026 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# Installs an unpacked pre-built package into DST, keeping files another
# component installed there already: the packages bundle their dependencies,
# which the superbuild builds first, thus first writer wins. Files of the
# previous install are removed, thus version bumps still replace them.
#
# cmake -DSRC=<dir> -DDST=<dir> -DMANIFEST=<file> -P <this file>

cmake_minimum_required(VERSION 3.10)

foreach (ARG SRC DST MANIFEST)
  if (NOT ${ARG})
    message(FATAL_ERROR "package_install.cmake: ${ARG} not set")
  endif()
endforeach()

if (EXISTS "${MANIFEST}")
  file(STRINGS "${MANIFEST}" PREVIOUS)
  foreach (FILE IN LISTS PREVIOUS)
    file(REMOVE "${DST}/${FILE}")
  endforeach()
endif()

file(GLOB_RECURSE FILES RELATIVE "${SRC}" "${SRC}/*")
set(INSTALLED "")
foreach (FILE IN LISTS FILES)
  if (NOT EXISTS "${DST}/${FILE}")
    get_filename_component(DIR "${FILE}" DIRECTORY)
    file(COPY "${SRC}/${FILE}" DESTINATION "${DST}/${DIR}")
    list(APPEND INSTALLED "${FILE}")
  endif()
endforeach()

string(REPLACE ";" "\n" INSTALLED "${INSTALLED}")
file(WRITE "${MANIFEST}" "${INSTALLED}\n")
