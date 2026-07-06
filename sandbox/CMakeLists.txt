add_library(ocpe_sandbox STATIC
    src/sandbox.cpp
    src/compiler.cpp
    src/runner.cpp
    src/language.cpp
)

target_include_directories(ocpe_sandbox PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ocpe_sandbox PUBLIC ocpe_shared)
