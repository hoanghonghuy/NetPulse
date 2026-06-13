# =============================================================================
# NetPulseResources.cmake
#
# MinGW/llvm-windres treats UTF-8 literals as Latin-1 unless they use the L""
# prefix and windres is invoked with --codepage=65001. Generate a MinGW-safe
# resource script before compiling resources.
# =============================================================================

function(netpulse_prepare_mingw_resource_script generated_rc)
    set(prepare_script "${CMAKE_SOURCE_DIR}/scripts/prepare-mingw-rc.ps1")

    execute_process(
        COMMAND powershell -NoProfile -ExecutionPolicy Bypass
            -File ${prepare_script}
            -InputPath "${CMAKE_SOURCE_DIR}/resources/app.rc"
            -OutputPath ${generated_rc}
        RESULT_VARIABLE prepare_rc_result
    )

    if(NOT prepare_rc_result EQUAL 0)
        message(FATAL_ERROR "Failed to prepare MinGW resource script: ${generated_rc}")
    endif()

    add_custom_command(
        OUTPUT ${generated_rc}
        COMMAND powershell -NoProfile -ExecutionPolicy Bypass
            -File ${prepare_script}
            -InputPath "${CMAKE_SOURCE_DIR}/resources/app.rc"
            -OutputPath ${generated_rc}
        DEPENDS
            "${CMAKE_SOURCE_DIR}/resources/app.rc"
            ${prepare_script}
        COMMENT "Generating MinGW-safe resource script with Unicode escapes"
        VERBATIM
    )
endfunction()

function(netpulse_setup_resource_script)
    set(NETPULSE_APP_RC "${CMAKE_SOURCE_DIR}/resources/app.rc" PARENT_SCOPE)

    if(MINGW)
        set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} --codepage=65001" PARENT_SCOPE)
        set(generated_rc "${CMAKE_BINARY_DIR}/generated/app.mingw.rc")
        netpulse_prepare_mingw_resource_script(${generated_rc})

        add_custom_target(netpulse_resources DEPENDS ${generated_rc})
        set(NETPULSE_APP_RC ${generated_rc} PARENT_SCOPE)
        set(NETPULSE_RESOURCES_TARGET netpulse_resources PARENT_SCOPE)
    endif()
endfunction()

function(netpulse_add_resource_dependencies target)
    if(MINGW AND TARGET netpulse_resources)
        add_dependencies(${target} netpulse_resources)
    endif()
endfunction()
