add_library(shadow_boot_foundation STATIC src/BootFoundation.cpp src/ApplicationLoop.cpp src/FprSeedProjection.cpp
    src/NativeBootPrefix.cpp
    src/NativeL2Prefix.cpp
    src/NativeCrtPrefix.cpp
    src/NativeBi2Prefix.cpp
    src/ConstructorTableProjection.cpp)
target_compile_features(shadow_boot_foundation PUBLIC cxx_std_17)
target_include_directories(shadow_boot_foundation PUBLIC include)

add_executable(shadow_boot_probe src/boot_probe.cpp)
target_link_libraries(shadow_boot_probe PRIVATE shadow_boot_foundation)

add_executable(shadow_boot_tests tests/test_boot.cpp)
target_link_libraries(shadow_boot_tests PRIVATE shadow_boot_foundation)

add_executable(shadow_boot_fpr_projection tools/fpr_projection_probe.cpp)
target_link_libraries(shadow_boot_fpr_projection PRIVATE shadow_boot_foundation)
add_executable(shadow_boot_native_prefix tools/native_prefix_probe.cpp)
target_link_libraries(shadow_boot_native_prefix PRIVATE shadow_boot_foundation)
add_executable(shadow_boot_native_prefix_tests tests/test_native_prefix.cpp)
target_link_libraries(shadow_boot_native_prefix_tests PRIVATE shadow_boot_foundation)
add_executable(shadow_boot_native_l2 tools/native_l2_probe.cpp)
target_link_libraries(shadow_boot_native_l2 PRIVATE shadow_boot_foundation)
add_executable(shadow_boot_native_l2_tests tests/test_native_l2.cpp)
target_link_libraries(shadow_boot_native_l2_tests PRIVATE shadow_boot_foundation)
add_executable(shadow_boot_native_crt tools/native_crt_probe.cpp)
target_link_libraries(shadow_boot_native_crt PRIVATE shadow_boot_foundation)
add_executable(shadow_boot_native_crt_tests tests/test_native_crt.cpp)
target_link_libraries(shadow_boot_native_crt_tests PRIVATE shadow_boot_foundation)
add_executable(shadow_boot_native_bi2 tools/native_bi2_probe.cpp)
target_link_libraries(shadow_boot_native_bi2 PRIVATE shadow_boot_foundation)
add_executable(shadow_boot_native_bi2_tests tests/test_native_bi2.cpp)
target_link_libraries(shadow_boot_native_bi2_tests PRIVATE shadow_boot_foundation)
add_executable(shadow_boot_fpr_seed_tests tests/test_fpr_seed_projection.cpp)
target_link_libraries(shadow_boot_fpr_seed_tests PRIVATE shadow_boot_foundation)

add_executable(shadow_boot_application_loop_tests tests/test_application_loop.cpp)
target_link_libraries(shadow_boot_application_loop_tests PRIVATE shadow_boot_foundation)

add_executable(shadow_boot_constructor_table_tests tests/test_constructor_table_projection.cpp)
target_link_libraries(shadow_boot_constructor_table_tests PRIVATE shadow_boot_foundation)
add_test(NAME boot_constructor_table_projection COMMAND shadow_boot_constructor_table_tests)

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
        add_test(NAME boot_rsp_packet_regression
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_rsp.py")
        add_test(NAME boot_l2_capture_regression
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_l2_capture.py")
        add_test(NAME boot_state_diff_regression
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_boot_state_diff.py")
        add_test(NAME boot_bi2_capture_provenance
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_bi2_tools.py")
        add_test(NAME boot_pal_semantic_recognizer
            COMMAND "${Python3_EXECUTABLE}" -B
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_recognizer.py"
                "${SHADOW_BOOT_DOL_PATH}")
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
        add_test(NAME boot_pal_raw_bi2_os_prefix
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_binary_note.py"
                "${SHADOW_BOOT_DOL_PATH}"
                "${CMAKE_CURRENT_SOURCE_DIR}/research/BINARY_BI2_OS_PREFIX_40.md"
                --expected-words 95 --expected-branches 19 --expected-sprs 4
                --required-code-range 80003140:80003148
                --required-code-range 80003188:80003264
                --required-code-range 80370BF0:80370C14
                --required-code-range 80370E68:80370EA4
                --required-code-range 80379628:80379628
                --required-code-range 80379648:80379668)
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
    add_test(NAME boot_pal_fpr_seed_projection
        COMMAND ${CMAKE_COMMAND}
            "-DDOL=${SHADOW_BOOT_DOL_PATH}"
            "-DPROGRAM=$<TARGET_FILE:shadow_boot_fpr_seed_tests>"
            -DMODE=fpr_seed
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_fixture.cmake")
    add_test(NAME boot_pal_native_sync_completion
        COMMAND ${CMAKE_COMMAND}
            "-DDOL=${SHADOW_BOOT_DOL_PATH}"
            "-DPROGRAM=$<TARGET_FILE:shadow_boot_native_prefix_tests>"
            -DMODE=native_sync
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_fixture.cmake")
    add_test(NAME boot_pal_native_l2_completion
        COMMAND ${CMAKE_COMMAND}
            "-DDOL=${SHADOW_BOOT_DOL_PATH}"
            "-DPROGRAM=$<TARGET_FILE:shadow_boot_native_l2_tests>"
            -DMODE=native_l2
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_fixture.cmake")
    add_test(NAME boot_pal_native_handler_crt
        COMMAND ${CMAKE_COMMAND}
            "-DDOL=${SHADOW_BOOT_DOL_PATH}"
            "-DPROGRAM=$<TARGET_FILE:shadow_boot_native_crt_tests>"
            -DMODE=native_crt
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_fixture.cmake")
    add_test(NAME boot_pal_native_bi2_os_prefix
        COMMAND ${CMAKE_COMMAND}
            "-DDOL=${SHADOW_BOOT_DOL_PATH}"
            "-DPROGRAM=$<TARGET_FILE:shadow_boot_native_bi2_tests>"
            -DMODE=native_bi2
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_fixture.cmake")
    add_test(NAME boot_pal_user_mode_decline
        COMMAND ${CMAKE_COMMAND}
            "-DDOL=${SHADOW_BOOT_DOL_PATH}"
            "-DPROGRAM=$<TARGET_FILE:shadow_boot_probe>"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_user_mode_decline.cmake")
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
