cmake_minimum_required(VERSION 3.20)
set(APP_NAME u-doom)
project(${APP_NAME})
string(TOLOWER  ${CMAKE_SYSTEM_NAME} OS_NAME)
set(DOOMGENERIC_DIR ../components/doomgeneric/doomgeneric)
set(LODEPNG_DIR ../lodepng)

# This application
add_executable(
    ${APP_NAME} ubx_doom_port.c
    usleep.c
    ${DOOMGENERIC_DIR}/dummy.c
    ${DOOMGENERIC_DIR}/am_map.c
    ${DOOMGENERIC_DIR}/doomdef.c
    ${DOOMGENERIC_DIR}/doomstat.c
    ${DOOMGENERIC_DIR}/dstrings.c
    ${DOOMGENERIC_DIR}/d_event.c
    ${DOOMGENERIC_DIR}/d_items.c
    ${DOOMGENERIC_DIR}/d_iwad.c
    ${DOOMGENERIC_DIR}/d_loop.c
    ${DOOMGENERIC_DIR}/d_main.c
    ${DOOMGENERIC_DIR}/d_mode.c
    ${DOOMGENERIC_DIR}/d_net.c
    ${DOOMGENERIC_DIR}/f_finale.c
    ${DOOMGENERIC_DIR}/f_wipe.c
    ${DOOMGENERIC_DIR}/g_game.c
    ${DOOMGENERIC_DIR}/hu_lib.c
    ${DOOMGENERIC_DIR}/hu_stuff.c
    ${DOOMGENERIC_DIR}/info.c
    ${DOOMGENERIC_DIR}/i_cdmus.c
    ${DOOMGENERIC_DIR}/i_endoom.c
    ${DOOMGENERIC_DIR}/i_joystick.c
    ${DOOMGENERIC_DIR}/i_scale.c
    ${DOOMGENERIC_DIR}/i_sound.c
    ${DOOMGENERIC_DIR}/i_system.c
    ${DOOMGENERIC_DIR}/i_timer.c
    ${DOOMGENERIC_DIR}/memio.c
    ${DOOMGENERIC_DIR}/m_argv.c
    ${DOOMGENERIC_DIR}/m_bbox.c
    ${DOOMGENERIC_DIR}/m_cheat.c
    ${DOOMGENERIC_DIR}/m_config.c
    ${DOOMGENERIC_DIR}/m_controls.c
    ${DOOMGENERIC_DIR}/m_fixed.c
    ${DOOMGENERIC_DIR}/m_menu.c
    ${DOOMGENERIC_DIR}/m_misc.c
    ${DOOMGENERIC_DIR}/m_random.c
    ${DOOMGENERIC_DIR}/p_ceilng.c
    ${DOOMGENERIC_DIR}/p_doors.c
    ${DOOMGENERIC_DIR}/p_enemy.c
    ${DOOMGENERIC_DIR}/p_floor.c
    ${DOOMGENERIC_DIR}/p_inter.c
    ${DOOMGENERIC_DIR}/p_lights.c
    ${DOOMGENERIC_DIR}/p_map.c
    ${DOOMGENERIC_DIR}/p_maputl.c
    ${DOOMGENERIC_DIR}/p_mobj.c
    ${DOOMGENERIC_DIR}/p_plats.c
    ${DOOMGENERIC_DIR}/p_pspr.c
    ${DOOMGENERIC_DIR}/p_saveg.c
    ${DOOMGENERIC_DIR}/p_setup.c
    ${DOOMGENERIC_DIR}/p_sight.c
    ${DOOMGENERIC_DIR}/p_spec.c
    ${DOOMGENERIC_DIR}/p_switch.c
    ${DOOMGENERIC_DIR}/p_telept.c
    ${DOOMGENERIC_DIR}/p_tick.c
    ${DOOMGENERIC_DIR}/p_user.c
    ${DOOMGENERIC_DIR}/r_bsp.c
    ${DOOMGENERIC_DIR}/r_data.c
    ${DOOMGENERIC_DIR}/r_draw.c
    ${DOOMGENERIC_DIR}/r_main.c
    ${DOOMGENERIC_DIR}/r_plane.c
    ${DOOMGENERIC_DIR}/r_segs.c
    ${DOOMGENERIC_DIR}/r_sky.c
    ${DOOMGENERIC_DIR}/r_things.c
    ${DOOMGENERIC_DIR}/sha1.c
    ${DOOMGENERIC_DIR}/sounds.c
    ${DOOMGENERIC_DIR}/statdump.c
    ${DOOMGENERIC_DIR}/st_lib.c
    ${DOOMGENERIC_DIR}/st_stuff.c
    ${DOOMGENERIC_DIR}/s_sound.c
    ${DOOMGENERIC_DIR}/tables.c
    ${DOOMGENERIC_DIR}/v_video.c
    ${DOOMGENERIC_DIR}/wi_stuff.c
    ${DOOMGENERIC_DIR}/w_checksum.c
    ${DOOMGENERIC_DIR}/w_file.c
    ${DOOMGENERIC_DIR}/w_main.c
    ${DOOMGENERIC_DIR}/w_wad.c
    ${DOOMGENERIC_DIR}/z_zone.c
    ${DOOMGENERIC_DIR}/w_file_stdc.c
    ${DOOMGENERIC_DIR}/i_input.c
    ${DOOMGENERIC_DIR}/i_video.c
    ${DOOMGENERIC_DIR}/doomgeneric.c
    ${LODEPNG_DIR}/lodepng.c
)

# Definitions
add_compile_definitions(
    U_CFG_APP_SHORT_RANGE_UART=5
    U_CFG_APP_SHORT_RANGE_MODULE_TYPE=U_SHORT_RANGE_MODULE_TYPE_NINA_W15
    U_SHORT_RANGE_UART_BAUD_RATE=921600
)

# Get and build the ubxlib library
set(UBXLIB_BASE ${CMAKE_CURRENT_LIST_DIR}/../components/ubxlib)
include(${UBXLIB_BASE}/port/platform/${OS_NAME}/${OS_NAME}.cmake)
target_link_libraries(${APP_NAME} ubxlib ${UBXLIB_REQUIRED_LINK_LIBS})
target_include_directories(
    ${APP_NAME} PUBLIC ${UBXLIB_INC} ${UBXLIB_PUBLIC_INC_PORT}
    ${DOOMGENERIC_DIR}
    ${LODEPNG_DIR}
)
