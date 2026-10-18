# ==============================================================================
# cmake/NezumiHelpers.cmake
#
# Helper functions for Nezumi's examples, tests, and for games using Nezumi.
# ==============================================================================

# Only process this file once, even if several CMakeLists.txt include it.
include_guard(GLOBAL)

# ------------------------------------------------------------------------------
# nezumi_copy_assets(<target> [ENGINE] [DIRS <dir>...])
#
# Copies asset folders into an `assets/` folder next to the built executable.
# The copy runs as a post-build step, i.e. whenever <target> is rebuilt.
#
#   ENGINE  Also copy Nezumi's own runtime files: the engine's assets/ folder
#           (shaders, graphics, ...) plus the third-party fonts from
#           thirdparty/fonts/, which end up in assets/fonts/. Use this in a
#           game when the engine loads files at runtime. Engine files are
#           copied first so a game can override any of them.
#   DIRS    Folders to copy. Defaults to the calling project's assets/ folder,
#           which is skipped silently when it does not exist.
#
# Resulting layout next to the executable:
#
#   assets/
#     shaders/   <- from Nezumi's assets/shaders/
#     graphics/  <- from Nezumi's assets/graphics/
#     fonts/     <- from Nezumi's thirdparty/fonts/ (fonts + their licenses)
#     ...        <- the game's own assets, copied last
#
# Examples:
#
#   nezumi_copy_assets(00-hello-world)                 # engine example
#   nezumi_copy_assets(my_game ENGINE)                 # game + engine assets
#   nezumi_copy_assets(my_game DIRS "${CMAKE_SOURCE_DIR}/data")
# ------------------------------------------------------------------------------

function(nezumi_copy_assets TARGET)
    # --------------------------------------------------------------------------
    # 1. Parse the optional arguments.
    #
    # cmake_parse_arguments(<prefix> <flags> <one-value keywords>
    #                       <multi-value keywords> <args to parse>)
    #
    # With prefix "ARG", this creates:
    #   ARG_ENGINE  TRUE if the caller wrote ENGINE, otherwise FALSE
    #   ARG_DIRS    the list of folders written after DIRS (empty if none)
    #
    # ARGN holds every argument after TARGET.
    # --------------------------------------------------------------------------
    cmake_parse_arguments(ARG "ENGINE" "" "DIRS" ${ARGN})

    # --------------------------------------------------------------------------
    # 2. Work out where the Nezumi source tree is.
    #
    # CMAKE_CURRENT_FUNCTION_LIST_DIR is the folder containing *this* file
    # (Nezumi's cmake/ folder), no matter which project calls the function.
    # Its parent is the Nezumi root. This keeps working when Nezumi lives
    # somewhere else, e.g. fetched with FetchContent into a game's build tree.
    # --------------------------------------------------------------------------
    cmake_path(GET CMAKE_CURRENT_FUNCTION_LIST_DIR PARENT_PATH engine_root)

    set(engine_assets "${engine_root}/assets")            # shaders, graphics, ...
    set(engine_fonts  "${engine_root}/thirdparty/fonts")  # external fonts + licenses

    # --------------------------------------------------------------------------
    # 3. Build the list of source folders to copy, in copy order.
    #
    # Order matters: later copies overwrite earlier ones, so the engine's
    # folder goes first and the game's folders can override engine files.
    # --------------------------------------------------------------------------
    set(asset_dirs "")

    # Engine assets first, if the caller asked for them.
    if(ARG_ENGINE)
        list(APPEND asset_dirs "${engine_assets}")
    endif()

    # Then either the folders the caller listed, or by default the calling
    # project's own assets/ folder (only if it exists).
    #
    # PROJECT_SOURCE_DIR is the root of the most recent project() call. For
    # Nezumi's own examples that is the Nezumi root, so this line is how the
    # examples pick up the engine assets even without the ENGINE flag.
    if(ARG_DIRS)
        list(APPEND asset_dirs ${ARG_DIRS})
    elseif(EXISTS "${PROJECT_SOURCE_DIR}/assets")
        list(APPEND asset_dirs "${PROJECT_SOURCE_DIR}/assets")
    endif()

    # If a folder ended up in the list twice (e.g. an engine example passed
    # ENGINE), only copy it once. The first occurrence is kept.
    list(REMOVE_DUPLICATES asset_dirs)

    # --------------------------------------------------------------------------
    # 4. Destination folder.
    #
    # $<TARGET_FILE_DIR:...> is a generator expression: CMake replaces it at
    # build time with the folder the executable is written to. This handles
    # multi-config generators (Visual Studio, Ninja Multi-Config) where the
    # folder differs per configuration, e.g. build/Debug vs build/Release.
    # --------------------------------------------------------------------------
    set(out_dir "$<TARGET_FILE_DIR:${TARGET}>/assets")

    # --------------------------------------------------------------------------
    # 5. Add one post-build copy step per source folder.
    #
    # add_custom_command(TARGET ... POST_BUILD) runs the command after the
    # target is linked. Commands run in the order they are added here, which
    # preserves the override order from step 3.
    #
    # `cmake -E copy_directory <src> <dst>` copies the *contents* of <src>
    # into <dst>, creating <dst> if needed and overwriting existing files.
    # --------------------------------------------------------------------------
    foreach(dir IN LISTS asset_dirs)
        # Fail at configure time with a clear message instead of at build time
        # with a confusing copy error.
        if(NOT IS_DIRECTORY "${dir}")
            message(FATAL_ERROR "nezumi_copy_assets(${TARGET}): not a directory: ${dir}")
        endif()

        add_custom_command(
            TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${dir}" "${out_dir}"
            COMMENT "Copying ${dir} to ${TARGET}"
            VERBATIM  # pass arguments to the shell exactly as written
        )

        # The engine also loads fonts from assets/fonts/ at runtime, but in
        # the source tree they live in thirdparty/fonts/ because they come
        # from an outside source. So whenever the engine's assets/ folder is
        # copied (via ENGINE or via an engine example's default), copy the
        # fonts right after it, into assets/fonts/.
        #
        # cmake_path(COMPARE) compares paths rather than raw strings, so
        # trivial differences like trailing slashes don't break the match.
        cmake_path(COMPARE "${dir}" EQUAL "${engine_assets}" is_engine_assets)

        if(is_engine_assets)
            add_custom_command(
                TARGET ${TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${engine_fonts}" "${out_dir}/fonts"
                COMMENT "Copying ${engine_fonts} to ${TARGET}"
                VERBATIM
            )
        endif()
    endforeach()
endfunction()
