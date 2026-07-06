add_executable(ocpe_worker
    src/main.cpp
    src/worker.cpp
    src/evaluator.cpp
)

target_include_directories(ocpe_worker PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ocpe_worker PRIVATE
    ocpe_shared
    ocpe_sandbox
)
