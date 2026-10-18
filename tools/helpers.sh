# ==============================================================================
# tools/helpers.sh
#
# Common shared helpers for the scripts in tools/. Not executable on its own.
#
# Provides:
#   OS, ARCH, PLATFORM      host information
#   PROJECT_PATH            repository root
#   PRESETS                 configure presets the scripts accept
#   log / warn / die        message output
#   require_commands        check external tools exist
#   require_arg             check an option received a value
#   print_usage             print a script's header comment as help
#   normalize_preset        validate and lowercase a preset name
#   cpu_count               number of CPUs, portable across Linux and macOS
#
# Everything here works with macOS's default bash 3.2.
# ==============================================================================

# Guard against running this file directly
if [ "${BASH_SOURCE[0]}" == "${0}" ]; then
    echo "ERROR: helpers.sh is a library; source it instead of running it" >&2
    exit 1
fi

# ------------------------------------------------------------------------------
# Host and project information
# ------------------------------------------------------------------------------

OS="$(uname -s)"
ARCH="$(uname -m)"

case "$OS" in
    Linux*)               PLATFORM="linux"   ;;
    Darwin*)              PLATFORM="macos"   ;;
    MINGW*|MSYS*|CYGWIN*) PLATFORM="windows" ;;
    *)                    PLATFORM="unknown" ;;
esac

PROJECT_PATH="$(dirname "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)")"

if [ "$PLATFORM" == "windows" ]; then
    PROJECT_PATH="$(cygpath -m "$PROJECT_PATH")"
fi

# Must match the configure presets in CMakePresets.json
PRESETS="debug release profile"

# ------------------------------------------------------------------------------
# log / warn / die
#
# Single place that decides message format and output stream.
#
# Usage: log  <tag> <message>
#        warn <tag> <message> [hint...]   # hints are printed indented
#        die  <tag> <message> [hint...]
# ------------------------------------------------------------------------------

log() {
    echo "[$1] $2"
}

warn() {
    local tag=$1
    local message=$2
    shift 2

    echo "[$tag] WARNING: $message" >&2

    local hint
    for hint in "$@"; do
        echo "    $hint" >&2
    done
}

die() {
    local tag=$1
    local message=$2
    shift 2

    echo "[$tag] ERROR: $message" >&2

    local hint
    for hint in "$@"; do
        echo "    $hint" >&2
    done

    exit 1
}

# ------------------------------------------------------------------------------
# require_commands
#
# Verifies external tools exist before use.
#
# Usage: require_commands <tag> <cmd> [cmd...]
# ------------------------------------------------------------------------------

require_commands() {
    local tag=$1
    shift

    local cmd
    local missing=""

    for cmd in "$@"; do
        command -v "$cmd" &> /dev/null || missing="$missing $cmd"
    done

    if [ -n "$missing" ]; then
        die "$tag" "required command(s) not found on PATH:$missing"
    fi
}

# ------------------------------------------------------------------------------
# require_arg
#
# Verifies that an option which takes a value actually received one, so that
# `build.sh --preset` fails with a clear message rather than silently consuming
# the next option or expanding to an empty string.
#
# Usage: require_arg <option_name> <remaining_arg_count>
# ------------------------------------------------------------------------------

require_arg() {
    if [ "$2" -lt 2 ]; then
        die "$SCRIPT_TAG" "argument $1 requires a value" \
                          "run \`$SCRIPT_TAG --help\` for usage"
    fi
}

# ------------------------------------------------------------------------------
# print_usage
#
# Prints the leading comment block of a script as help text, stripping the
# leading "# " from each line. Keeping help text and file header as one thing
# means they cannot drift apart.
#
# Usage: print_usage <script_path>
# ------------------------------------------------------------------------------

print_usage() {
    sed -n '2,/^$/p' "$1" \
        | sed -e '/^[^#]/d' -e 's/^# \{0,1\}//' -e '/^=\{3,\}$/d'
}

# ------------------------------------------------------------------------------
# normalize_preset
#
# Accepts a preset name in any casing and echoes it in lowercase, or returns 1
# if it is not one of PRESETS.
#
# Usage: preset="$(normalize_preset <string>)" || die ...
# ------------------------------------------------------------------------------

normalize_preset() {
    local preset candidate
    preset="$(printf '%s' "$1" | tr '[:upper:]' '[:lower:]')"

    for candidate in $PRESETS; do
        if [ "$preset" == "$candidate" ]; then
            echo "$preset"
            return 0
        fi
    done

    return 1
}

# ------------------------------------------------------------------------------
# cpu_count
#
# Usage: jobs="$(cpu_count)"
# ------------------------------------------------------------------------------

cpu_count() {
    nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4
}
