/*
 * Attention: automatically generated.
 */
#ifndef COMMON__NCBI_PACKAGE_VER__H
#define COMMON__NCBI_PACKAGE_VER__H

#define NCBI_PACKAGE                       1
#define NCBI_PACKAGE_NAME                  "public"
#define NCBI_PACKAGE_VERSION_MAJOR         12
#define NCBI_PACKAGE_VERSION_MINOR         0
#define NCBI_PACKAGE_VERSION_PATCH         0
#define NCBI_PACKAGE_CONFIG                ""

#define NCBI_PACKAGE_VERSION_STRINGIFY(x)  #x
#define NCBI_PACKAGE_VERSION_COMPOSE_STR(a, b, c)  \
    NCBI_PACKAGE_VERSION_STRINGIFY(a) "."          \
    NCBI_PACKAGE_VERSION_STRINGIFY(b) "."          \
    NCBI_PACKAGE_VERSION_STRINGIFY(c)

#define NCBI_PACKAGE_VERSION          \
    NCBI_PACKAGE_VERSION_COMPOSE_STR  \
    (NCBI_PACKAGE_VERSION_MAJOR,      \
     NCBI_PACKAGE_VERSION_MINOR,      \
     NCBI_PACKAGE_VERSION_PATCH)

#endif  /* COMMON__NCBI_PACKAGE_VER__H */
