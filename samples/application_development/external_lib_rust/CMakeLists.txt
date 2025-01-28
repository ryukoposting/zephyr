# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(external_lib_rust)

target_sources(app PRIVATE src/main.c)

zephyr_add_rust_library(mylib
	PROFILE release
)

target_link_libraries(app PUBLIC mylib)
