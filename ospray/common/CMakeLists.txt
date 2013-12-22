
ADD_LIBRARY(OSPcommon SHARED
	ospcommon.cpp
	managed.cpp
	data.cpp
	)
TARGET_LINK_LIBRARIES(OSPcommon embree)
