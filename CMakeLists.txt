cmake_minimum_required(VERSION 3.16)
project(libharu-pdfua
    VERSION 0.1.0
    DESCRIPTION "PDF/UA-1 tagged-PDF extensions for libharu"
    LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
    PRIVATE
        vendor/libharu/include
        ${CMAKE_CURRENT_BINARY_DIR}/vendor/libharu/include
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
endif()

# --- tests (CTest wiring placeholder; see tests/README.md) --------------
enable_testing()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()
