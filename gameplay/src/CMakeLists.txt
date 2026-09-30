cmake_minimum_required(VERSION 3.16)
project(shadow_gameplay CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SHADOW_GAME_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../files" CACHE PATH
    "Path to the user's extracted game files (never committed)")
if(EXISTS "${SHADOW_GAME_FILES_DIR}/nukkoro2.inf")
    set(SHADOW_HAS_GAME_FILES TRUE)
else()
    set(SHADOW_HAS_GAME_FILES FALSE)
endif()

# Native reconstruction of Shadow the Hedgehog gameplay systems recovered from main.dol.
add_library(shadow_gameplay STATIC
    Task.cpp
    SetSystem.cpp
    EnemyFramework.cpp
    EnemyAI.cpp
    GunSoldierAI.cpp
    GunBeetleAI.cpp
    GunRobotAI.cpp
    EggPawnAI.cpp
    BkSoldierAI.cpp
    BkWingSmallAI.cpp
    BkWormAI.cpp
    BkLarva.cpp
    PlayerPhysics.cpp
    PlayerBehavior.cpp
    StageFlow.cpp
    Mission.cpp
    StageTable.generated.cpp
    SetObjCatalog.generated.cpp)
target_include_directories(shadow_gameplay PUBLIC include)

enable_testing()
add_executable(test_set_task tests/test_set_task.cpp)
target_link_libraries(test_set_task shadow_gameplay)
if(SHADOW_HAS_GAME_FILES)
    add_test(NAME set_task COMMAND test_set_task ${SHADOW_GAME_FILES_DIR})
endif()

add_executable(test_enemy_framework tests/test_enemy_framework.cpp)
target_link_libraries(test_enemy_framework shadow_gameplay)
add_test(NAME enemy_framework COMMAND test_enemy_framework)

add_executable(test_mission tests/test_mission.cpp)
target_link_libraries(test_mission shadow_gameplay)
if(SHADOW_HAS_GAME_FILES)
    add_test(NAME mission COMMAND test_mission ${SHADOW_GAME_FILES_DIR})
endif()

add_executable(test_gunsoldier_ai tests/test_gunsoldier_ai.cpp)
target_link_libraries(test_gunsoldier_ai shadow_gameplay)
if(SHADOW_HAS_GAME_FILES)
    add_test(NAME gunsoldier_ai COMMAND test_gunsoldier_ai ${SHADOW_GAME_FILES_DIR})
endif()

add_executable(test_bksoldier_ai tests/test_bksoldier_ai.cpp)
target_link_libraries(test_bksoldier_ai shadow_gameplay)
add_test(NAME bksoldier_ai COMMAND test_bksoldier_ai)

add_executable(test_gunbeetle_ai tests/test_gunbeetle_ai.cpp)
target_link_libraries(test_gunbeetle_ai shadow_gameplay)
add_test(NAME gunbeetle_ai COMMAND test_gunbeetle_ai)

add_executable(test_gunrobot_ai tests/test_gunrobot_ai.cpp)
target_link_libraries(test_gunrobot_ai shadow_gameplay)
add_test(NAME gunrobot_ai COMMAND test_gunrobot_ai)

add_executable(test_bkwingsmall_ai tests/test_bkwingsmall_ai.cpp)
target_link_libraries(test_bkwingsmall_ai shadow_gameplay)
add_test(NAME bkwingsmall_ai COMMAND test_bkwingsmall_ai)

add_executable(test_bkworm_ai tests/test_bkworm_ai.cpp)
target_link_libraries(test_bkworm_ai shadow_gameplay)
add_test(NAME bkworm_ai COMMAND test_bkworm_ai)

add_executable(test_eggpawn_ai tests/test_eggpawn_ai.cpp)
target_link_libraries(test_eggpawn_ai shadow_gameplay)
add_test(NAME eggpawn_ai COMMAND test_eggpawn_ai)

add_executable(test_bklarva tests/test_bklarva.cpp)
target_link_libraries(test_bklarva shadow_gameplay)
add_test(NAME bklarva COMMAND test_bklarva)

add_executable(test_player_physics tests/test_player_physics.cpp)
target_link_libraries(test_player_physics shadow_gameplay)
add_test(NAME player_physics COMMAND test_player_physics)

add_executable(test_player_behavior tests/test_player_behavior.cpp)
target_link_libraries(test_player_behavior shadow_gameplay)
add_test(NAME player_behavior COMMAND test_player_behavior)

# Integration simulator: all reconstructed modules on real stage data (harness stubs are labelled in the source)
add_executable(stage_sim sim/stage_sim.cpp)
target_link_libraries(stage_sim shadow_gameplay)
if(SHADOW_HAS_GAME_FILES)
add_test(NAME stage_sim_stg0100 COMMAND stage_sim ${SHADOW_GAME_FILES_DIR} 100)
add_test(NAME stage_sim_stg0100_no_respawn COMMAND stage_sim ${SHADOW_GAME_FILES_DIR} 100)
add_test(NAME stage_sim_stg0100_wing_native COMMAND stage_sim ${SHADOW_GAME_FILES_DIR} 100)
add_test(NAME stage_sim_stg0301_robot_native COMMAND stage_sim ${SHADOW_GAME_FILES_DIR} 301)
add_test(NAME stage_sim_stg0201_worm_native COMMAND stage_sim ${SHADOW_GAME_FILES_DIR} 201)
add_test(NAME stage_sim_stg0201 COMMAND stage_sim ${SHADOW_GAME_FILES_DIR} 201)
add_test(NAME stage_sim_stg0404 COMMAND stage_sim ${SHADOW_GAME_FILES_DIR} 404)
add_test(NAME stage_sim_stg0301 COMMAND stage_sim ${SHADOW_GAME_FILES_DIR} 301)
add_test(NAME stage_sim_stg0302 COMMAND stage_sim ${SHADOW_GAME_FILES_DIR} 302)
add_test(NAME stage_sim_stg0401 COMMAND stage_sim ${SHADOW_GAME_FILES_DIR} 401)
add_test(NAME stage_sim_stg0501 COMMAND stage_sim ${SHADOW_GAME_FILES_DIR} 501)
add_test(NAME stage_sim_stg0502 COMMAND stage_sim ${SHADOW_GAME_FILES_DIR} 502)
add_test(NAME stage_sim_stg0504 COMMAND stage_sim ${SHADOW_GAME_FILES_DIR} 504)
add_test(NAME stage_sim_stg0601 COMMAND stage_sim ${SHADOW_GAME_FILES_DIR} 601)
set_tests_properties(stage_sim_stg0100 PROPERTIES PASS_REGULAR_EXPRESSION "RESULT: Dark mission cleared -> next stage index 6 ")
set_tests_properties(stage_sim_stg0100_no_respawn PROPERTIES PASS_REGULAR_EXPRESSION "GUN Beetle duplicate SET-slot spawns: 0")
set_tests_properties(stage_sim_stg0100_wing_native PROPERTIES PASS_REGULAR_EXPRESSION "BK WingSmall native SET spawns: 18")
set_tests_properties(stage_sim_stg0301_robot_native PROPERTIES PASS_REGULAR_EXPRESSION "GUN Robot native SET spawns: 11")
set_tests_properties(stage_sim_stg0201_worm_native PROPERTIES PASS_REGULAR_EXPRESSION "BK Worm native SET spawns: 9")
set_tests_properties(stage_sim_stg0201 PROPERTIES PASS_REGULAR_EXPRESSION "RESULT: Hero mission cleared -> next stage index 11 ")
set_tests_properties(stage_sim_stg0404 PROPERTIES PASS_REGULAR_EXPRESSION "RESULT: Hero mission cleared -> next stage index 32 ")
set_tests_properties(stage_sim_stg0301 PROPERTIES PASS_REGULAR_EXPRESSION "RESULT: Dark mission cleared -> next stage index 13 ")
set_tests_properties(stage_sim_stg0302 PROPERTIES PASS_REGULAR_EXPRESSION "RESULT: Dark mission cleared -> next stage index 14 ")
set_tests_properties(stage_sim_stg0401 PROPERTIES PASS_REGULAR_EXPRESSION "RESULT: Dark mission cleared -> next stage index 30 ")
set_tests_properties(stage_sim_stg0501 PROPERTIES PASS_REGULAR_EXPRESSION "RESULT: Hero mission cleared -> next stage index 24 ")
set_tests_properties(stage_sim_stg0502 PROPERTIES PASS_REGULAR_EXPRESSION "RESULT: Dark mission cleared -> next stage index 34 ")
set_tests_properties(stage_sim_stg0504 PROPERTIES PASS_REGULAR_EXPRESSION "RESULT: Hero mission cleared -> next stage index 26 ")
set_tests_properties(stage_sim_stg0601 PROPERTIES PASS_REGULAR_EXPRESSION "RESULT: Dark mission cleared -> next stage index 38 ")
endif()
