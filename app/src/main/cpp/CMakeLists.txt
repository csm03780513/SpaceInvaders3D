# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)
set(CMAKE_CXX_STANDARD 17)
project(SpaceInvaders3D LANGUAGES C CXX)
# Find Vulkan via NDK
find_package(Vulkan REQUIRED)
find_package (oboe REQUIRED CONFIG)


set(NATIVE_APP_GLUE_DIR ${CMAKE_SOURCE_DIR}/native_app_glue)
add_library(SpaceInvaders3D SHARED
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        main.cpp
        Renderer.cpp
        ${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c
        FontManager.cpp
        ParticleSystem.cpp
        SimpleSFXPlayer.h
        SimpleSFXPlayer.cpp
        #        volk/volk.c
)
target_include_directories(SpaceInvaders3D PRIVATE
        ${CMAKE_SOURCE_DIR}/native_app_glue
        ${CMAKE_SOURCE_DIR}/glm
        ${CMAKE_SOURCE_DIR}/stb
        ${CMAKE_SOURCE_DIR}/oboe/include
        ${CMAKE_SOURCE_DIR}/dr_libs
)
target_link_directories(SpaceInvaders3D PRIVATE
        #        ${CMAKE_SOURCE_DIR}/volk
)
add_library(native_app_glue STATIC ${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c)
target_link_libraries(SpaceInvaders3D
        ${Vulkan_LIBRARIES}
        android
        native_app_glue
        oboe::oboe
        log)

# For volk loader
#target_compile_definitions(SpaceInvaders3D PRIVATE VK_NO_PROTOTYPES)