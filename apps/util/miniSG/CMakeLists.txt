INCLUDE_DIRECTORIES(${OSPRAY_DIR}/ospray)


ADD_EXECUTABLE(its2obj
	its2obj.cpp
)

ADD_LIBRARY(ospray_minisg 
	miniSG.cpp
	importer.cpp
	importOBJ.cpp
	importSTL.cpp
	importMSG.cpp
	)
