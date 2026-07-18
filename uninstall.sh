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
echo -e "${BLUE}    ROOT LANG UNINSTALLER                ${NC}"
echo -e "${BLUE}=========================================${NC}"
echo



if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}[ERROR]${YELLOW} This script needs to be run as root ${NC}"
    echo
    echo
    exit 1
fi


echo -e "${YELLOW}[WARNING]${NC} This will remove root_lang from your system."
echo
read -p "Continue with uninstallation? (Y/n): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]] && [[ ! -z $REPLY ]]; then
    echo "Uninstallation cancelled."
    exit 0
fi

echo
echo -e "${BLUE}[INFO]${NC} Starting uninstallation process..."


echo -e "${BLUE}[INFO]${NC} Removing compiler..."
rm -f "$BINDIR/rtlc"


echo -e "${BLUE}[INFO]${NC} Removing runtime and standard library..."
rm -rf "$LIBDIR"


echo -e "${BLUE}[INFO]${NC} Removing environment registration..."
rm -f "$PROFILE_SCRIPT"

echo
echo -e "${GREEN}[SUCCESS]${NC} root_lang has been uninstalled successfully!"
echo
echo -e "${YELLOW}[NOTE]${NC} ROOTLANG_HOME may still be set in your current shell."
echo -e "        It will be gone in new terminals."
