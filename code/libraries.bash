#!/bin/bash

# -----------------------------------------------------------------------------
# Script for installing C/MPI/OpenMP development dependencies
# Compatible with Debian/Ubuntu, Fedora/RHEL, and Arch Linux distributions
# -----------------------------------------------------------------------------


set -e


if command -v apt-get &> /dev/null; then
    
    sudo apt-get update
    sudo apt-get install -y build-essential libopenmpi-dev

elif command -v dnf &> /dev/null; then
    
    sudo dnf install -y 'Development Tools' openmpi-devel

elif command -v pacman &> /dev/null; then
   
    sudo pacman -Syu --noconfirm base-devel openmpi
fi