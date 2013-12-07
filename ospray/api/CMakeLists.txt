CONFIGURE_OSPRAY()

ADD_LIBRARY(OSPRay
	api.cpp
	device.cpp
	localdevice.cpp
	)
TARGET_LINK_LIBRARIES(OSPRay OSPcore)





