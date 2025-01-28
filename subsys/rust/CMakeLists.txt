# Copyright (c) 2025 Evan Perry Grove
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_RUST)
	include(cmake/add_rust_library.cmake)
	include(cmake/get_rust_target.cmake)
endif()
