file(GLOB_RECURSE INSTRUCTIONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

idf_component_register(
    SRCS ${INSTRUCTIONS_SRCS}
    REQUIRES
        lvgl
        crosspad-core
        crosspad-gui
)

# Preserve static AppRegistrar constructors (REGISTER_APP / auto-registration)
set_property(TARGET ${COMPONENT_LIB} PROPERTY WHOLE_ARCHIVE ON)