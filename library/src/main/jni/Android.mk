LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := loader

LOCAL_C_INCLUDES :=  $(LOCAL_PATH)/.
LOCAL_SRC_FILES := loader/loader.c
LOCAL_CFLAGS :=  -fPIC -ffreestanding -mregparm=3
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
    LOCAL_LDFLAGS := -static -nostdlib -Wl,-Ttext=0x10000000,-z,noexecstack
else ifeq ($(TARGET_ARCH_ABI), arm64-v8a)
    LOCAL_LDFLAGS := -static -nostdlib -Wl,-Ttext=0x2000000000,-z,noexecstack
else
endif
include $(BUILD_EXECUTABLE)