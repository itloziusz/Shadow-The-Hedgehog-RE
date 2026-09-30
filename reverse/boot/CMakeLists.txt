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
                --expected-words 78 --expected-branches 8 --expected-sprs 17
                --required-code-range 80003154:80003158
                --required-code-range 800032B0:8000333C
                --required-code-range 80003400:80003420
                --required-code-range 80370BA8:80370BBC
                --required-code-range 80371714:80371764
                --required-code-range 803725F4:80372600)
        add_test(NAME boot_pal_raw_coverage_negative
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_verify_binary_note.py"
                "${SHADOW_BOOT_DOL_PATH}"
                "${CMAKE_CURRENT_SOURCE_DIR}/research/BINARY_BOOT_PREFIX.md")
        add_test(NAME boot_pal_raw_fpr_prefix
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_binary_note.py"
                "${SHADOW_BOOT_DOL_PATH}"
                "${CMAKE_CURRENT_SOURCE_DIR}/research/BINARY_FPR_PREFIX.md"
                --expected-words 74 --expected-branches 1 --expected-sprs 1
                --required-code-range 80370CDC:80370E00)
        add_test(NAME boot_pal_raw_paired_setup
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_binary_note.py"
                "${SHADOW_BOOT_DOL_PATH}"
                "${CMAKE_CURRENT_SOURCE_DIR}/research/PAIRED_SETUP_STATE.md"
                --expected-words 29 --expected-branches 3 --expected-sprs 14
                --required-code-range 80371714:80371764
                --required-code-range 80370BA8:80370BB4
                --required-code-range 803725F4:80372600)
        add_test(NAME boot_pal_raw_cache_handler
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_binary_note.py"
                "${SHADOW_BOOT_DOL_PATH}"
                "${CMAKE_CURRENT_SOURCE_DIR}/research/BINARY_CACHE_HANDLER.md"
                --expected-words 305 --expected-branches 56 --expected-sprs 14
                --required-code-range 80370ADC:80370AF0
                --required-code-range 80370AFC:80370B08
                --required-code-range 80370B60:80370BA4
                --required-code-range 80370C8C:80370CD8
                --required-code-range 803724F4:80372504
                --required-code-range 80372604:80372614
                --required-code-range 80372640:803726D4
                --required-code-range 80372838:80372928
                --required-code-range 80373378:8037358C
                --required-code-range 8037611C:8037612C
                --required-code-range 80376144:80376164)
        add_test(NAME boot_pal_raw_crt_prefix
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_binary_note.py"
                "${SHADOW_BOOT_DOL_PATH}"
                "${CMAKE_CURRENT_SOURCE_DIR}/research/BINARY_CRT_PREFIX.md"
                --expected-words 111 --expected-branches 25 --expected-sprs 0
                --expected-data-words 41
                --required-code-range 8000315C:8000316C
                --required-code-range 80003340:800033FC
                --required-code-range 8000540C:800054F0
                --required-data-range 80005544:800055E4)
        add_test(NAME boot_pal_crt_return_projection
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_crt_return_projection.py"
                "${SHADOW_BOOT_DOL_PATH}")
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
    add_test(NAME boot_pal_synthetic_hle_hid2_read
        COMMAND ${CMAKE_COMMAND}
            "-DDOL=${SHADOW_BOOT_DOL_PATH}"
            "-DPROGRAM=$<TARGET_FILE:shadow_boot_probe>"
            -DMODE=hle_hid2_read
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_fixture.cmake")
    add_test(NAME boot_pal_synthetic_hle_hid0_request
        COMMAND ${CMAKE_COMMAND}
            "-DDOL=${SHADOW_BOOT_DOL_PATH}"
            "-DPROGRAM=$<TARGET_FILE:shadow_boot_probe>"
            -DMODE=hle_hid0_request
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_fixture.cmake")
    add_test(NAME boot_pal_application_loop
        COMMAND ${CMAKE_COMMAND}
            "-DDOL=${SHADOW_BOOT_DOL_PATH}"
            "-DPROGRAM=$<TARGET_FILE:shadow_boot_application_loop_tests>"
            -DMODE=loop
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_fixture.cmake")
endif()
