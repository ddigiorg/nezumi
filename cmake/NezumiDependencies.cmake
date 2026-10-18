# ==============================================================================
# cmake/NezumiDependencies.cmake
#
# Fetches and configures every third-party dependency with FetchContent.
#
# Sources are downloaded once per build directory into `.build/<preset>/_deps`
# and reused on later configures.
#
# Every dependency is pulled in as SYSTEM so the engine's -Wall -Wextra do not
# report warnings from third-party headers.
#
# Overriding a dependency (e.g. to test a local SDL checkout):
#   cmake --preset debug -D FETCHCONTENT_SOURCE_DIR_SDL3=/path/to/SDL
# ==============================================================================

include(FetchContent)

# Everything happens inside a function so the option variables set below stay
# local to the dependency configuration and never leak into a game project
# that adds Nezumi with add_subdirectory or FetchContent.
function(nezumi_fetch_dependencies)

    # Third-party code is always built as static libraries. This removes the
    # need to copy DLLs next to executables and keeps Debug and Release builds
    # self-contained.
    set(BUILD_SHARED_LIBS OFF)

    # FreeType and GLM declare old CMake minimums, under which option() ignores
    # the plain variables set below. Force the modern behaviour so the
    # FT_DISABLE_* and GLM_BUILD_* settings actually take effect.
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

    # --------------------------------------------------------------------------
    # SDL3 3.4.2 (static)
    # --------------------------------------------------------------------------

    set(SDL_SHARED       OFF)
    set(SDL_STATIC       ON)
    set(SDL_TEST_LIBRARY OFF)
    set(SDL_TESTS        OFF)
    set(SDL_EXAMPLES     OFF)
    set(SDL_INSTALL      OFF)

    FetchContent_Declare(SDL3
        URL      https://github.com/libsdl-org/SDL/releases/download/release-3.4.2/SDL3-3.4.2.tar.gz
        URL_HASH SHA256=ef39a2e3f9a8a78296c40da701967dd1b0d0d6e267e483863ce70f8a03b4050c
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        SYSTEM
    )

    # --------------------------------------------------------------------------
    # FreeType 2.13.3 (static)
    #
    # Every optional dependency is disabled to avoid link errors against
    # whatever happens to be installed on the build machine.
    # --------------------------------------------------------------------------

    set(FT_DISABLE_PNG      TRUE)
    set(FT_DISABLE_HARFBUZZ TRUE)
    set(FT_DISABLE_BZIP2    TRUE)
    set(FT_DISABLE_ZLIB     TRUE)
    set(FT_DISABLE_BROTLI   TRUE)
    set(SKIP_INSTALL_ALL    TRUE)

    FetchContent_Declare(freetype
        GIT_REPOSITORY https://github.com/freetype/freetype.git
        GIT_TAG        VER-2-13-3
        GIT_SHALLOW    TRUE
        SYSTEM
    )

    # --------------------------------------------------------------------------
    # GLM 1.0.1 (header-only)
    # --------------------------------------------------------------------------

    set(GLM_BUILD_LIBRARY OFF)
    set(GLM_BUILD_TESTS   OFF)
    set(GLM_BUILD_INSTALL OFF)

    FetchContent_Declare(glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        1.0.1
        GIT_SHALLOW    TRUE
        SYSTEM
    )

    # --------------------------------------------------------------------------
    # stb (header-only, no releases; pinned to a commit)
    # --------------------------------------------------------------------------

    FetchContent_Declare(stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG        2c980bb59875b0d32144a71867fbdebb2f77cd20
        SYSTEM
    )

    # --------------------------------------------------------------------------
    # FastNoiseLite 1.1.1 (header-only)
    # --------------------------------------------------------------------------

    FetchContent_Declare(fastnoiselite
        GIT_REPOSITORY https://github.com/Auburn/FastNoiseLite.git
        GIT_TAG        v1.1.1
        GIT_SHALLOW    TRUE
        SYSTEM
    )

    FetchContent_MakeAvailable(SDL3 freetype glm stb fastnoiselite)

    # Expose the source directories of the header-only repos (which have no
    # CMakeLists.txt) to the parent scope so targets can be created below.
    set(NEZUMI_STB_DIR           "${stb_SOURCE_DIR}"               PARENT_SCOPE)
    set(NEZUMI_FASTNOISELITE_DIR "${fastnoiselite_SOURCE_DIR}/Cpp" PARENT_SCOPE)

endfunction()

nezumi_fetch_dependencies()

# ------------------------------------------------------------------------------
# FreeType
#
# FreeType's own build creates a target called `freetype` but only provides the
# `Freetype::Freetype` name through its installed config file. Add the alias so
# the engine uses the same name in-tree as it would with find_package.
# ------------------------------------------------------------------------------

if(NOT TARGET Freetype::Freetype)
    add_library(Freetype::Freetype ALIAS freetype)
endif()

# ------------------------------------------------------------------------------
# GLAD
#
# The OpenGL 4.6 Core loader is generated once with glad2 2.0.8 and committed
# under thirdparty/glad. Its output never changes for a given API version, so
# vendoring it removes Python from the build requirements entirely.
#
# To regenerate:
#   pip install glad2==2.0.8
#   python -m glad --api gl:core=4.6 --out-path thirdparty/glad c --loader
# ------------------------------------------------------------------------------

set(NEZUMI_GLAD_DIR "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/glad")

find_package(OpenGL REQUIRED)

add_library(nezumi_glad STATIC "${NEZUMI_GLAD_DIR}/src/gl.c")
target_include_directories(nezumi_glad SYSTEM PUBLIC "${NEZUMI_GLAD_DIR}/include")
target_link_libraries(nezumi_glad PRIVATE OpenGL::GL)

# ------------------------------------------------------------------------------
# stb
# ------------------------------------------------------------------------------

add_library(nezumi_stb INTERFACE)
target_include_directories(nezumi_stb SYSTEM INTERFACE "${NEZUMI_STB_DIR}")

# ------------------------------------------------------------------------------
# FastNoiseLite
# ------------------------------------------------------------------------------

add_library(nezumi_fastnoiselite INTERFACE)
target_include_directories(nezumi_fastnoiselite SYSTEM INTERFACE "${NEZUMI_FASTNOISELITE_DIR}")
