#!/bin/bash

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PREFIX="/usr/local"
LIBDIR="$PREFIX/lib/root_lang"
BINDIR="$PREFIX/bin"
PROFILE_SCRIPT="/etc/profile.d/root_lang.sh"

clear
echo -e "${BLUE}=========================================${NC}"
echo -e "${BLUE}    ROOT LANG INSTALLER                  ${NC}"
echo -e "${BLUE}=========================================${NC}"
echo


if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}[ERROR]${YELLOW} This script needs to be run as root ${NC}"
    echo
    echo
    exit 1
fi


SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"


cd "$SCRIPT_DIR"


echo -e "${YELLOW}[WARNING]${NC} This will install root_lang system-wide."
echo -e "The compiler will be available as ${GREEN}rtlc${NC}."
echo
read -p "Continue with installation? (Y/n): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]] && [[ ! -z $REPLY ]]; then
    echo "Installation cancelled."
    exit 0
fi

clear
echo -e "${BLUE}[INFO]${NC} Starting installation process..."


detect_package_manager() {
    if command -v pacman &> /dev/null; then
        echo "pacman"
    elif command -v apt &> /dev/null; then
        echo "apt"
    elif command -v yum &> /dev/null; then
        echo "yum"
    elif command -v dnf &> /dev/null; then
        echo "dnf"
    elif command -v zypper &> /dev/null; then
        echo "zypper"
    else
        echo "unknown"
    fi
}

PKG_MANAGER=$(detect_package_manager)
echo -e "${BLUE}[INFO]${NC} Detected package manager: $PKG_MANAGER"

install_dependencies() {
    case $PKG_MANAGER in
        pacman)
            echo -e "${BLUE}[INFO]${NC} Installing dependencies with pacman..."
            pacman -S --needed --noconfirm gcc make
            ;;
        apt)
            echo -e "${BLUE}[INFO]${NC} Updating package list and installing dependencies with apt..."
            apt update
            apt install -y build-essential
            ;;
        yum)
            echo -e "${BLUE}[INFO]${NC} Installing dependencies with yum..."
            yum groupinstall -y "Development Tools"
            ;;
        dnf)
            echo -e "${BLUE}[INFO]${NC} Installing dependencies with dnf..."
            dnf groupinstall -y "Development Tools"
            ;;
        zypper)
            echo -e "${BLUE}[INFO]${NC} Installing dependencies with zypper..."
            zypper install -y -t pattern devel_basis
            ;;
        *)
            echo -e "${RED}[ERROR]${NC} Unsupported package manager. Please install dependencies manually:"
            echo "  - GCC compiler (gcc)"
            echo "  - GNU Make (make)"
            exit 1
            ;;
    esac
}

check_dependencies() {
    clear
    echo -e "${BLUE}[INFO]${NC} Checking for required dependencies..."

    local missing_deps=()

    if ! command -v cc &> /dev/null && ! command -v gcc &> /dev/null; then
        missing_deps+=("gcc")
    fi

    if ! command -v make &> /dev/null; then
        missing_deps+=("make")
    fi

    if [ ${#missing_deps[@]} -ne 0 ]; then
        echo -e "${YELLOW}[WARNING]${NC} Missing dependencies: ${missing_deps[*]}"
        echo -e "${BLUE}[INFO]${NC} Installing missing dependencies..."
        install_dependencies
    else
        echo -e "${GREEN}[OK]${NC} All dependencies found."
    fi
}

check_dependencies

clear

echo -e "${BLUE}[INFO]${NC} Building the root_lang compiler..."
export PATH=/usr/bin:/bin:/usr/local/bin:$PATH

echo -e "${BLUE}[INFO]${NC} Cleaning up any build artifacts..."
make clean > /dev/null 2>&1 || true

make
if [ $? -ne 0 ]; then
    echo -e "${RED}[ERROR]${NC} Build failed."
    exit 1
fi

clear

if [ ! -f "bin/rootc" ]; then
    echo -e "${RED}[ERROR]${NC} Build failed. Compiler binary not found at bin/rootc"
    exit 1
fi

echo -e "${GREEN}[OK]${NC} Build successful."

clear

echo -e "${BLUE}[INFO]${NC} Installing compiler to $BINDIR..."
mkdir -p "$BINDIR"
cp bin/rootc "$BINDIR/rtlc"
chmod +x "$BINDIR/rtlc"

echo -e "${BLUE}[INFO]${NC} Installing runtime and standard library to $LIBDIR..."
mkdir -p "$LIBDIR/runtime"
mkdir -p "$LIBDIR/stdlib"
cp runtime/rootrt.c runtime/rootrt.h "$LIBDIR/runtime/"
cp stdlib/root.rtl "$LIBDIR/stdlib/"

echo -e "${BLUE}[INFO]${NC} Registering ROOTLANG_HOME for all users..."
tee "$PROFILE_SCRIPT" > /dev/null << EOF
# Added by the root_lang installer.
export ROOTLANG_HOME="$LIBDIR"
EOF
chmod 644 "$PROFILE_SCRIPT"

# Make it available in the current shell session too.
export ROOTLANG_HOME="$LIBDIR"

clear

echo -e "${GREEN}[SUCCESS]${NC} root_lang has been installed successfully!"
echo
echo -e "Compile a program with:  ${GREEN}rtlc filename.rtl${NC}"
echo -e "Then run it with:        ${GREEN}./filename${NC}"
echo
echo "The runtime and standard library live in:"
echo "  $LIBDIR"
echo
echo -e "In your programs, import the standard library with:"
echo -e "  ${BLUE}use(\"stdlib/root.rtl\") as std;${NC}"
echo
echo -e "${YELLOW}[NOTE]${NC} ROOTLANG_HOME was set in $PROFILE_SCRIPT."
echo -e "${YELLOW}[NOTE]${NC} Open a new terminal (or run 'source $PROFILE_SCRIPT')"
echo -e "        so the standard library resolves from anywhere."
echo
echo -e "${YELLOW}[NOTE]${NC} Make sure $BINDIR is in your PATH if it's not already."
