cmake_minimum_required(VERSION 3.16)
project(libharu-pdfua
    VERSION 0.7.0
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
# Static build and no bundled examples still match migrate-n's own
# CMakeLists.txt recipe for vendoring this same library. PNG discovery is
# the one deliberate divergence from that shared recipe, added in
# Milestone 6 (see docs/roadmap.md) once this project's own demo set
# needed real PNG support (tagged_png_demo.c, tagged_image_transform_demo.c)
# -- migrate-n itself still has no use for libharu's PNG/JPEG image
# support and is unaffected by this project's own CMakeLists.txt anyway
# (it vendors libharu separately).
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static bundled libraries" FORCE)
set(LIBHPDF_EXAMPLES OFF CACHE BOOL "Do not build bundled libharu examples" FORCE)
add_subdirectory(vendor/libharu EXCLUDE_FROM_ALL)

# vendor/libharu's own find_package(PNG) result (PNG_FOUND) doesn't
# propagate out of add_subdirectory()'s child scope -- confirmed directly
# (a debug message(STATUS "${PNG_FOUND}") printed empty here even with
# libpng installed) -- so call it again in this scope to gate
# tagged_png_demo/tagged_image_transform_demo below on the same real
# result libharu itself already found.
find_package(PNG)

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
        # The generated (${CMAKE_CURRENT_BINARY_DIR}) copy of
        # vendor/libharu/include comes BEFORE the source-tree one,
        # deliberately: a real, confirmed hazard while porting
        # tagged_png_demo.c (Milestone 6) -- a stale, gitignored
        # hpdf_config.h left behind in the source tree by an unrelated
        # tool (migrate-n's own configure script had written one there,
        # with NOPNG/NOJPEG defined) silently shadowed the correct,
        # freshly-generated one, because vendor/libharu's own (unmodified,
        # see NOTICE.md) CMakeLists.txt adds its source include dir before
        # its binary one. We can't fix that in vendor/libharu itself, but
        # we CAN make sure our own targets always prefer the generated
        # header over any stray file that happens to exist in the source
        # tree.
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/vendor/libharu/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor/libharu/include>
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

    # Milestone 6, third demo pass: security/annotations/attachments --
    # still no new dependency or licensed asset (see docs/roadmap.md's
    # Milestone 6 section).
    add_executable(tagged_encryption_demo demo/tagged_encryption_demo.c)
    target_link_libraries(tagged_encryption_demo PRIVATE hpdf_ua)
    target_compile_definitions(tagged_encryption_demo PRIVATE
        HPDF_UA_DEMO_FONT_PATH="${HPDF_UA_DEMO_FONT_PATH}")

    add_executable(tagged_text_annotation_demo demo/tagged_text_annotation_demo.c)
    target_link_libraries(tagged_text_annotation_demo PRIVATE hpdf_ua)
    target_compile_definitions(tagged_text_annotation_demo PRIVATE
        HPDF_UA_DEMO_FONT_PATH="${HPDF_UA_DEMO_FONT_PATH}")

    add_executable(tagged_attach_demo demo/tagged_attach_demo.c)
    target_link_libraries(tagged_attach_demo PRIVATE hpdf_ua)
    target_compile_definitions(tagged_attach_demo PRIVATE
        HPDF_UA_DEMO_FONT_PATH="${HPDF_UA_DEMO_FONT_PATH}"
        HPDF_UA_DEMO_ATTACH_PATH="${CMAKE_CURRENT_SOURCE_DIR}/CHANGES.md")

    add_executable(tagged_slide_show_demo demo/tagged_slide_show_demo.c)
    target_link_libraries(tagged_slide_show_demo PRIVATE hpdf_ua)
    target_compile_definitions(tagged_slide_show_demo PRIVATE
        HPDF_UA_DEMO_FONT_PATH="${HPDF_UA_DEMO_FONT_PATH}")

    # Milestone 6, fourth demo pass: PNG images -- the first demos in this
    # project needing a real new dependency, libpng (see this file's own
    # top comment on why PNG discovery is no longer disabled). Skipped
    # gracefully, not a hard configure error, if libpng isn't available,
    # matching this option's own "best effort" spirit for LIBHARU_PDFUA_BUILD_DEMOS.
    if(PNG_FOUND)
        set(HPDF_UA_DEMO_PNG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/images/pngsuite")

        add_executable(tagged_png_demo demo/tagged_png_demo.c)
        target_link_libraries(tagged_png_demo PRIVATE hpdf_ua)
        target_compile_definitions(tagged_png_demo PRIVATE
            HPDF_UA_DEMO_FONT_PATH="${HPDF_UA_DEMO_FONT_PATH}"
            HPDF_UA_DEMO_PNG_DIR="${HPDF_UA_DEMO_PNG_DIR}")

        add_executable(tagged_image_transform_demo demo/tagged_image_transform_demo.c)
        target_link_libraries(tagged_image_transform_demo PRIVATE hpdf_ua)
        target_compile_definitions(tagged_image_transform_demo PRIVATE
            HPDF_UA_DEMO_FONT_PATH="${HPDF_UA_DEMO_FONT_PATH}"
            HPDF_UA_DEMO_PNG_DIR="${HPDF_UA_DEMO_PNG_DIR}")
    else()
        message(STATUS "libpng not found -- tagged_png_demo and "
            "tagged_image_transform_demo will not be built (install "
            "libpng dev headers, e.g. `brew install libpng` or "
            "`apt install libpng-dev`, and reconfigure to enable them)")
    endif()

    # Milestone 6, sixth demo pass: JPEG. No new dependency (libharu
    # embeds a JPEG's own DCT-encoded byte stream as-is, no decode), so
    # unlike the PNG demos above this one always builds. Its two images
    # are the project maintainer's own photographs, CC BY 4.0 licensed
    # for this specific use (see NOTICE.md and demo/tagged_jpeg_demo.c's
    # own top comment for why -- the original demo's own sample images
    # had no stated license, which held this port back in the previous
    # pass).
    add_executable(tagged_jpeg_demo demo/tagged_jpeg_demo.c)
    target_link_libraries(tagged_jpeg_demo PRIVATE hpdf_ua)
    target_compile_definitions(tagged_jpeg_demo PRIVATE
        HPDF_UA_DEMO_FONT_PATH="${HPDF_UA_DEMO_FONT_PATH}"
        HPDF_UA_DEMO_JPEG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/images/jpeg-demo")

    # Milestone 6, CJK demo batch: real embedded Japanese/Simplified-
    # Chinese TrueType fonts (see NOTICE.md for why these specific font
    # files, not upstream Noto CJK's own OTF/OTC releases).
    set(HPDF_UA_DEMO_JP_FONT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/fonts/NotoSansJP-Regular.ttf")
    set(HPDF_UA_DEMO_SC_FONT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/fonts/NotoSansSC-Regular.ttf")

    add_executable(tagged_japanese_font_demo demo/tagged_japanese_font_demo.c)
    target_link_libraries(tagged_japanese_font_demo PRIVATE hpdf_ua)
    target_compile_definitions(tagged_japanese_font_demo PRIVATE
        HPDF_UA_DEMO_JP_FONT_PATH="${HPDF_UA_DEMO_JP_FONT_PATH}")

    add_executable(tagged_chfont_demo demo/tagged_chfont_demo.c)
    target_link_libraries(tagged_chfont_demo PRIVATE hpdf_ua)
    target_compile_definitions(tagged_chfont_demo PRIVATE
        HPDF_UA_DEMO_JP_FONT_PATH="${HPDF_UA_DEMO_JP_FONT_PATH}"
        HPDF_UA_DEMO_SC_FONT_PATH="${HPDF_UA_DEMO_SC_FONT_PATH}")
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
