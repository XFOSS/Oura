add_library(ourlang STATIC
    placeholder.cpp
)

set_target_properties(ourlang PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
