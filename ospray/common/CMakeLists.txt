
ADD_LIBRARY(OSPcommon 
	ospcommon.cpp
	managed.cpp
	data.cpp
	)
TARGET_LINK_LIBRARIES(OSPcommon embree)
