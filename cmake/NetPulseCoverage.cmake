# Optional coverage instrumentation for NetPulseTests (Phase 0 baseline tooling).

option(ENABLE_COVERAGE "Enable compiler coverage instrumentation for NetPulseTests" OFF)

function(netpulse_apply_test_coverage target)
    if(NOT ENABLE_COVERAGE)
        return()
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target} PRIVATE
            -fprofile-instr-generate
            -fcoverage-mapping
        )
        target_link_options(${target} PRIVATE -fprofile-instr-generate)
    elseif(MSVC)
        target_compile_options(${target} PRIVATE /Zi)
        target_link_options(${target} PRIVATE /DEBUG)
    endif()
endfunction()
