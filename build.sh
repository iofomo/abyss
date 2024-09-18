#/bin/bash
set -e
#brew install cmake
#cd out/
#make clean
#ABI=arm64-v8a  #armeabi-v7a
#MINSDKVERSION=23
#NDK=/Users/mac/Library/Android/sdk/ndk/25.2.9519653
#
#CMAKE_ROOT=/Users/mac/Library/Android/sdk/cmake/3.6.0
#CMAKE_ROOT=/Users/mac/Library/Android/sdk/cmake/3.6.0

#使用android下的cmake，不要使用系统的cmake
#DCMAKE_LIBRARY_OUTPUT_DIRECTORY DCMAKE_RUNTIME_OUTPUT_DIRECTORY 输出目录
#DCMAKE_MAKE_PROGRAM make程序
#https://developer.android.com/ndk/guides/cmake?hl=zh-cn#command-line_1
#$CMAKE_ROOT/bin/cmake \
#    -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
#    -DANDROID_ABI=$ABI \
#    -DANDROID_PLATFORM=android-$MINSDKVERSION \
#    -DCMAKE_BUILD_TYPE=Debug \
#    -DCMAKE_ANDROID_NDK=$NDK \
#    -DCMAKE_ANDROID_ARCH_ABI=$ABI \
#    -DCMAKE_SYSTEM_NAME=Android \
#    -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=bin\
#    -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=bin\
#    -DCMAKE_MAKE_PROGRAM=$CMAKE_ROOT/bin/ninja \
#    -DCMAKE_SYSTEM_VERSION=$MINSDKVERSION  ../app/src/main/jni/
#
#make
#adb push bin/testsvc /data/local/tmp

#构建loader
#ndk 14
#cd app/src/main/jni
#ndk-build
#cd ..
#adb push ./libs/arm64-v8a/loader /data/local/tmp/loader_arm64
#adb push ./libs/armeabi-v7a/loader /data/local/tmp/loader_arm32
#adb push ./libs/arm64-v8a/loader /sdcard/loader_arm64
#adb push ./libs/armeabi-v7a/loader /sdcard/loader_arm32
#rm -rf ./libs
#rm -rf ./obj
#cd ../../../

#会同时编译arm32和arm64
./gradlew build
adb push ./app/build/intermediates/cmake/debug/obj/arm64-v8a/testsvc /data/local/tmp

#推送源码
adb shell rm -rf /data/local/tmp/source
adb push ./app/src/main/jni/ /data/local/tmp/source
#set substitute-path  /Users/mac/dev/code/InterceptSysCall/app/src/main/jni /data/local/tmp/source


#adb push ./app/build/intermediates/cmake/debug/obj/arm64-v8a/crashdemo /data/local/tmp
#adb push ./app/build/intermediates/cmake/debug/obj/arm64-v8a/test1 /data/local/tmp
#adb push ./app/build/intermediates/cmake/debug/obj/arm64-v8a/test2 /data/local/tmp
adb push ./app/build/intermediates/cmake/debug/obj/arm64-v8a/usedemo /data/local/tmp

adb push ./app/build/intermediates/cmake/debug/obj/arm64-v8a/myecho /data/local/tmp
adb push ./app/build/intermediates/cmake/debug/obj/arm64-v8a/execvedemo /data/local/tmp
adb push ./app/build/intermediates/cmake/debug/obj/arm64-v8a/execvedemo /sdcard/

#adb push ./app/build/intermediates/cmake/debug/obj/armeabi-v7a/myecho /data/local/tmp
#adb push ./app/build/intermediates/cmake/debug/obj/arm64-v8a/execvedemo /data/local/tmp

adb push ./app/build/intermediates/cmake/debug/obj/arm64-v8a/ptracedemo /data/local/tmp
adb push ./app/build/intermediates/cmake/debug/obj/arm64-v8a/ptraceemudemo /data/local/tmp
adb push ./app/build/intermediates/cmake/debug/obj/arm64-v8a/ptraceemudemo /sdcard/
#adb push ./app/build/intermediates/cmake/debug/obj/armeabi-v7a/ptraceemudemo /data/local/tmp

adb push ./app/build/intermediates/cmake/debug/obj/arm64-v8a/multithreaddemo /data/local/tmp

#adb shell kill -9  `adb shell ps -ef|grep testsvc$ | awk '{print $2}' | head -1`
#find . -name liblibrary_static.a
#./app/.cxx/Debug/5b471h49/armeabi-v7a/liblibrary_static.a
#./app/.cxx/Debug/5b471h49/arm64-v8a/liblibrary_static.a
# set substitute-path  /Users/mac/dev/code/InterceptSysCall/app/src/main/jni /data/local/tmp/source