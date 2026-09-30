add_library(shadow_boot_foundation STATIC src/BootFoundation.cpp src/ApplicationLoop.cpp)
target_compile_features(shadow_boot_foundation PUBLIC cxx_std_17)
target_include_directories(shadow_boot_foundation PUBLIC include)

add_executable(shadow_boot_probe src/boot_probe.cpp)
target_link_libraries(shadow_boot_probe PRIVATE shadow_boot_foundation)

add_executable(shadow_boot_tests tests/test_boot.cpp)
target_link_libraries(shadow_boot_tests PRIVATE shadow_boot_foundation)

add_executable(shadow_boot_application_loop_tests tests/test_application_loop.cpp)
target_link_libraries(shadow_boot_application_loop_tests PRIVATE shadow_boot_foundation)

# Correct a reference-only hardware arithmetic bug with a focused executable
# check. This target does not connect the hardware path to the native entry.
add_executable(shadow_boot_fpscr_tests
    tests/test_fpscr.cpp
    experimental_native_boot/src/hardware_semantics.cpp)
target_compile_features(shadow_boot_fpscr_tests PRIVATE cxx_std_17)
target_include_directories(shadow_boot_fpscr_tests PRIVATE experimental_native_boot/src)
add_test(NAME boot_fpscr_arithmetic COMMAND shadow_boot_fpscr_tests)

# An exact PAL input is optional and read-only. The tests cover bounded entry
# and recurring-loop slices; neither establishes game boot or CRT parity.
set(SHADOW_BOOT_DOL_PATH "${PROJECT_SOURCE_DIR}/sys/main.dol" CACHE FILEPATH
    "Path to the user's PAL GUPP8P main.dol fixture (never committed)")
if(EXISTS "${SHADOW_BOOT_DOL_PATH}")
    find_package(Python3 QUIET COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(NAME boot_pal_raw_prefix
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_binary_note.py"
                "${SHADOW_BOOT_DOL_PATH}"
                "${CMAKE_CURRENT_SOURCE_DIR}/research/BINARY_BOOT_PREFIX.md"
                --expected-words 78 --expected-branches 8 --expected-sprs 17)
        add_test(NAME boot_pal_raw_fpr_prefix
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_binary_note.py"
                "${SHADOW_BOOT_DOL_PATH}"
                "${CMAKE_CURRENT_SOURCE_DIR}/research/BINARY_FPR_PREFIX.md"
                --expected-words 74 --expected-branches 1 --expected-sprs 1)
        add_test(NAME boot_pal_raw_cache_handler
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_binary_note.py"
                "${SHADOW_BOOT_DOL_PATH}"
                "${CMAKE_CURRENT_SOURCE_DIR}/research/BINARY_CACHE_HANDLER.md"
                --expected-words 305 --expected-branches 56 --expected-sprs 14)
        add_test(NAME boot_pal_raw_crt_prefix
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_binary_note.py"
                "${SHADOW_BOOT_DOL_PATH}"
                "${CMAKE_CURRENT_SOURCE_DIR}/research/BINARY_CRT_PREFIX.md"
                --expected-words 111 --expected-branches 25 --expected-sprs 0
                --expected-data-words 41)
    endif()
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
    add_test(NAME boot_pal_synthetic_hle_prefix
        COMMAND ${CMAKE_COMMAND}
            "-DDOL=${SHADOW_BOOT_DOL_PATH}"
            "-DPROGRAM=$<TARGET_FILE:shadow_boot_probe>"
            -DMODE=hle_prefix
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_fixture.cmake")
    add_test(NAME boot_pal_application_loop
        COMMAND ${CMAKE_COMMAND}
            "-DDOL=${SHADOW_BOOT_DOL_PATH}"
            "-DPROGRAM=$<TARGET_FILE:shadow_boot_application_loop_tests>"
            -DMODE=loop
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_fixture.cmake")
endif()
