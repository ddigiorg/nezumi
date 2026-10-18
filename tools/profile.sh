#!/bin/bash
# ==============================================================================
# tools/profile.sh
#
# Records a perf profile of an example (or any executable) and renders it as a
# flame graph SVG.
#
# Profile the `profile` preset: it is an optimized build that keeps debug
# symbols and frame pointers, so stacks unwind correctly with the default
# --call-graph fp. Build it first with `tools/build.sh --preset profile`.
#
# Usage: tools/profile.sh [options] <example|path> [-- <program arguments>]
#
#   <example>  name of an example, e.g. 01-renderer, looked up in
#              .build/<preset>/examples/bin/
#   <path>     path to any executable
#
# Options:
#   -h, --help                       Show this message
#   -p, --preset <name>              Build to look examples up in (default: profile)
#   -o, --output <path>              Output SVG (default: .profile/<name>-<time>.svg)
#   -f, --freq <hz>                  Sampling frequency (default: 999)
#   -c, --call-graph <fp|dwarf|lbr>  Stack unwinding method (default: fp)
#       --keep-data                  Keep the raw perf.data file next to the SVG
#
# Everything after -- is passed through to the program unchanged.
#
# Examples:
#
#   tools/profile.sh 01-renderer
#   tools/profile.sh --call-graph dwarf ./my_game -- --level 2
#
# Requires: linux, perf, perl, git
# ==============================================================================

SCRIPT_TAG="profile.sh"

set -euo pipefail

SCRIPT_PATH="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
source "$SCRIPT_PATH/helpers.sh"

# FlameGraph has no releases; pin a commit so output is reproducible
readonly FLAMEGRAPH_URL="https://github.com/brendangregg/FlameGraph.git"
readonly FLAMEGRAPH_REF="41fee1f99f9276008b7cd112fca19dc3ea84ac32"

# ------------------------------------------------------------------------------
# Parse Arguments
# ------------------------------------------------------------------------------

ARG_PRESET="profile"
ARG_TARGET=""
ARG_OUTPUT_PATH=""
ARG_FREQ="999"
ARG_CALL_GRAPH="fp"
ARG_KEEP_DATA=0
ARG_EXE_ARGS=()

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
        -o|--output)
            require_arg "$1" $#
            ARG_OUTPUT_PATH="$2"
            shift 2
            ;;
        -f|--freq)
            require_arg "$1" $#
            if ! [[ "$2" =~ ^[1-9][0-9]*$ ]]; then
                die "$SCRIPT_TAG" "--freq expects a positive integer, got: '$2'"
            fi
            ARG_FREQ="$2"
            shift 2
            ;;
        -c|--call-graph)
            require_arg "$1" $#
            case "$2" in
                fp|dwarf|lbr) ARG_CALL_GRAPH="$2" ;;
                *) die "$SCRIPT_TAG" "unknown call graph method: $2" "expected fp, dwarf or lbr" ;;
            esac
            shift 2
            ;;
        --keep-data)
            ARG_KEEP_DATA=1
            shift
            ;;
        --)
            # Everything after -- belongs to the program, including things that
            # look like our own options. Consume the separator and stop parsing.
            shift
            ARG_EXE_ARGS=("$@")
            break
            ;;
        -*)
            die "$SCRIPT_TAG" "unexpected option: $1" \
                              "run \`$SCRIPT_TAG --help\` for usage"
            ;;
        *)
            if [ -n "$ARG_TARGET" ]; then
                die "$SCRIPT_TAG" "only one program can be profiled, got '$ARG_TARGET' and '$1'"
            fi
            ARG_TARGET="$1"
            shift
            ;;
    esac
done

# ------------------------------------------------------------------------------
# Paths
# ------------------------------------------------------------------------------

EXAMPLES_BIN_PATH="$PROJECT_PATH/.build/$ARG_PRESET/examples/bin"
PROFILE_PATH="$PROJECT_PATH/.profile"
FLAMEGRAPH_PATH="$PROFILE_PATH/FlameGraph"

# ------------------------------------------------------------------------------
# list_examples
#
# Prints the names of the built examples for the current preset, one per line.
# ------------------------------------------------------------------------------

list_examples() {
    if [ -d "$EXAMPLES_BIN_PATH" ]; then
        find "$EXAMPLES_BIN_PATH" -mindepth 1 -maxdepth 1 -type f -perm -u+x \
            -exec basename {} \; | sort
    fi
}

# ------------------------------------------------------------------------------
# resolve_executable
#
# Turns ARG_TARGET (an example name or a path) into EXE_PATH.
# ------------------------------------------------------------------------------

EXE_PATH=""

# die_with_examples <message>
# Fails with a list of the examples available for the current preset.
die_with_examples() {
    local hints=("examples built for '$ARG_PRESET':")
    local name

    while IFS= read -r name; do
        hints+=("  $name")
    done < <(list_examples)

    if [ "${#hints[@]}" -eq 1 ]; then
        hints+=("  (none; run tools/build.sh --preset $ARG_PRESET)")
    fi

    die "$SCRIPT_TAG" "$1" "${hints[@]}"
}

resolve_executable() {
    if [ -z "$ARG_TARGET" ]; then
        die_with_examples "no program given"
    fi

    if [[ "$ARG_TARGET" == */* ]]; then
        EXE_PATH="$ARG_TARGET"
    else
        EXE_PATH="$EXAMPLES_BIN_PATH/$ARG_TARGET"
    fi

    if [ ! -f "$EXE_PATH" ] || [ ! -x "$EXE_PATH" ]; then
        die_with_examples "not an executable file: $EXE_PATH"
    fi
}

# ------------------------------------------------------------------------------
# check_environment
#
# Verifies that perf profiling is actually possible before spending time on a
# clone or a recording run.
# ------------------------------------------------------------------------------

check_environment() {
    if [ "$PLATFORM" != "linux" ]; then
        die "$SCRIPT_TAG" "perf is Linux-only (detected PLATFORM=$PLATFORM)"
    fi

    require_commands "$SCRIPT_TAG" perf perl git

    # perf_event_paranoid gates access to hardware counters for unprivileged
    # users. 3 blocks everything, 2 blocks kernel samples. Warn rather than
    # fail: a user-space-only profile is still useful, and the recording will
    # produce its own error if it truly cannot proceed.
    local paranoid_path="/proc/sys/kernel/perf_event_paranoid"
    if [ -r "$paranoid_path" ]; then
        local paranoid
        paranoid="$(cat "$paranoid_path")"
        if [ "$paranoid" -gt 1 ]; then
            warn "$SCRIPT_TAG" "perf_event_paranoid=$paranoid may block sampling" \
                               "Loosen it for this session with:" \
                               "sudo sysctl -w kernel.perf_event_paranoid=1"
        fi
    fi
}

# ------------------------------------------------------------------------------
# setup_flamegraph
#
# Clones Brendan Gregg's FlameGraph scripts into .profile/FlameGraph once, at
# the pinned commit. A clone that is missing its scripts (for example after an
# interrupted run) is removed and cloned again.
#
# URL: https://github.com/brendangregg/FlameGraph
# ------------------------------------------------------------------------------

flamegraph_ready() {
    [ -f "$FLAMEGRAPH_PATH/stackcollapse-perf.pl" ] && [ -f "$FLAMEGRAPH_PATH/flamegraph.pl" ]
}

setup_flamegraph() {
    if flamegraph_ready; then
        return 0
    fi

    if [ -e "$FLAMEGRAPH_PATH" ]; then
        warn "flamegraph" "removing incomplete clone: $FLAMEGRAPH_PATH"
        rm -rf "$FLAMEGRAPH_PATH"
    fi

    log "flamegraph" "cloning..."
    git clone --quiet "$FLAMEGRAPH_URL" "$FLAMEGRAPH_PATH" \
        || die "flamegraph" "clone failed: $FLAMEGRAPH_URL"
    git -C "$FLAMEGRAPH_PATH" checkout --quiet "$FLAMEGRAPH_REF" \
        || { rm -rf "$FLAMEGRAPH_PATH"; die "flamegraph" "could not check out $FLAMEGRAPH_REF"; }

    flamegraph_ready || die "flamegraph" "clone is missing its scripts: $FLAMEGRAPH_PATH"
    log "flamegraph" "ready"
}

# ------------------------------------------------------------------------------
# run_perf_record
#
# Runs the program under perf until it exits. A non-zero exit is reported but
# not treated as fatal: a crash partway through still leaves a usable profile of
# everything up to that point.
# ------------------------------------------------------------------------------

run_perf_record() {
    log "$SCRIPT_TAG" "recording (close the program to finish)..."

    local status=0
    perf record \
        --call-graph "$ARG_CALL_GRAPH" \
        -F "$ARG_FREQ" \
        -o "$PERF_DATA_PATH" \
        -- "$EXE_PATH" ${ARG_EXE_ARGS[@]+"${ARG_EXE_ARGS[@]}"} || status=$?

    if [ "$status" -ne 0 ]; then
        warn "$SCRIPT_TAG" "recording exited with status $status"
    fi

    if [ ! -s "$PERF_DATA_PATH" ]; then
        die "$SCRIPT_TAG" "no samples recorded" \
                          "Check kernel.perf_event_paranoid and that the run lasted long enough."
    fi
}

# ------------------------------------------------------------------------------
# render_flamegraph
#
# Folds the recorded stacks and renders the SVG.
#
# The pipeline runs without pipefail so that a truncated stack in perf script
# does not discard an otherwise good profile; the output file is size-checked
# afterwards instead.
# ------------------------------------------------------------------------------

render_flamegraph() {
    log "$SCRIPT_TAG" "rendering flame graph..."

    (
        set +o pipefail
        perf script -i "$PERF_DATA_PATH" 2>/dev/null \
            | perl "$FLAMEGRAPH_PATH/stackcollapse-perf.pl" \
            | perl "$FLAMEGRAPH_PATH/flamegraph.pl" \
                --title "$(basename "$EXE_PATH") ($ARG_PRESET)" \
                --width 1600 \
            > "$ARG_OUTPUT_PATH"
    )

    if [ ! -s "$ARG_OUTPUT_PATH" ]; then
        die "$SCRIPT_TAG" "flame graph is empty" \
                          "perf script may have produced no resolvable symbols."
    fi
}

# ------------------------------------------------------------------------------
# Main
# ------------------------------------------------------------------------------

resolve_executable
check_environment

mkdir -p "$PROFILE_PATH"

STAMP="$(basename "$EXE_PATH")-$(date +%Y%m%d-%H%M%S)"
ARG_OUTPUT_PATH="${ARG_OUTPUT_PATH:-$PROFILE_PATH/$STAMP.svg}"
PERF_DATA_PATH="$PROFILE_PATH/$STAMP.perf.data"

# Remove the raw data on any exit unless asked to keep it
if [ "$ARG_KEEP_DATA" -eq 0 ]; then
    trap 'rm -f "$PERF_DATA_PATH" "$PERF_DATA_PATH.old"' EXIT
fi

log "$SCRIPT_TAG" "program=$EXE_PATH"
log "$SCRIPT_TAG" "freq=${ARG_FREQ}Hz call-graph=$ARG_CALL_GRAPH"

setup_flamegraph
run_perf_record
render_flamegraph

log "$SCRIPT_TAG" "flame graph: $ARG_OUTPUT_PATH"
if [ "$ARG_KEEP_DATA" -eq 1 ]; then
    log "$SCRIPT_TAG" "perf data:   $PERF_DATA_PATH"
fi
