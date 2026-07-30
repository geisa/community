cmake_minimum_required(VERSION 3.21)
project(adm_client C)

set(WAKAAMA_INCLUDE_DIR "/usr/include/wakaama" CACHE PATH "Path to the Wakaama header directory")
set(WAKAAMA_TINYDTLS_INCLUDE_DIR "/usr/include/tinydtls" CACHE PATH "Path to the Wakaama TinyDTLS header directory")
set(WAKAAMA_LIB_DIR "/usr/lib/wakaama" CACHE PATH "Path to the Wakaama static library directory")

set(GEISA_PACKAGE_SCRIPT_PATH "/usr/sbin/manage_package.sh" CACHE STRING "Path to manage_package.sh")
add_definitions(-DGEISA_PACKAGE_SCRIPT_PATH=\"${GEISA_PACKAGE_SCRIPT_PATH}\")

add_executable(adm_client
    sources/lwm2mclient.c
    sources/object_connectivity_moni.c
    sources/object_device.c
    sources/object_firmware.c
    sources/object_location.c
    sources/object_security.c
    sources/object_server.c
    sources/object_software.c
    sources/system_api.c
    sources/shared/commandline.c
    sources/shared/platform.c
)

target_compile_definitions(adm_client PRIVATE
    LWM2M_CLIENT_MODE
    LWM2M_BOOTSTRAP
    LWM2M_SUPPORT_SENML_JSON
    LWM2M_COAP_DEFAULT_BLOCK_SIZE=1024
    LWM2M_COAP_MAX_MESSAGE_SIZE=2048
)

# Build-time include directories
target_include_directories(adm_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/shared
    ${WAKAAMA_INCLUDE_DIR}
    ${WAKAAMA_TINYDTLS_INCLUDE_DIR}
    ${WAKAAMA_TINYDTLS_INCLUDE_DIR}/include
    ${WAKAAMA_TINYDTLS_INCLUDE_DIR}/third_party
    ${WAKAAMA_TINYDTLS_INCLUDE_DIR}/third_party/tinydtls
)

# Build-time link to static library
target_link_libraries(adm_client PRIVATE
    ${WAKAAMA_LIB_DIR}/libwakaama_static.a
)