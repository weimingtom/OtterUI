LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := otter-prebuilt
LOCAL_SRC_FILES := ../../../../API/lib/android/libOtter.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../../API/inc
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libpng-prebuilt
LOCAL_SRC_FILES := libpng/libpng.a
include $(PREBUILT_STATIC_LIBRARY)

#------------------------------------------------------------------------------
# MAIN LIB
#------------------------------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE := ottersample

LOCAL_CFLAGS := -DANDROID_NDK
LOCAL_CFLAGS += -DPLATFORM_ANDROID
LOCAL_CFLAGS += -DDISABLE_IMPORTGL
LOCAL_CFLAGS += -I../../src
				
LOCAL_SRC_FILES := main.cpp
LOCAL_SRC_FILES += ../../../src/FileSystems/SampleFileSystem.cpp
LOCAL_SRC_FILES += ../../../src/Renderers/SampleRenderer.cpp
LOCAL_SRC_FILES += ../../../src/Renderers/OGLESRenderer.cpp
LOCAL_SRC_FILES += ../../../src/SoundSystems/AndroidSoundSystem.cpp
LOCAL_SRC_FILES += ../../../src/Views/IntroHandler.cpp
LOCAL_SRC_FILES += ../../../src/Views/BasicControlsViewHandler.cpp
LOCAL_SRC_FILES += ../../../src/Views/LabelsViewHandler.cpp
LOCAL_SRC_FILES += ../../../src/Views/MaskViewHandler.cpp
LOCAL_SRC_FILES += ../../../src/Views/TablesViewHandler.cpp
LOCAL_SRC_FILES += ../../../src/Views/ViewHandler.cpp
LOCAL_SRC_FILES += ../../../src/Plugins/SamplePlugin.cpp
LOCAL_SRC_FILES += ../../../src/Plugins/Circle.cpp
LOCAL_SRC_FILES += ../../../src/SampleUI.cpp

LOCAL_LDLIBS := -lGLESv1_CM -ldl -llog -lz -lstdc++

LOCAL_STATIC_LIBRARIES := otter-prebuilt libpng-prebuilt

LOCAL_ARM_MODE := arm
include $(BUILD_SHARED_LIBRARY)
