add_executable(ocpe_api
    src/main.cpp
    src/server.cpp
)

target_include_directories(ocpe_api PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${httplib_SOURCE_DIR}
)

target_link_libraries(ocpe_api PRIVATE
    ocpe_shared
    ocpe_sandbox
)
