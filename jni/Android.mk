LOCAL_PATH := $(call my-dir)

# ---- 先编译 And64InlineHook 这个工具库 ----
include $(CLEAR_VARS)
LOCAL_MODULE            := And64InlineHook
LOCAL_SRC_FILES         := And64InlineHook/And64InlineHook.cpp
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/And64InlineHook
LOCAL_CPPFLAGS          := -std=c++17 -O2
include $(BUILD_STATIC_LIBRARY)

# ---- 再编译我们的模块 ----
include $(CLEAR_VARS)
LOCAL_MODULE           := widevine-spoof
LOCAL_SRC_FILES        := module.cpp
LOCAL_C_INCLUDES       := $(LOCAL_PATH)
LOCAL_STATIC_LIBRARIES := And64InlineHook
LOCAL_LDLIBS           := -llog -ldl
LOCAL_CPPFLAGS         := -std=c++17 -O2 -fvisibility=hidden -fexceptions -frtti
include $(BUILD_SHARED_LIBRARY)
