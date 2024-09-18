
# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.6.0)

# Declares and names the project.

project("library")

set(CMAKE_ANDROID_ARCH_ABI "armeabi-v7a")
set(CMAKE_ANDROID_ARCH_ABI "arm64-v8a")
# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

file(GLOB SOURCE_FILES
        cmn/cmn_vlarray.c
        cmn/cmn_pthread_cond.c
        cmn/cmn_back_call_stack.c
        cmn/cmn_proc.c
        cmn/cmn_utils.c
        ptrace/wait.c
        ptrace/ptrace.c
        syscall/sysnum.c
        syscall/seccomp.c
        syscall/syscall.c
        syscall/enter.c
        syscall/exit.c
        syscall/chain.c
        execve/elf.c
        execve/enter.c
        execve/exit.c
        execve/auxv.c
        execve/shebang.c
        #            execve/aoxp.c
        path/path.c
        path/temp.c
        tracee/mem.c
        tracee/tracee.c
        tracee/reg.c
        tracee/event.c
        library.c
        note.c)





include_directories(.)
include_directories(./tracee)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.



add_library( # Sets the name of the library.
        abyss
        SHARED
        # Provides a relative path to your source file(s).
        ${SOURCE_FILES}
        )

target_link_libraries( # Specifies the target library.
        abyss
        # Links the target library to the log library
        # included in the NDK.
        ${log-lib} )

#----结束生成lirary 库

add_executable(
        testsvc
        test/main.c
        ${SOURCE_FILES}
)

target_link_libraries( # Specifies the target library.
        testsvc

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib} )

#add_executable(
#        crashdemo
#        test/crashdemo.c
#)
#
#add_executable(
#        test1
#        test/test.c
#)
#
#add_executable(
#        test2
#        test/test2.c
#        cmn/cmn_pthread_cond.c
#)
#


add_executable(
        multithreaddemo
        test/multithreaddemo.c
        ${SOURCE_FILES}
)
target_link_libraries( # Specifies the target library.
        multithreaddemo

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib} )

add_executable(
        myecho
        test/myecho.c
)

add_executable(
        fopen_demo
        test/fopen_demo.c
)

add_executable(
        execvedemo
        test/execvedemo.c
        ${SOURCE_FILES}
)
target_link_libraries( # Specifies the target library.
        execvedemo
        # Links the target library to the log library
        # included in the NDK.
        ${log-lib} )


add_executable(
        ptraceemudemo
        test/ptraceemudemo.c
        ${SOURCE_FILES}
)
target_link_libraries( # Specifies the target library.
        ptraceemudemo
        # Links the target library to the log library
        # included in the NDK.
        ${log-lib} )

add_executable(
        ptracedemo
        test/ptrace/ptracedemo.cpp
        test/ptrace/inject.cpp
)
target_link_libraries( # Specifies the target library.
        ptracedemo
        # Links the target library to the log library
        # included in the NDK.
        ${log-lib} )