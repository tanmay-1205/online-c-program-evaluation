find_package(OpenSSL REQUIRED)

add_library(ocpe_shared STATIC
    src/config.cpp
    src/redis_client.cpp
    src/json_utils.cpp
    src/submission.cpp
    src/problem_loader.cpp
    src/auth.cpp
)

target_include_directories(ocpe_shared PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ocpe_shared PUBLIC
    hiredis
    Threads::Threads
    nlohmann_json::nlohmann_json
    OpenSSL::Crypto
)
