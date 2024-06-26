LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := recgraph
LOCAL_SRC_FILES := recgraph.c
LOCAL_LDLIBS    := -llog

include $(BUILD_SHARED_LIBRARY)
