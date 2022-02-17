aux_source_directory(${bound_checking_function_dir}/src lib_sec_source)
add_library(securec STATIC ${lib_sec_source})