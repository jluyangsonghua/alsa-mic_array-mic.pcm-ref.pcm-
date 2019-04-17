LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE := mic_array.default

LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += hardware/libhardware

#MIC_CHANNEL := $(strip $(BOARD_ROKID_MIC_CHANNELS))
#LOCAL_SHARED_LIBRARIES := liblog libcutils
#LOCAL_CFLAGS := -DROKID_TARGET_DEVICE=\"$(TARGET_DEVICE)\"
#LOCAL_CFLAGS += -DMIC_CHANNEL=$(MIC_CHANNEL)

LOCAL_SRC_FILES := alsalib_mic_array.c
LOCAL_C_INCLUDES += mic_array_alsa_from_lujiangnan/alsa-lib/include
LOCAL_SHARED_LIBRARIES += libasound libcutils libutils
LOCAL_LDLIBS := -llog
#LOCAL_CFLAGS += -DBOARD_ROKID_MIC_USE_ALSA_LIB=$(BOARD_ROKID_MIC_USE_ALSA_LIB)
LOCAL_CFLAGS += \
		-fPIC -DPIC -D_POSIX_SOURCE \
		-DALSA_CONFIG_DIR=\"/system/usr/share/alsa\" \
		-DALSA_PLUGIN_DIR=\"/system/usr/lib/alsa-lib\" \
		-DALSA_DEVICE_DIRECTORY=\"/dev/snd/\" \
		-finline-limit=300 -finline-functions -fno-inline-functions-called-once

LOCAL_MODULE_TARGET_ARCH := arm

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under, $(LOCAL_PATH))

