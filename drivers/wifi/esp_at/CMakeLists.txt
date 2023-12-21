# SPDX-License-Identifier: Apache-2.0

if(CONFIG_WIFI_ESP_AT)

  zephyr_library_include_directories(
    ${ZEPHYR_BASE}/drivers/modem
  )

  zephyr_library_sources(
    esp.c
    esp_socket.c
    esp_offload.c
  )

  zephyr_library_sources_ifdef(CONFIG_WIFI_ESP_BLUETOOTH_SHIM esp_bt.c)
endif()
