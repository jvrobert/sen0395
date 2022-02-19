idf_component_register(SRCS "src/Sen0395.cpp"
                    INCLUDE_DIRS "include")

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)