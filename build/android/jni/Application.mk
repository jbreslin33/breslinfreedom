APP_PLATFORM := ${APP_PLATFORM}
APP_ABI := ${TARGET_ABI}
APP_STL := c++_shared
NDK_TOOLCHAIN_VERSION := ${COMPILER_VERSION}
APP_DEPRECATED_HEADERS := true
APP_MODULES := minetest
ifndef NDEBUG
APP_OPTIM := debug
endif

APP_CPPFLAGS += -fexceptions -std=c++11 -frtti