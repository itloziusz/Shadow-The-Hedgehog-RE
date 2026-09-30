add_library(shadow_boot_foundation STATIC src/BootFoundation.cpp src/ApplicationLoop.cpp)
target_compile_features(shadow_boot_foundation PUBLIC cxx_std_17)
target_include_directories(shadow_boot_foundation PUBLIC include)

add_executable(shadow_boot_probe src/boot_probe.cpp)
target_link_libraries(shadow_boot_probe PRIVATE shadow_boot_foundation)

add_executable(shadow_boot_tests tests/test_boot.cpp)
target_link_libraries(shadow_boot_tests PRIVATE shadow_boot_foundation)

add_executable(shadow_boot_application_loop_tests tests/test_application_loop.cpp)
target_link_libraries(shadow_boot_application_loop_tests PRIVATE shadow_boot_foundation)

# An exact PAL input is optional and read-only. The tests cover bounded entry
# and recurring-loop slices; neither establishes game boot or CRT parity.
set(SHADOW_BOOT_DOL_PATH "${PROJECT_SOURCE_DIR}/sys/main.dol" CACHE FILEPATH
    "Path to the user's PAL GUPP8P main.dol fixture (never committed)")
if(EXISTS "${SHADOW_BOOT_DOL_PATH}")
    add_test(NAME boot_pal_fixture
        COMMAND ${CMAKE_COMMAND}
            "-DDOL=${SHADOW_BOOT_DOL_PATH}"
            "-DPROGRAM=$<TARGET_FILE:shadow_boot_probe>"
            -DMODE=probe
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_fixture.cmake")
    add_test(NAME boot_pal_regression
        COMMAND ${CMAKE_COMMAND}
            "-DDOL=${SHADOW_BOOT_DOL_PATH}"
            "-DPROGRAM=$<TARGET_FILE:shadow_boot_tests>"
            -DMODE=regression
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_fixture.cmake")
    add_test(NAME boot_pal_application_loop
        COMMAND ${CMAKE_COMMAND}
            "-DDOL=${SHADOW_BOOT_DOL_PATH}"
            "-DPROGRAM=$<TARGET_FILE:shadow_boot_application_loop_tests>"
            -DMODE=loop
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_fixture.cmake")
endif()
