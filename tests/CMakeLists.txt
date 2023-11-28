project(tests)

add_executable(${PROJECT_NAME}
        test_tokenizer.cpp

        test_parser.cpp
        test_fuzzing_1.cpp

        test_boolean.cpp
        test_eval.cpp
        test_integer.cpp
        test_list.cpp
        test_fuzzing_2.cpp

        test_symbol.cpp
        test_pair_mut.cpp
        test_control_flow.cpp
        test_lambda.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${PROJECT_SOURCE_DIR}
)

find_package(Catch2 3 REQUIRED)

target_link_libraries(${PROJECT_NAME}
        scheme
        allocations_checker
        Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
enable_testing()
catch_discover_tests(tests)