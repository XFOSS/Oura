enable_testing()

add_executable(lexer_tests lexer_tests.cc)
target_link_libraries(lexer_tests PRIVATE ourlang)
add_test(NAME LexerTests COMMAND lexer_tests)

add_executable(codegen_tests codegen_tests.cc)
target_link_libraries(codegen_tests PRIVATE ourlang)
add_test(NAME CodegenTests COMMAND codegen_tests)
