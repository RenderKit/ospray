SET (ISPC_INCLUDE_DIR "")
MACRO (INCLUDE_DIRECTORIES_ISPC)
  SET (ISPC_INCLUDE_DIR ${ISPC_INCLUDE_DIR} ${ARGN})
ENDMACRO ()

SET(ISPC_PLUS OFF CACHE BOOL
	"Use ISPC+?")
MARK_AS_ADVANCED(ISPC_PLUS)

SET(ISPC_ADDRESSING 32 CACHE INT
	"32vs64 bit addressing in ispc")
MARK_AS_ADVANCED(ISPC_ADDRESSING)

IF(ISPC_PLUS)
	INCLUDE_DIRECTORIES(/home/wald/Projects/ISPC++/)
ENDIF()

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
MACRO (ispc_compile)
	IF (CMAKE_OSX_ARCHITECTURES STREQUAL "i386")
		SET(ISPC_ARCHITECTURE "x86")
	ELSE()
		SET(ISPC_ARCHITECTURE "x86-64")
	ENDIF()
	
	IF(ISPC_INCLUDE_DIR)
		STRING(REGEX REPLACE ";" ";-I;" ISPC_INCLUDE_DIR_PARMS "${ISPC_INCLUDE_DIR}")
		SET(ISPC_INCLUDE_DIR_PARMS "-I" ${ISPC_INCLUDE_DIR_PARMS}) 
	ENDIF()

	IF (THIS_IS_MIC)
		SET(CMAKE_ISPC_FLAGS --opt=force-aligned-memory --target generic-16 --emit-c++ --c++-include-file=${PROJECT_SOURCE_DIR}/ospray/common/ispc_knc_backend.h)
#${ISPC_DIR}/examples/intrinsics/knc.h)
		SET(ISPC_TARGET_EXT ".dev.cpp")
	ELSEIF (ISPC_PLUS)
		SET(ISPC_TARGET_EXT ".dev.cpp")
	ELSE()
		SET(CMAKE_ISPC_FLAGS --target ${OSPRAY_ISPC_TARGET} --addressing=${ISPC_ADDRESSING})
		SET(ISPC_TARGET_EXT ".dev.o")
	ENDIF()

	SET(ISPC_OBJECTS "")
	FOREACH(src ${ARGN})
		GET_FILENAME_COMPONENT(fname ${src} NAME_WE)
		GET_FILENAME_COMPONENT(dir ${src} PATH)

		SET(ISPC_TARGET_DIR ${CMAKE_CURRENT_BINARY_DIR})
		IF("${dir}" STREQUAL "")
			SET(outdir ${ISPC_TARGET_DIR})
		ELSE("${dir}" STREQUAL "")
			SET(outdir ${ISPC_TARGET_DIR}/${dir})
		ENDIF("${dir}" STREQUAL "")
		SET(outdirh ${ISPC_TARGET_DIR})

		SET(deps "")
		IF (EXISTS ${outdir}/${fname}.dev.idep)
			FILE(READ ${outdir}/${fname}.dev.idep contents)
			STRING(REGEX REPLACE " " ";"     contents "${contents}")
			STRING(REGEX REPLACE ";" "\\\\;" contents "${contents}")
			STRING(REGEX REPLACE "\n" ";"    contents "${contents}")
			FOREACH(dep ${contents})
				IF (EXISTS ${dep})
					SET(deps ${deps} ${dep})
				ENDIF (EXISTS ${dep})
			ENDFOREACH(dep ${contents})
		ENDIF ()

		SET(results "${outdir}/${fname}${ISPC_TARGET_EXT}")

		# if we have multiple targets add additional object files
		IF (__XEON__)
			IF (${targets} MATCHES ".*,.*")
				FOREACH(target ${target_list})
					SET(results ${results} "${outdir}/${fname}${ISPC_TARGET_EXT}")
				ENDFOREACH()
			ENDIF()
		ENDIF()
		IF (ISPC_PLUS)
			ADD_CUSTOM_COMMAND(
				OUTPUT ${results} ${outdirh}/${fname}_ispc.h
				COMMAND mkdir -p ${outdir} \; /home/wald/Projects/ISPC++/bin/ispc++
				-I ${CMAKE_CURRENT_SOURCE_DIR} 
				-I /home/wald/Projects/ISPC++/include
			${ISPC_INCLUDE_DIR_PARMS}
			--target avx
			${CMAKE_ISPC_FLAGS}
			-h ${outdirh}/${fname}_ispc.h
			-MMM  ${outdir}/${fname}.dev.idep 
			-o ${outdir}/${fname}${ISPC_TARGET_EXT}
			${CMAKE_CURRENT_SOURCE_DIR}/${src} 
			\;
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${src}
			${deps})
		ELSE()
		ADD_CUSTOM_COMMAND(
			OUTPUT ${results} ${outdirh}/${fname}_ispc.h
			COMMAND mkdir -p ${outdir} \; ispc
			-I ${CMAKE_CURRENT_SOURCE_DIR} 
			${ISPC_INCLUDE_DIR_PARMS}
			--arch=${ISPC_ARCHITECTURE}
			--pic
			-O3
			--woff
			${CMAKE_ISPC_FLAGS}
			--opt=fast-math
			-h ${outdirh}/${fname}_ispc.h
			-MMM  ${outdir}/${fname}.dev.idep 
			-o ${outdir}/${fname}${ISPC_TARGET_EXT}
			${CMAKE_CURRENT_SOURCE_DIR}/${src} 
			\;
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${src}
			${deps})
		ENDIF()
		SET(ISPC_OBJECTS ${ISPC_OBJECTS} ${results})

	ENDFOREACH()

ENDMACRO()


MACRO (add_ispc_executable name)
	SET(ISPC_SOURCES "")
	SET(OTHER_SOURCES "")
	FOREACH(src ${ARGN})
		message("src $src")
		GET_FILENAME_COMPONENT(ext ${src} EXT)
		IF (ext STREQUAL ".ispc")
			SET(ISPC_SOURCES ${ISPC_SOURCES} ${src})
		ELSE ()
			SET(OTHER_SOURCES ${OTHER_SOURCES} ${src})
		ENDIF ()
	ENDFOREACH()
	ISPC_COMPILE(${ISPC_SOURCES})
	ADD_EXECUTABLE(${name} ${ISPC_OBJECTS} ${OTHER_SOURCES})
ENDMACRO()

MACRO (OSPRAY_ADD_LIBRARY name type)
	SET(ISPC_SOURCES "")
	SET(OTHER_SOURCES "")
	FOREACH(src ${ARGN})
		GET_FILENAME_COMPONENT(ext ${src} EXT)
		IF (ext STREQUAL ".ispc")
			SET(ISPC_SOURCES ${ISPC_SOURCES} ${src})
		ELSE ()
			SET(OTHER_SOURCES ${OTHER_SOURCES} ${src})
		ENDIF ()
	ENDFOREACH()
	ISPC_COMPILE(${ISPC_SOURCES})
	ADD_LIBRARY(${name} ${type} ${ISPC_OBJECTS} ${OTHER_SOURCES})
ENDMACRO()

