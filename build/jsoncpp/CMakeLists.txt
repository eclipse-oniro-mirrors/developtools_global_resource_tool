aux_source_directory(${jsoncpp_dir}/src/lib_json json_source)

add_library(jsoncpp STATIC ${json_source})