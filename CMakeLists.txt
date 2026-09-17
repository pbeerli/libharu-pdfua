cmake_minimum_required(VERSION 3.16)
project(libharu-pdfua
    VERSION 0.6.5
    DESCRIPTION "PDF/UA-1 tagged-PDF extensions for libharu"
    LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

# --- vendored libharu, unmodified (see NOTICE.md) ---------------------
# Same recipe migrate-n's own CMakeLists.txt uses to vendor this same
# library, kept in sync deliberately: static build, no bundled PNG
# discovery (this project doesn't need libharu's PNG/JPEG image support),
# no bundled examples (this project has its own demos, see demo/).
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static bundled libraries" FORCE)
set(CMAKE_DISABLE_FIND_PACKAGE_PNG ON)
set(LIBHPDF_EXAMPLES OFF CACHE BOOL "Do not build bundled libharu examples" FORCE)
add_subdirectory(vendor/libharu EXCLUDE_FROM_ALL)

# --- this project's own PDF/UA tagging module --------------------------
add_library(hpdf_ua STATIC
    src/ua/hpdf_ua_docmeta.c
    src/ua/hpdf_ua_context.c
    src/ua/hpdf_ua_structure.c
    src/ua/hpdf_ua_metadata.c
)
target_include_directories(hpdf_ua
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor/libharu/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/vendor/libharu/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(hpdf_ua PUBLIC hpdf)

# --- demos --------------------------------------------------------------
option(LIBHARU_PDFUA_BUILD_DEMOS "Build the tagged-PDF demo programs" ON)
if(LIBHARU_PDFUA_BUILD_DEMOS)
    # Absolute path to the vendored embeddable demo font (Milestone 4), so
    # each demo can load it regardless of the process's working directory.
    set(HPDF_UA_DEMO_FONT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/fonts/DejaVuSans.ttf")

    add_executable(tagged_table_demo demo/tagged_table_demo.c)
    target_link_libraries(tagged_table_demo PRIVATE hpdf_ua)
    target_compile_definitions(tagged_table_demo PRIVATE
        HPDF_UA_DEMO_FONT_PATH="${HPDF_UA_DEMO_FONT_PATH}")

    add_executable(tagged_histogram_demo demo/tagged_histogram_demo.c)
    target_link_libraries(tagged_histogram_demo PRIVATE hpdf_ua)
    target_compile_definitions(tagged_histogram_demo PRIVATE
        HPDF_UA_DEMO_FONT_PATH="${HPDF_UA_DEMO_FONT_PATH}")

    add_executable(tagged_skyline_demo demo/tagged_skyline_demo.c)
    target_link_libraries(tagged_skyline_demo PRIVATE hpdf_ua)
    target_compile_definitions(tagged_skyline_demo PRIVATE
        HPDF_UA_DEMO_FONT_PATH="${HPDF_UA_DEMO_FONT_PATH}")

    add_executable(tagged_example_demo demo/tagged_example_demo.c)
    target_link_libraries(tagged_example_demo PRIVATE hpdf_ua)
    target_compile_definitions(tagged_example_demo PRIVATE
        HPDF_UA_DEMO_FONT_PATH="${HPDF_UA_DEMO_FONT_PATH}")

    add_executable(docmeta_demo demo/docmeta_demo.c)
    target_link_libraries(docmeta_demo PRIVATE hpdf_ua)

    # Milestone 6: tagged ports of a representative slice of libharu's own
    # original demo set (vendor/libharu's demo/ directory was trimmed when
    # vendored, see NOTICE.md) -- a font demo, an image demo, and an
    # annotation demo, each using this project's own tagging API instead of
    # being left untagged.
    add_executable(tagged_font_demo demo/tagged_font_demo.c)
    target_link_libraries(tagged_font_demo PRIVATE hpdf_ua)
    target_compile_definitions(tagged_font_demo PRIVATE
        HPDF_UA_DEMO_FONT_PATH="${HPDF_UA_DEMO_FONT_PATH}")

    add_executable(tagged_image_demo demo/tagged_image_demo.c)
    target_link_libraries(tagged_image_demo PRIVATE hpdf_ua)
    target_compile_definitions(tagged_image_demo PRIVATE
        HPDF_UA_DEMO_FONT_PATH="${HPDF_UA_DEMO_FONT_PATH}")

    add_executable(tagged_annotation_demo demo/tagged_annotation_demo.c)
    target_link_libraries(tagged_annotation_demo PRIVATE hpdf_ua)
    target_compile_definitions(tagged_annotation_demo PRIVATE
        HPDF_UA_DEMO_FONT_PATH="${HPDF_UA_DEMO_FONT_PATH}")

    # Milestone 6, second demo pass: tagged ports of a first, no-new-
    # dependency batch of libharu's remaining original demos (see
    # docs/roadmap.md's Milestone 6 section) -- vector graphics, extended
    # graphics state, the Standard-14 font list, text features, encoding
    # selection, and a multi-entry document outline.
    add_executable(tagged_arc_demo demo/tagged_arc_demo.c)
    target_link_libraries(tagged_arc_demo PRIVATE hpdf_ua)
    target_compile_definitions(tagged_arc_demo PRIVATE
        HPDF_UA_DEMO_FONT_PATH="${HPDF_UA_DEMO_FONT_PATH}")

    add_executable(tagged_line_demo demo/tagged_line_demo.c)
    target_link_libraries(tagged_line_demo PRIVATE hpdf_ua)
    target_compile_definitions(tagged_line_demo PRIVATE
        HPDF_UA_DEMO_FONT_PATH="${HPDF_UA_DEMO_FONT_PATH}")

    add_executable(tagged_ext_gstate_demo demo/tagged_ext_gstate_demo.c)
    target_link_libraries(tagged_ext_gstate_demo PRIVATE hpdf_ua)
    target_compile_definitions(tagged_ext_gstate_demo PRIVATE
        HPDF_UA_DEMO_FONT_PATH="${HPDF_UA_DEMO_FONT_PATH}")

    add_executable(tagged_font_list_demo demo/tagged_font_list_demo.c)
    target_link_libraries(tagged_font_list_demo PRIVATE hpdf_ua)

    add_executable(tagged_text_demo demo/tagged_text_demo.c)
    target_link_libraries(tagged_text_demo PRIVATE hpdf_ua)
    target_compile_definitions(tagged_text_demo PRIVATE
        HPDF_UA_DEMO_FONT_PATH="${HPDF_UA_DEMO_FONT_PATH}")

    add_executable(tagged_encoding_list_demo demo/tagged_encoding_list_demo.c)
    target_link_libraries(tagged_encoding_list_demo PRIVATE hpdf_ua)
    target_compile_definitions(tagged_encoding_list_demo PRIVATE
        HPDF_UA_DEMO_FONT_PATH="${HPDF_UA_DEMO_FONT_PATH}")

    add_executable(tagged_outline_demo demo/tagged_outline_demo.c)
    target_link_libraries(tagged_outline_demo PRIVATE hpdf_ua)
    target_compile_definitions(tagged_outline_demo PRIVATE
        HPDF_UA_DEMO_FONT_PATH="${HPDF_UA_DEMO_FONT_PATH}")
endif()

# --- tests (CTest wiring placeholder; see tests/README.md) --------------
enable_testing()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()

# --- installation ---------------------------------------------------------
# Lets a downstream project do `find_package(hpdf_ua)` and link
# `hpdf_ua::hpdf_ua` instead of vendoring this whole tree itself. `hpdf`
# (vendor/libharu's own target) is exported in the same set since hpdf_ua
# links it PUBLICly -- CMake requires every target a target's usage
# requirements point at to belong to some export set too. Both this
# project's own header and libharu's headers land flat under
# CMAKE_INSTALL_INCLUDEDIR (matching libharu's own upstream install
# convention, since hpdf_ua.h itself does `#include "hpdf.h"` expecting it
# on the plain include path, not a subdirectory).
install(TARGETS hpdf
    EXPORT hpdf_ua-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY vendor/libharu/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vendor/libharu/include/hpdf_config.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(TARGETS hpdf_ua
    EXPORT hpdf_ua-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES include/hpdf_ua/hpdf_ua.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hpdf_ua
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/hpdf_ua-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
    cmake/hpdf_ua-config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/hpdf_ua-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hpdf_ua
)
install(EXPORT hpdf_ua-targets
    FILE hpdf_ua-targets.cmake
    NAMESPACE hpdf_ua::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hpdf_ua
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/hpdf_ua-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/hpdf_ua-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hpdf_ua
)
