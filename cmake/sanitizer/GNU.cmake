target_compile_options(${PROJECT_NAME}
    PUBLIC
        "-fsanitize=address,undefined"  # sanitizers
        "-fno-omit-frame-pointer"       # address sanitizer flags
)

target_link_options(${PROJECT_NAME}
    PUBLIC
        "-fsanitize=address,undefined"
        "-fno-omit-frame-pointer"
)
