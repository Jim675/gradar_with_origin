file(GLOB_RECURSE PROJECT_SOURCES *.cpp *.ui)

qt5_add_resources(QRC_FILES gradarvisual.qrc)
qt5_create_translation(QM_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/en_US.ts 
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/zh_CN.ts 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(GRadarVisual
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${QRC_FILES}
        ${QM_FILES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET test APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(GRadarVisual SHARED
            ${PROJECT_SOURCES}
            ${QRC_FILES}
            ${QM_FILES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(GRadarVisual
            ${PROJECT_SOURCES}
            ${QRC_FILES}
            ${QM_FILES}
        )
    endif()
endif()

target_include_directories(GRadarVisual PRIVATE
    GMapper2D/
    core/
    map/
    metstar/
    visual3d/
    util/
    ${VTK_INCLUDE_DIRS}
    ${LIBTORCH_PATH}/include
    ${LIBTORCH_PATH}/include/torch/csrc/api/include
)

target_link_directories(GRadarVisual PRIVATE
    ${LIBTORCH_PATH}/lib
)

target_link_libraries(GRadarVisual PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    OpenMP::OpenMP_CXX
    ${VTK_LIBRARIES}
    asmjit
    c10
    torch
    pytorch_jni
    torch_cpu
    fbjni
    dnnl
    kineto
    fbgemm
    XNNPACK
    cpuinfo
    clog
    libprotoc
    pthreadpool
    libprotobuf
    libprotobuf-lite
)

if(LIBTORCH_WITH_CUDA)
    target_link_libraries(GRadarVisual PRIVATE
        nvfuser_codegen
        torch_cuda
        caffe2_nvrtc
        c10_cuda
    )
endif()

set_target_properties(GRadarVisual PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# dll copy command 
file(GLOB DEP_DLL_FILES "${VTK_DIR}/../../../bin/*.dll" "${LIBTORCH_PATH}/lib/*.dll")

add_custom_command(TARGET GRadarVisual POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DEP_DLL_FILES} $<TARGET_FILE_DIR:GRadarVisual>
)