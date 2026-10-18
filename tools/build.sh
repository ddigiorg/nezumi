#!/bin/bash
# ==============================================================================
# tools/build.sh
#
# Configures, builds and optionally tests the engine, examples and tests using
# the presets in CMakePresets.json. Dependencies are fetched by CMake during
# the first configure, so there is no separate dependency step.
#
# This script is a convenience wrapper. It is equivalent to:
#
#   cmake --preset debug
#   cmake --build --preset debug
#   ctest --preset debug
#
# Usage: tools/build.sh [options] [-- <extra cmake configure args>]
#
# Options:
#   -h, --help                Show this message
#   -p, --preset <name>       debug, release, or profile (default: debug)
#   -j, --jobs <N>            Max parallel compile jobs (default: CPU count)
#       --clean               Rebuild everything, keeping downloaded sources
#       --fresh               Delete the build tree first (re-downloads deps)
#       --configure-only      Configure (and fetch dependencies), skip build
#       --test                Run all tests after building
#       --headless            Run only tests that need no display
#
# Examples:
#
#   tools/build.sh
#   tools/build.sh --preset release --test
#   tools/build.sh -- -D NEZUMI_BUILD_TESTS=OFF
#
# Requires: cmake 3.25+
# ==============================================================================

SCRIPT_TAG="build.sh"

set -euo pipefail

SCRIPT_PATH="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
source "$SCRIPT_PATH/helpers.sh"

# ------------------------------------------------------------------------------
# Parse Arguments
# ------------------------------------------------------------------------------

ARG_PRESET="debug"
ARG_JOBS=""
ARG_CLEAN=0
ARG_FRESH=0
ARG_CONFIGURE_ONLY=0
ARG_TEST=0
ARG_TEST_LABEL=""
ARG_CMAKE_EXTRA=()

while [ $# -gt 0 ]; do
    # `--opt=value` is rewritten into `--opt value`
    if [[ "$1" == --*=* ]]; then
        set -- "${1%%=*}" "${1#*=}" "${@:2}"
    fi

    case "$1" in
        -h|--help)
            print_usage "${BASH_SOURCE[0]}"
            exit 0
            ;;
        -p|--preset)
            require_arg "$1" $#
            if ! ARG_PRESET="$(normalize_preset "$2")"; then
                die "$SCRIPT_TAG" "unknown preset: $2" "expected one of: $PRESETS"
            fi
            shift 2
            ;;
        -j|--jobs)
            require_arg "$1" $#
            if ! [[ "$2" =~ ^[1-9][0-9]*$ ]]; then
                die "$SCRIPT_TAG" "--jobs expects a positive integer, got: '$2'"
            fi
            ARG_JOBS="$2"
            shift 2
            ;;
        --clean)
            ARG_CLEAN=1
            shift
            ;;
        --fresh)
            ARG_FRESH=1
            shift
            ;;
        --configure-only)
            ARG_CONFIGURE_ONLY=1
            shift
            ;;
        --test)
            ARG_TEST=1
            shift
            ;;
        --headless)
            ARG_TEST=1
            ARG_TEST_LABEL="headless"
            shift
            ;;
        --)
            shift
            ARG_CMAKE_EXTRA=("$@")
            break
            ;;
        *)
            die "$SCRIPT_TAG" "unexpected argument: $1" \
                              "run \`$SCRIPT_TAG --help\` for usage"
            ;;
    esac
done

if [ "$ARG_CONFIGURE_ONLY" -eq 1 ] && [ "$ARG_TEST" -eq 1 ]; then
    die "$SCRIPT_TAG" "--configure-only and --test/--headless cannot be combined"
fi

# ------------------------------------------------------------------------------
# Main
# ------------------------------------------------------------------------------

require_commands "$SCRIPT_TAG" cmake ctest

# cmake --preset reads CMakePresets.json from the current directory
cd "$PROJECT_PATH"

BUILD_PATH="$PROJECT_PATH/.build/$ARG_PRESET"
export CMAKE_BUILD_PARALLEL_LEVEL="${ARG_JOBS:-${CMAKE_BUILD_PARALLEL_LEVEL:-$(cpu_count)}}"

log "$SCRIPT_TAG" "OS=$OS ($PLATFORM) ARCH=$ARCH"
log "$SCRIPT_TAG" "preset=$ARG_PRESET jobs=$CMAKE_BUILD_PARALLEL_LEVEL"
log "$SCRIPT_TAG" "build=$BUILD_PATH"

if [ "$ARG_FRESH" -eq 1 ]; then
    log "$SCRIPT_TAG" "removing $BUILD_PATH..."
    rm -rf "$BUILD_PATH"
fi

log "$SCRIPT_TAG" "configuring..."
# ${arr[@]+"${arr[@]}"} expands an empty array safely under `set -u` in bash 3.2
cmake --preset "$ARG_PRESET" ${ARG_CMAKE_EXTRA[@]+"${ARG_CMAKE_EXTRA[@]}"}

if [ "$ARG_CONFIGURE_ONLY" -eq 1 ]; then
    log "$SCRIPT_TAG" "configured ($ARG_PRESET)"
    exit 0
fi

log "$SCRIPT_TAG" "building..."
BUILD_ARGS=(--build --preset "$ARG_PRESET")
if [ "$ARG_CLEAN" -eq 1 ]; then
    BUILD_ARGS+=(--clean-first)
fi
cmake "${BUILD_ARGS[@]}"

if [ "$ARG_TEST" -eq 1 ]; then
    log "$SCRIPT_TAG" "testing${ARG_TEST_LABEL:+ (label: $ARG_TEST_LABEL)}..."
    if [ -n "$ARG_TEST_LABEL" ]; then
        ctest --preset "$ARG_PRESET" -L "$ARG_TEST_LABEL"
    else
        ctest --preset "$ARG_PRESET"
    fi
fi

log "$SCRIPT_TAG" "complete ($ARG_PRESET)"
