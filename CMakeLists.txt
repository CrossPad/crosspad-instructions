file(GLOB_RECURSE INSTRUCTIONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

idf_component_register(
    SRCS ${INSTRUCTIONS_SRCS}
    INCLUDE_DIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    REQUIRES
        lvgl
        crosspad-core
        crosspad-gui
)