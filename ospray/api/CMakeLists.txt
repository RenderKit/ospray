CONFIGURE_OSPRAY()

ADD_LIBRARY(osp_api${OSPRAY_LIB_SUFFIX} SHARED
	api.cpp
	device.cpp
	localdevice.cpp
	)
TARGET_LINK_LIBRARIES(osp_api${OSPRAY_LIB_SUFFIX} osp_core${OSPRAY_LIB_SUFFIX})





