# ##################################################################
# target that generates API documentation via DoxyGen
# ##################################################################
FIND_PACKAGE(Doxygen)
IF(DOXYGEN_FOUND)
	CONFIGURE_FILE(
		${CMAKE_CURRENT_SOURCE_DIR}/doc/dox/doxyconfig.in
		${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
	ADD_CUSTOM_TARGET(doc
		${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating Doxygen documentation" VERBATIM
		)
ENDIF(DOXYGEN_FOUND)

