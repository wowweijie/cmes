file(GLOB SOURCES "*cc")
add_library(engine STATIC ${SOURCES})
target_include_directories(engine PUBLIC)
target_link_libraries(engine)
