LOCAL_PATH := $(call /D:/ctc/Github/native-audio2/)

include $(CLEAR_VARS)
LOCAL_MODULE := fftw3
LOCAL_SRC_FILES := $(LOCAL_PATH)/fftw3/lib/$(TARGET_ARCH_ABI)/libfftw3.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/fftw3/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := native-audio-jni
LOCAL_SRC_FILES := $(LOCAL_PATH)/app/src/main/cpp/native-audio-jni.cpp
LOCAL_LDLIBS := -llog -lm
LOCAL_STATIC_LIBRARIES := fftw3
include $(BUILD_SHARED_LIBRARY)
