add_library(ourlang INTERFACE)
target_include_directories(ourlang INTERFACE ${CMAKE_CURRENT_LIST_DIR}/../include)

add_executable(ouro_lang main.cpp)
target_link_libraries(ouro_lang PRIVATE ourlang)

set_target_properties(ouro_lang PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
