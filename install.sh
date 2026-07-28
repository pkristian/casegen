#!/bin/sh
#
# casegen installer — fetches build dependencies, clones into a temporary directory,
# builds, installs, and takes the clone with it on the way out.
#
#   sh install.sh                     install to /usr/local (sudo if needed)
#   sh install.sh --prefix ~/.local   install somewhere you already own
#   sh install.sh --no-deps           skip the package manager entirely
#
# Written for /bin/sh, and with no prompts anywhere, because the usual way to run this
# is piped from curl — where there is no terminal to answer a question with.

set -eu

REPO="https://github.com/pkristian/casegen.git"
PREFIX="/usr/local"
SKIP_DEPS=0


say()  { printf '==> %s\n' "$*"; }
warn() { printf 'warning: %s\n' "$*" >&2; }
die()  { printf 'error: %s\n' "$*" >&2; exit 1; }
have() { command -v "$1" > /dev/null 2>&1; }


# A heredoc rather than sed over this file's own header: piped from curl, $0 is "sh"
# and there is no file to read the header back out of.
usage()
{
    cat <<'EOF'
casegen installer — fetches build dependencies, clones into a temporary directory,
builds, installs, and takes the clone with it on the way out.

  --prefix DIR   where to install (default /usr/local)
  --no-deps      never touch the package manager
  -h, --help     this message
EOF
    exit "${1:-0}"
}


while [ $# -gt 0 ]; do
    case "$1" in
        --prefix)   [ $# -ge 2 ] || die "--prefix needs a directory"; PREFIX="$2"; shift 2 ;;
        --prefix=*) PREFIX="${1#--prefix=}"; shift ;;
        --no-deps)  SKIP_DEPS=1; shift ;;
        -h|--help)  usage 0 ;;
        *)          printf 'error: unknown option "%s"\n\n' "$1" >&2; usage 2 ;;
    esac
done


# --------------------------------------------------------------- privileges --

# Only escalate for the steps that actually need it, and work out which those are by
# testing, not by assuming that a prefix outside $HOME means root.
SUDO=""
if [ "$(id -u)" -ne 0 ]; then
    have sudo && SUDO="sudo" || SUDO=""
fi

can_write()
{
    # Writable if it exists and takes a write, or if it can be created at all.
    if [ -d "$1" ]; then
        [ -w "$1" ]
    else
        mkdir -p "$1" 2> /dev/null
    fi
}


# ------------------------------------------------------------- dependencies --

missing=""
have git   || missing="$missing git"
have cmake || missing="$missing cmake"
have make  || missing="$missing make"
have cc || have gcc || have clang || missing="$missing c-compiler"

if [ -n "$missing" ] && [ "$SKIP_DEPS" -eq 1 ]; then
    die "missing:$missing — and --no-deps was given, so nothing will be installed"
fi

if [ -n "$missing" ]; then
    say "missing:$missing"

    # Package names differ per distribution; the compiler comes with the toolchain
    # metapackage everywhere except macOS, where it comes with the Xcode tools.
    if   have apt-get; then INSTALL="apt-get install -y build-essential cmake git"; REFRESH="apt-get update"
    elif have dnf;     then INSTALL="dnf install -y gcc make cmake git";            REFRESH=""
    elif have yum;     then INSTALL="yum install -y gcc make cmake git";            REFRESH=""
    elif have pacman;  then INSTALL="pacman -S --needed --noconfirm base-devel cmake git"; REFRESH=""
    elif have zypper;  then INSTALL="zypper install -y gcc make cmake git";         REFRESH=""
    elif have apk;     then INSTALL="apk add build-base cmake git";                 REFRESH=""
    elif have brew;    then INSTALL="brew install cmake git";                       REFRESH=""
    else
        die "no supported package manager found — install$missing yourself, then re-run with --no-deps"
    fi

    # brew refuses to run under sudo, and manages its own prefix anyway.
    case "$INSTALL" in brew\ *) ESCALATE="" ;; *) ESCALATE="$SUDO" ;; esac

    # REFRESH is empty for every package manager but apt. Spelled as an if purely for
    # legibility — the `[ -n "$REFRESH" ] && { ...; }` form is safe here too, since a
    # test that fails before the last command of an AND-list does not trip `set -e`.
    if [ -n "$REFRESH" ]; then
        say "$ESCALATE $REFRESH"
        $ESCALATE $REFRESH
    fi

    say "$ESCALATE $INSTALL"
    $ESCALATE $INSTALL

    have cc || have gcc || have clang || die "still no C compiler after installing"
fi


# -------------------------------------------------------------------- build --

# Cleaning up on the way out is the whole point of building in a temporary directory,
# so the trap is set before the clone rather than after: an interrupt or a failed
# build leaves nothing behind either.
TMP="$(mktemp -d "${TMPDIR:-/tmp}/casegen-install.XXXXXX")"
trap 'rm -rf "$TMP"' EXIT INT TERM

say "cloning into $TMP"
git clone --depth 1 --quiet "$REPO" "$TMP/casegen"

# cmake and make between them produce a hundred lines of progress that nobody reading an
# installer wants. Keep it, though: on failure it is the only thing worth having.
LOG="$TMP/build.log"
run_quietly()
{
    if ! "$@" >> "$LOG" 2>&1; then
        printf '\n--- last 40 lines of %s ---\n' "$LOG" >&2
        tail -n 40 "$LOG" >&2
        die "$1 failed"
    fi
}

say "building"
run_quietly make -C "$TMP/casegen" build

say "installing to $PREFIX"
if can_write "$PREFIX/bin"; then
    run_quietly make -C "$TMP/casegen" install PREFIX="$PREFIX"
elif [ -n "$SUDO" ]; then
    run_quietly $SUDO make -C "$TMP/casegen" install PREFIX="$PREFIX"
else
    die "$PREFIX/bin is not writable and sudo is not available — try --prefix ~/.local"
fi


# ------------------------------------------------------------------ confirm --

BIN="$PREFIX/bin/casegen"
[ -x "$BIN" ] || die "installed, but $BIN is missing or not executable"

# Prove the thing runs before claiming success, and prove it does the one job it has.
got="$(printf 'installed ok\n' | "$BIN" -c screaming-snake - 2>&1)" \
    || die "$BIN did not run: $got"
[ "$got" = "INSTALLED_OK" ] || die "$BIN ran but produced \"$got\""

say "installed $BIN"

case ":${PATH}:" in
    *":$PREFIX/bin:"*) printf '    try: casegen --help\n' ;;
    *) warn "$PREFIX/bin is not on your PATH — add it, or run $BIN directly" ;;
esac
