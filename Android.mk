LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    src/lib/CECProcessor.cpp \
    src/lib/LibCEC.cpp \
    src/lib/LibCECC.cpp \
    src/lib/CECClient.cpp \
    src/lib/adapter/AdapterFactory.cpp \
    src/lib/devices/CECAudioSystem.cpp \
    src/lib/devices/CECBusDevice.cpp \
    src/lib/devices/CECDeviceMap.cpp \
    src/lib/devices/CECPlaybackDevice.cpp \
    src/lib/devices/CECRecordingDevice.cpp \
    src/lib/devices/CECTuner.cpp \
    src/lib/devices/CECTV.cpp \
    src/lib/implementations/ANCommandHandler.cpp \
    src/lib/implementations/CECCommandHandler.cpp \
    src/lib/implementations/SLCommandHandler.cpp \
    src/lib/implementations/VLCommandHandler.cpp \
    src/lib/implementations/RLCommandHandler.cpp \
    src/lib/implementations/PHCommandHandler.cpp \
    src/lib/implementations/RHCommandHandler.cpp \
    src/lib/implementations/AQCommandHandler.cpp \
    src/lib/adapter/Tegra/TegraCECAdapterDetection.cpp \
    src/lib/adapter/Tegra/TegraCECAdapterCommunication.cpp

LOCAL_CFLAGS := -DHAVE_CONFIG_H
    
LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/src \
    $(LOCAL_PATH)/android \
    external/stlport/stlport \
    bionic \
    bionic/libstdc++/include

LOCAL_SHARED_LIBRARIES := \
    libstlport


LOCAL_MODULE := libcec
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    src/testclient/main.cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/src \
    $(LOCAL_PATH)/android \
    external/stlport/stlport \
    bionic \
    bionic/libstdc++/include

LOCAL_SHARED_LIBRARIES := \
    libcec \
    libstlport \
    libdl

LOCAL_MODULE := cec-client
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

include $(BUILD_EXECUTABLE)