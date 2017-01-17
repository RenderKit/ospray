
# lits of components that have already been built. we use this for
# dependencies tracking: any part of ospray that 'depends' on a given
# component can simply call 'OSPRAY_BUILD_COMPONENT(...)' without
# having to worry about whether this component is already built - if
# it isn't, then it is going to be built by calling this macro; while
# if it already is built, we don't have to worry about building it
# twice.

# the macro any part of ospray can use to request ospray to
# include/build a specific component
#MACRO(OSPRAY_BUILD_COMPONENT comp)
#  IF (";${OSPRAY_LIST_OF_ALREADY_BUILT_COMPONENTS};" MATCHES ";${comp};")
#    # component already built; nothing to do!
#  ELSE()
#    
#    # TODO: check if the directory exists, and if not, check it out and/or warn user about it missing
#    SET(INCLUDED_AS_AN_OSPRAY_COMPONENT ON)
#    SET(OSPRAY_LIST_OF_ALREADY_BUILT_COMPONENTS ${OSPRAY_LIST_OF_ALREADY_BUILT_COMPONENTS} ${comp}
#      CACHE INTERNAL "" FORCE)
#    ADD_SUBDIRECTORY(${OSPRAY_COMPONENTS_ROOT}/${comp}
#      ${CMAKE_BINARY_DIR}/built_components/${comp}
#      EXCLUDE_FROM_ALL)
#  ENDIF()
#ENDMACRO()
