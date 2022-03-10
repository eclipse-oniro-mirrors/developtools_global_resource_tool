set(sqlite3_source
    ${sqlite3_dir}/src/sqlite3.c
)

add_library(sqlite3 STATIC ${sqlite3_source})
add_definitions(-DSQLITE_OMIT_LOAD_EXTENSION)
target_link_libraries(sqlite3 pthread m)