cmake_minimum_required(VERSION 3.16)

project(Proxima LANGUAGES CXX)

# For .ui files
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)

# .cpp .h .ui files
set(SOURCES
        source/main.cpp
        source/core/components/display.h
        source/core/components/display.cpp
        source/core/components/frame.h
        source/core/components/frame.cpp
        source/core/data/media_file.h
        source/core/data/media_file.cpp
        source/core/data/media_collection.h
        source/core/data/media_collection.cpp
        source/core/processing/image_processor.h
        source/core/processing/image_processor.cpp
        source/core/processing/color_correction.h
        source/core/processing/color_correction.cpp
        source/core/processing/deconvolution.h
        source/core/processing/deconvolution.cpp
        source/core/processing/wavelets.h
        source/core/processing/wavelets.cpp
        source/core/stacking/alignment.h
        source/core/stacking/alignment.cpp
        source/core/stacking/stacker.h
        source/core/stacking/stacker.cpp
        source/threading/thread.h
        source/threading/stack_thread.h
        source/threading/stack_thread.cpp
        source/threading/analysis_thread.h
        source/threading/analysis_thread.cpp
        source/ui/workspace.h
        source/ui/workspace.cpp
        source/ui/media_viewer.h
        source/ui/media_viewer.cpp
        source/ui/dialogs/stacking_dialog/stack_dialog.h
        source/ui/dialogs/stacking_dialog/stack_dialog.cpp
        source/ui/widgets/main_window/main_window.h
        source/ui/widgets/main_window/main_window.cpp
)

# OpenCV
set(OpenCV_DIR "C:/libs/opencv/build")
include_directories("C:/libs/opencv/build/include")
link_directories("C:/libs/opencv/build/x64/vc16/lib")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(OPENCV_LIBS opencv_world4110d)
else()
    set(OPENCV_LIBS opencv_world4110)
endif()

# Boost
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/boost)

include_directories(
    source
    source/core
    source/ui
    source/ui/widgets
    source/ui/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/libs
)

qt_add_executable(Proxima
        ${SOURCES}
)

target_link_libraries(Proxima PRIVATE
        Qt6::Widgets
        Qt6::Core
        Qt6::Gui
        ${OPENCV_LIBS}
)