project(scheme)

add_library(${PROJECT_NAME}
        src/tokenizer.cpp
        src/parser.cpp
        src/object.cpp
        src/scheme.cpp
)

target_include_directories(${PROJECT_NAME}
        PUBLIC ${PROJECT_SOURCE_DIR}/include
)