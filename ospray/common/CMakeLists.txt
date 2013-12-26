CONFIGURE_OSPRAY()
ADD_LIBRARY(osp_common${OSPRAY_LIB_SUFFIX} SHARED
	ospcommon.cpp
	managed.cpp
	data.cpp
	)
# link core to embree. EMBREE_LIB points to either libembree.so or
# libembree_xeonphi.so, depending on what arch we're building on.
TARGET_LINK_LIBRARIES(osp_common${OSPRAY_LIB_SUFFIX} ${EMBREE_LIB})
