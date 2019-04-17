LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := mic_array_test.c
LOCAL_MODULE := mic_array_test_yidong
LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	liblog libhardware


#LOCAL_SHARED_LIBRARIES += \
#	mic_array


include $(BUILD_EXECUTABLE)

