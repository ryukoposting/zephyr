# Copyright (c) 2025 Evan Perry Grove
# SPDX-License-Identifier: Apache-2.0

function(zephyr_add_rust_library LIB_NAME)
	set(oneValueArgs CRATE_NAME PROFILE TARGET)
	set(multiValueArgs FEATURES)
	cmake_parse_arguments(PARSE_ARGV 1 arg
		"" "${oneValueArgs}" "${multiValueArgs}"
	)

	if(DEFINED arg_CRATE_NAME)
		set(CRATE_NAME ${arg_CRATE_NAME})
	else()
		set(CRATE_NAME ${LIB_NAME})
	endif()

	zephyr_get_rust_target(TARGET "${arg_TARGET}")
	set(TARGET_FLAG --target=${TARGET})
	message(STATUS "rust: Selected target ${TARGET} for crate ${CRATE_NAME}")

	include(ExternalProject)

	set(CRATE_DIR   ${CMAKE_CURRENT_LIST_DIR}/${CRATE_NAME})
	set(BUILD_DIR   ${CMAKE_CURRENT_BINARY_DIR}/${CRATE_NAME})
	set(INCLUDE_DIR ${BINARY_DIR_INCLUDE}/generated/${CRATE_NAME})
	
	file(MAKE_DIRECTORY ${BUILD_DIR})
	file(MAKE_DIRECTORY ${INCLUDE_DIR})
	
	if(${arg_PROFILE} STREQUAL "dev")
	set(LIB_DIR ${BUILD_DIR}/${TARGET}/debug)
	else()
	set(LIB_DIR ${BUILD_DIR}/${TARGET}/${arg_PROFILE})
	endif()

	set(LIB_FILE ${LIB_DIR}/lib${CRATE_NAME}.a)
	set(BINDINGS_FILE ${INCLUDE_DIR}/bindings.h)

	if(args_FEATURES)
		string(REPLACE ";" "," FEATURES_DELIM "${args_FEATURES}")
		set(FEATURES_FLAG --features ${FEATURES_DELIM})
	endif()

	ExternalProject_Add(
		${LIB_NAME}_project
		PREFIX ${BUILD_DIR}
		SOURCE_DIR ${CRATE_DIR}
		BINARY_DIR ${CRATE_DIR}
		CONFIGURE_COMMAND cbindgen --config ${CRATE_DIR}/cbindgen.toml --crate ${CRATE_NAME} --output ${BINDINGS_FILE} ${CRATE_DIR}
		BUILD_COMMAND cargo build -p ${CRATE_NAME} --manifest-path=${CRATE_DIR}/Cargo.toml ${TARGET_FLAG} --target-dir ${BUILD_DIR} --profile ${arg_PROFILE} ${FEATURES_FLAG}
		INSTALL_COMMAND ""
		BUILD_BYPRODUCTS ${LIB_FILE} ${BINDINGS_FILE}
	)

	add_library(${LIB_NAME} STATIC IMPORTED GLOBAL)
	add_dependencies(
		${LIB_NAME}
		${LIB_NAME}_project
	)

	set_target_properties(${LIB_NAME} PROPERTIES IMPORTED_LOCATION             ${LIB_FILE})
	set_target_properties(${LIB_NAME} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${INCLUDE_DIR})
endfunction()
