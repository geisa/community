#!/bin/bash
#
# Yocto build system initiator.
#
# Copyright (C) 2019-2022 Savoir-faire Linux, Inc.
# This program is distributed under the Apache License 2.0.

# Name:       print_usage
# Brief:      Print script usage
print_usage()
{
cat <<EOF
This script builds a Yocto distribution

./$MYNAME [OPTIONS]

Build configuration options:
  -d, --dl-dir DIR           use DIR as downloads cache directory
  -i, --image IMGNAME        build a specific image
                               (default: $default_image)
      --distro DISTRO        build a specific distribution
                               (default: $default_distro)
  -m, --machine MACHINE      build a specific target machine
                               (default: $default_machine)
  -s, --sstate-dir DIR       use DIR as sstate-cache directory
                               (default: ($(basename $BUILDDIR)/sstate-cache/))
  -k, --sdk                  compile SDK (bitbake -c populate_sdk)
      --set-version-id VERSION   set an os-release field with a specific version string
                               (default: $(scripts/get-version -d --dirty))

Running custom build commands:
  -- CUSTOM_COMMAND          run custom command instead of default:
                              bitbake $default_image

Performance options:
  -t, --tasks NUM            run NUM parallel Bitbake tasks
                               (default: number of cores=$(nproc))
  -j, --jobs NUM             parallel jobs/threads per task
                               (default: number of cores=$(nproc))

Misc. options:
  -r, --remove-build-dir     remove build directory ($(basename $BUILDDIR)/)
      --no-layers-update     don't auto-update bblayers.conf
      --fail-fast            don't use bitbake's -k option
  -v, --verbose              ignored
  -q, --quiet                quiet mode (only log error messages from this script)
      --log-level LEVEL      minimum log level to print script messages (debug, info, warn, error, none)
      --debug                print all executed commands
  -h, --help                 display this help message
EOF
}

# Name:       apply_patch
# Brief:      Test and apply a patch file if needed
# Param[in]:  Patch file
apply_patch()
{
    local patch_file="$1"
    local done_flag=patches/"$patch_file".done

    local patch_name="$(basename "$1")"
    local patch_dir="$(dirname "$1")"

    [ -z "$patch_dir" ] && patch_dir="."

    # Check if patches are for previous versions of build.sh
    if [ "$patch_dir" = "." ] && grep -qE "diff.*a/sources/" "patches/$patch_file"; then
        log error "Fatal: patch $patch_file should now be located in the corresponding meta layer tree"
        log error "       For instance: patches/sources/meta-openembedded/0001-xyz.patch"
        return 1
    fi

    if [ ! -d "$patch_dir" ]; then
        log error "Fatal: cannot apply $patch_name, $patch_dir does not exist"
        return 1
    fi

    log info "Checking $patch_file..."
    local patch_full_path="$PWD/patches/$patch_file"

    # Try to apply patch using git am, git apply or patch
    git -C "$patch_dir" am "$patch_full_path" 2> /dev/null ||
        git -C "$patch_dir" apply "$patch_full_path" 2> /dev/null ||
            patch -d "$patch_dir" --strip=1 --force --input "$patch_full_path" > /dev/null

    # Patch failed or already applied
    if [ "$?" -ne "0" ]; then
        if [ -f "$done_flag" ]; then
            log info "Patch $patch_file already applied -- skipping"
            return 0
        else
            log error "Error when applying patch $patch_file"
            return 1
        fi
    fi

    log info "Successfully applied $patch_file"
    touch "$done_flag"
}

# Name:       parse_options
# Brief:      Parse options from command line
# Param[in]:  Command line parameters
parse_options()
{
    ARGS=$(getopt -o "d:i:j:khm:rs:t:vq" -l "distro:,dl-dir:,help,image:,jobs:,machine:,no-layers-update,fail-fast,debug,remove-build-dir,sdk,set-version-id:,sstate-dir:,tasks:,verbose,quiet,log-level:" -n "build.sh" -- "$@")

    # Bad arguments
    if [ $? -ne 0 ]; then
        exit 1
    fi

    eval set -- "$ARGS"
    ncpus=$(nproc)
    BITBAKE_CONTINUE="-k"

    while true; do
        case "$1" in
            --distro)
                export DISTRO="$2"
                shift 2
                ;;

            -d|--dl-dir)
                if [ ! -d "$2" ]; then
                    log error "Fatal: specified dl-dir does not exist"
                    exit 1
                fi
                export DL_DIR=$(readlink -f "$2")
                export BB_GENERATE_MIRROR_TARBALLS="1"
                shift 2
                ;;

            --debug)
                set -x
                shift
                ;;

            -i|--image)
                export IMAGE="$2"
                shift 2
                ;;

            --no-layers-update)
                NO_LAYERS_UPDATE=yes
                shift
                ;;

            --fail-fast)
                BITBAKE_CONTINUE=""
                shift
                ;;

            -m|--machine)
                export MACHINE="$2"
                shift 2
                ;;

            -r|--remove-build-dir)
                REMOVE_BUILDDIR=1
                shift
                ;;

            -k|--sdk)
                COMPILE_SDK=1
                shift
                ;;

            -s|--sstate-dir)
                if [ ! -d "$2" ]; then
                    log error "Fatal: specified state-dir does not exist"
                    exit 1
                fi
                export SSTATE_DIR=$(readlink -f "$2")
                shift 2
                ;;

            --set-version-id)
                export YOCTO_BUILD_VERSION="$2"
                shift 2
                ;;

            -j|--jobs)
                if [ "$2" -le 0 ] || [ "$2" -gt "$ncpus" ]; then
                    log error "Fatal: specified jobs=$2 is invalid, valid range is [1-$ncpus]"
                    exit 1
                fi
                export PARALLEL_MAKE="-j $2"
                shift 2
                ;;

            -t|--tasks)
                if [ "$2" -le 0 ] || [ "$2" -gt "$ncpus" ]; then
                    log error "Fatal: specified tasks=$2 is invalid, valid range is [1-$ncpus]"
                    exit 1
                fi
                export BB_NUMBER_THREADS=$2
                shift 2
                ;;

            --meta-list-file)
                export META_LIST_FILE=$(readlink -f "$2")
                shift 2
                ;;

            -v|--verbose)
                # ignored, for backwards compatibility purposes
                shift
                ;;

            -q|--quiet)
                VERBOSE=0
                shift
                ;;

            --log-level)
                case "$2" in
                debug|info|warn|error|none) ;;
                *)
                    log error "Fatal: specified log-level=$2 is invalid, valid options are [debug, info, warn, error, none]"
                    exit 1
                    ;;
                esac
                LOGLEVEL="$2"
                shift 2
                ;;

            -h|--help)
                print_usage
                exit 1
                shift
                break
                ;;

            -|--)
                shift
                CMD="$*"
                break
                ;;

            *)
                print_usage
                exit 1
                shift
                break
                ;;
        esac
    done
}

# Name:       run_cmd
# Brief:      Run given command with enhanced display and return code checked
# Param[in]:  Command prefix for logs
# Param[in]:  The command itself
run_cmd()
{
  local log_prefix=$1; shift
  log info "$log_prefix: $*"

  # Launch command
  eval $*

  # Check command result, exit on error
  assert $?
}

# Name        filter_layers_blocklist
# Brief       Filter out layers listed in layers.blocklist from stdin
filter_layers_blocklist()
{
    local filter
    local layer
    local sep

    # Build $filter, a |-separated list of layers to exclude
    if [ -s "$TOPDIR/layers.blocklist" ]; then
        while read -r layer; do
            [ -z "$layer" ] && continue

            if [ -d "$SOURCESDIR/$layer" ]; then
                filter+="${sep}${SOURCESDIR}/$layer"
                sep="|" # separator for 2nd and next items
            else
                log error "layers.blocklist entry not found: $layer"
            fi
        done < "$TOPDIR/layers.blocklist"

        filter="($filter)"
    fi

    if [ "$filter" ]; then
        # eg. "^(meta-foobar|meta-foobiz)$"
        grep -Ev "^$filter$"
    else
        cat
    fi
}

# Name        update_layers
# Brief       Add layers in bblayers.conf using bitbake-layers add-layer,
#             unless NO_LAYERS_UPDATE is set.
update_layers()
{
    local layers_to_add

    [ -n "$NO_LAYERS_UPDATE" ] && return 0

    layers_to_add=$(find "$SOURCESDIR"/meta-* \
        "$SOURCESDIR"/poky/meta-* \
        "${BUILDDIR}"/workspace \
        -type f \
        -path '*/conf/layer.conf' \
        -print0 2>/dev/null |
        xargs -0 -n1 dirname |
        xargs -n1 dirname |
        filter_layers_blocklist)

    if [ -n "$layers_to_add" ]; then
        run_cmd "update layers" "bitbake-layers add-layer $layers_to_add"
    fi
}

# Name        C
# Brief       Print colorized string
# arg1        color name (see below)
# arg2        string
C()
{
    local color="$1"; shift
    local text="$*"
    local nc='\033[0m'
    local c

    # Only colorize a few terminal types
    case "$TERM" in
    linux*|xterm*|screen|vt102) ;;
    *)
        echo "$@"
        return
        ;;
    esac

    case "$color" in
        gray)   c='\033[1;30m' ;;
        red)    c='\033[1;31m' ;;
        green)  c='\033[1;32m' ;;
        yellow) c='\033[1;33m' ;;
        blue)   c='\033[1;34m' ;;
        purple) c='\033[1;35m' ;;
        cyan)   c='\033[1;36m' ;;

        orange_bg) c='\033[48;2;255;165;0m'
    esac

    printf "${c}${text}${nc}"
}

# Name        log
# Brief       Provide message logging to the terminal
# arg1        Log level (debug, info, warn, error - default : info)
# arg2..      Message to print
log() {
    local level="$1"; shift
    local color="cyan"
    local uplevel
    local upname

    # Sanitize log level
    case "$level" in
    debug|info|warn|error) ;;
    *) level="info" ;;
    esac

    # Only print messages above LOGLEVEL
    case "$LOGLEVEL" in
    debug) ;;
    info)
        case "$level" in
        debug) return ;;
        esac
        ;;
    warn)
        case "$level" in
        debug|info) return ;;
        esac
        ;;
    error)
        case "$level" in
        debug|info|warn) return ;;
        esac
        ;;
    none)
        return ;;
    esac

    # Apply formatting
    case "$level" in
    debug) color="purple" ;;
    info) color="cyan" ;;
    warn) color="orange_bg" ;;
    error) color="red" ;;
    esac

    # level -> LEVEL
    uplevel=$(echo $level | tr '[:lower:]' '[:upper:]')
    upname=$(echo $MYNAME | tr '[:lower:]' '[:upper:]')

    echo $(C $color "[$upname $uplevel] $*")
}

# Name        assert
# Brief       Check $arg1 as return code, exit program if nonzero
# arg1        Return code to check
# arg2..      (optional) Message to print in case of error
assert() {
    local retcode="$1"; shift
    local message="$*"

    case "$retcode" in
    0) ;;
    *)
        [ "$message" ] && log error "Fatal: $message"
        exit $retcode
        ;;
    esac
}


##########################
########## MAIN ##########
##########################

#### Local vars ####
# Be verbose by default
export VERBOSE=1
export LOGLEVEL="debug"

# Keep directory to retrieve tools
MYNAME="$(basename $0)"
TOPDIR=$(dirname $(readlink -f "$0"))
BUILDDIR=$TOPDIR/build
SOURCESDIR=$TOPDIR/sources
POKYDIR=$(dirname $(find "$SOURCESDIR" -name "oe-init-build-env" -print -quit))

# Change to top directory
cd "$TOPDIR"

# Inject a version string to be used in os-release_%.bbappend
export YOCTO_BUILD_VERSION=${YOCTO_BUILD_VERSION:-$(scripts/get-version -d --dirty)}

# Check for Poky directory
if [ -z "$POKYDIR" ]; then
  log error "poky directory cannot be found"
  exit 1
fi

# build.conf contains defaults for image, machine and distro
if [ -f "$TOPDIR"/build.conf ]; then
  . "$TOPDIR"/build.conf
else
  log warn "No build.conf file found - Using defaults for image/machine/distro"
  default_image="core-image-minimal"
  default_machine="qemux86-64"
  default_distro="poky-tiny"
fi

# Parse options
parse_options "$@"

# Adjust LOGLEVEL for verbosity
if [ "$VERBOSE" = "0" ]; then
  LOGLEVEL="error"
fi

# Display VARIABLES
log debug "CMD = '$CMD'"
log debug "DL_DIR = '$DL_DIR'"
log debug "BB_GENERATE_MIRROR_TARBALLS" = "$BB_GENERATE_MIRROR_TARBALLS"
log debug "SSTATE_DIR = '$SSTATE_DIR'"
log debug "BB_NUMBER_THREADS = '$BB_NUMBER_THREADS'"
log debug "PARALLEL_MAKE = '$PARALLEL_MAKE'"
log debug "YOCTO_BUILD_VERSION = '$YOCTO_BUILD_VERSION'"

# Apply patches
patches_list="$(find patches/ -name "*.patch" -printf "%P\n" 2> /dev/null)"
if [ "$patches_list" ] ; then
    log info "Applying patches..."
    for patch in $patches_list; do
        apply_patch "$patch" || exit 1
    done
fi

# Set variable readable from command line
export BB_ENV_PASSTHROUGH_ADDITIONS="$BB_ENV_PASSTHROUGH_ADDITIONS \
  ACCEPT_FSL_EULA \
  BB_GENERATE_MIRROR_TARBALLS \
  BB_NUMBER_THREADS \
  DISTRO \
  DL_DIR \
  MACHINE \
  PARALLEL_MAKE \
  SSTATE_DIR \
  YOCTO_BUILD_VERSION \
"

# Set image to build, default to core-image-minimal
export IMAGE=${IMAGE:-"$default_image"}
export MACHINE=${MACHINE:-"$default_machine"}
export DISTRO=${DISTRO:-"$default_distro"}

# For NXP BSPs, auto-accept their EULA
export ACCEPT_FSL_EULA="1"

if [ -n "$REMOVE_BUILDDIR" ]; then
  # Clean directory
  run_cmd "Removing build directory" rm -Rf "$BUILDDIR"
elif [ -z "$NO_LAYERS_UPDATE" ]; then
    # Clean layers
    run_cmd "Removing bblayers.conf" rm -f "$BUILDDIR"/conf/bblayers.conf
fi

# Init poky build
set "$BUILDDIR"
. "$POKYDIR"/oe-init-build-env >/dev/null

# Add layers
update_layers

# Build Yocto
if [ -n "$CMD" ]; then
  run_cmd "Running command" "$CMD"
elif [ -z "$COMPILE_SDK" ]; then
  run_cmd "Building image" bitbake "$BITBAKE_CONTINUE" "$IMAGE"
else
  run_cmd "Building sdk" bitbake "$BITBAKE_CONTINUE" "$IMAGE" -c populate_sdk
fi

# Generate the documentation
if [ "$(type -p asciidoctor-pdf)" ]; then
  cd "$TOPDIR"
  asciidoctor-pdf -o build/tmp/deploy/README.pdf ./README.adoc
  if [ -f .sonar/README.adoc ]; then
    asciidoctor-pdf -o build/tmp/deploy/README_sonar.pdf .sonar/README.adoc
  fi
fi