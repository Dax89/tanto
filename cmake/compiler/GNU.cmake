target_compile_options(${PROJECT_NAME}
    PUBLIC
        "-Wall"                     # essential
        "-Wextra"                   # essential
        "-Werror"                   # essential
        "-Wpedantic"                # essential
        "-Wfloat-equal"             # because usually testing floating-point numbers for equality is bad
        "-Wwrite-strings"           # give string constants the type const char[length] so that copying the address of one into a non-const char * pointer will get a warning
        "-Wsign-compare"            # determines whether warnings are issued when a comparison between signed and unsigned values could produce an incorrect result
        "-Wpointer-arith"           # warn if anything depends upon the size of a function or of void.
        "-Wshadow"                  # warn whenever a local variable shadows another local variable, parameter or global variable
        "-Wunreachable-code"        # warn if the compiler detects that code will never be executed.
        "-Wduplicated-cond"         # warn about duplicated condition in if-else-if chains
        "-Wnull-dereference"        # warn when the compiler detects paths that dereferences a null pointer.
        "-Wdouble-promotion"        # warn when a value of type float is implicitly promoted to double
        # "-O2"                       # enable standard optimizations
        "-Wmaybe-uninitialized"
        "-Wuninitialized"
        "-Wunused"
        "-Wunused-parameter"
        "-Wunused-value"
        "-Wunused-variable"
        "-Wunused-local-typedefs"
        "-Wunused-but-set-parameter"
        "-Wunused-but-set-variable"
        "-g"
        "-ggdb"

        # Report only
        "-Wno-error=unused"
        "-Wno-error=unused-function"
        "-Wno-error=unused-parameter"
        "-Wno-error=unused-value"
        "-Wno-error=unused-variable"
        "-Wno-error=unused-local-typedefs"
        "-Wno-error=unused-but-set-parameter"
        "-Wno-error=unused-but-set-variable"
)

target_link_options(${PROJECT_NAME}
    PUBLIC
        "-fno-rtti"
)
