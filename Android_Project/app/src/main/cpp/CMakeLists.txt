cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall")

add_library(native-audio-jni SHARED
            native-audio-jni.c)

# Include libraries needed for native-audio-jni lib
target_link_libraries(native-audio-jni
                      android
                      log
                      OpenSLES)

target_include_directories(native-audio-jni PRIVATE
        ${CMAKE_SOURCE_DIR}/../../../../fftw3/include)
target_link_libraries (native-audio-jni
        ${CMAKE_SOURCE_DIR}/../../../../fftw3/lib/${ANDROID_ABI}/libfftw3.a)
