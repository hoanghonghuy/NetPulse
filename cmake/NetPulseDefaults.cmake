# =============================================================================
# NetPulseDefaults.cmake
#
# Canonical build settings for NetPulse. Keep NetPulse.vcxproj in sync with the
# values defined here (see comment block at the top of NetPulse.vcxproj).
# =============================================================================

# Preprocessor definitions (add WIN32 / _DEBUG / NDEBUG per configuration in VS)
set(NETPULSE_COMPILE_DEFINITIONS
    UNICODE
    _UNICODE
    _WIN32_WINNT=0x0601
    WIN32_LEAN_AND_MEAN
)

# Windows libraries required by the application (no unused entries)
set(NETPULSE_LINK_LIBRARIES
    Iphlpapi.lib
    shell32.lib
    advapi32.lib
    comctl32.lib
    Dwmapi.lib
    winhttp.lib
    uxtheme.lib
    ws2_32.lib
    psapi.lib
    tdh.lib
)

set(NETPULSE_INCLUDE_DIRECTORIES
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/resources
    ${CMAKE_SOURCE_DIR}/third_party/sqlite
)

function(netpulse_apply_compile_defaults target)
    target_compile_definitions(${target} PRIVATE ${NETPULSE_COMPILE_DEFINITIONS})

    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /MP
            /utf-8
            /wd4100
        )
    elseif(MINGW)
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        target_link_options(${target} PRIVATE -static -static-libgcc -static-libstdc++)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

function(netpulse_apply_link_defaults target)
    if(WIN32)
        target_link_libraries(${target} PRIVATE ${NETPULSE_LINK_LIBRARIES})
    endif()
endfunction()

function(netpulse_apply_sqlite_warning_defaults target)
    if(MSVC)
        set_source_files_properties(
            ${CMAKE_SOURCE_DIR}/third_party/sqlite/sqlite3.c
            PROPERTIES COMPILE_FLAGS "/wd4996"
        )
    endif()
endfunction()

function(netpulse_apply_strict_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /WX)
    endif()
endfunction()

function(netpulse_apply_common_defaults target)
    netpulse_apply_compile_defaults(${target})
    netpulse_apply_link_defaults(${target})
    netpulse_apply_sqlite_warning_defaults(${target})
endfunction()

function(netpulse_apply_production_defaults target)
    netpulse_apply_common_defaults(${target})
    netpulse_apply_strict_warnings(${target})
endfunction()
