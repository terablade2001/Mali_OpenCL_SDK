project (Common)
add_library (Common common.cpp image.cpp)
target_link_libraries(Common)
target_include_directories (Common PUBLIC include)