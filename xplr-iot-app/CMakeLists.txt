cmake_minimum_required(VERSION 3.20)
set(APP_NAME u-doom-xplr-iot-app)
project(${APP_NAME})
string(TOLOWER  ${CMAKE_SYSTEM_NAME} OS_NAME)

# This application
add_executable(${APP_NAME} main.c)

# Definitions
add_compile_definitions(U_CFG_APP_SHORT_RANGE_UART=2)

# Get and build the ubxlib library
set(UBXLIB_BASE ${CMAKE_CURRENT_LIST_DIR}/../components/ubxlib)
include(${UBXLIB_BASE}/port/platform/${OS_NAME}/${OS_NAME}.cmake)
target_link_libraries(${APP_NAME} ubxlib ${UBXLIB_REQUIRED_LINK_LIBS})
target_include_directories(${APP_NAME} PUBLIC ${UBXLIB_INC} ${UBXLIB_PUBLIC_INC_PORT})
