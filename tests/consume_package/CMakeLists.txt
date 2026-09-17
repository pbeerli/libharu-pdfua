# A minimal external consumer of the *installed* hpdf_ua package, used
# only by tests/test_install.sh to prove `find_package(hpdf_ua)` and
# `hpdf_ua::hpdf_ua` actually work end-to-end for someone who installed
# this project rather than vendoring its source tree -- not part of the
# main project build (no add_subdirectory() call for this directory
# anywhere in the top-level CMakeLists.txt).
cmake_minimum_required(VERSION 3.16)
project(hpdf_ua_consume_test LANGUAGES C)

find_package(hpdf_ua REQUIRED)

add_executable(consume_test main.c)
target_link_libraries(consume_test PRIVATE hpdf_ua::hpdf_ua)
