cmake_minimum_required(VERSION 2.8.5)
project(germz LANGUAGES C)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT AMIGA)
	message(SEND_ERROR "This project only compiles for Amiga")
endif()

# ACE
add_subdirectory(deps/ace ace)

set(CMAKE_C_STANDARD 11)
file(GLOB_RECURSE SOURCES src/*.c src/*.h)
add_executable(germz ${SOURCES})
target_include_directories(germz PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_compile_options(germz PUBLIC -Wall -Wextra)
target_link_libraries(germz ace)
if(GAME_DEBUG)
  target_compile_definitions(germz PRIVATE GAME_DEBUG)
endif()

file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data")
SET(TOOLS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/deps/ace/tools")
SET(BITMAP_EXTRACT "${TOOLS_PATH}/bin/bitmap_extract")
SET(TILESET_CONV "${TOOLS_PATH}/bin/tileset_conv")

# Palette
file(GLOB GPL_PATH _res/germz.gpl)
set(PLT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/data/germz.plt)
convertPalette(germz ${GPL_PATH} ${PLT_PATH})

# Blobs & pleps
# Left plep anim = right plep anim mirrored Y
# Up plep anim = right plep anim rotated by 90deg
# Down plep anim = up plep anim mirrored X
# TODO: Automate
set(transparency_hex "#110000")
convertBitmaps(
	TARGET germz PALETTE ${PLT_PATH} MASK_COLOR ${transparency_hex}
	INTERLEAVED SOURCES
	_res/blob0.png _res/blob1.png _res/blob2.png _res/blob3.png _res/blob4.png
	_res/blob5.png _res/blob6.png _res/links.png _res/cursors.png
	_res/plep0.png _res/plep1.png _res/plep2.png _res/plep3.png _res/monitor.png
	_res/plep/born_right.png _res/plep/move_right.png _res/plep/win_right.png _res/plep/lose_right.png
	_res/plep/born_left.png _res/plep/move_left.png _res/plep/win_left.png _res/plep/lose_left.png
	_res/plep/born_up.png _res/plep/move_up.png _res/plep/win_up.png _res/plep/lose_up.png
	_res/plep/born_down.png _res/plep/move_down.png _res/plep/win_down.png _res/plep/lose_down.png
	DESTINATIONS
		data/blob0.bm data/blob1.bm data/blob2.bm data/blob3.bm data/blob4.bm
		data/blob5.bm data/blob6.bm data/links.bm data/cursors.bm
		data/plep0.bm data/plep1.bm data/plep2.bm data/plep3.bm data/monitor.bm
		data/born_right.bm data/move_right.bm data/win_right.bm data/lose_right.bm
		data/born_left.bm data/move_left.bm data/win_left.bm data/lose_left.bm
		data/born_up.bm data/move_up.bm data/win_up.bm data/lose_up.bm
		data/born_down.bm data/move_down.bm data/win_down.bm data/lose_down.bm
	MASKS
		data/blob_mask.bm NONE NONE NONE NONE
		NONE NONE NONE data/cursors_mask.bm
		data/plep_mask.bm NONE NONE NONE NONE
		data/born_right_mask.bm data/move_right_mask.bm data/win_right_mask.bm data/lose_right_mask.bm
		data/born_left_mask.bm data/move_left_mask.bm data/win_left_mask.bm data/lose_left_mask.bm
		data/born_up_mask.bm data/move_up_mask.bm data/win_up_mask.bm data/lose_up_mask.bm
		data/born_down_mask.bm data/move_down_mask.bm data/win_down_mask.bm data/lose_down_mask.bm
)
