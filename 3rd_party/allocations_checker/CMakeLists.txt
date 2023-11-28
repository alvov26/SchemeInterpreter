project(allocations_checker)

add_library(${PROJECT_NAME} STATIC
        allocations_checker.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
        ${PROJECT_SOURCE_DIR}
)
