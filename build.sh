#!/bin/bash
set -e

# ============================================================================
# VitaSpot Build Script for macOS
# ============================================================================

echo "╔═════════════════════════════════════════╗"
echo "║   VitaSpot Build Script (macOS)        ║"
echo "╚═════════════════════════════════════════╝"
echo ""

# Check VITASDK
if [ -z "$VITASDK" ]; then
    echo "❌ ERROR: VITASDK environment variable not set"
    echo ""
    echo "Please set VITASDK:"
    echo "  export VITASDK=/usr/local/vitasdk"
    echo ""
    exit 1
fi

echo "✓ VITASDK: $VITASDK"

# Check vitasdk installation
if [ ! -f "$VITASDK/share/vita.toolchain.cmake" ]; then
    echo "❌ ERROR: vitasdk not properly installed"
    echo "   vita.toolchain.cmake not found at $VITASDK/share/"
    exit 1
fi

echo "✓ vitasdk toolchain found"

# Clean previous build
echo ""
echo "🧹 Cleaning previous build..."
rm -rf build/
mkdir build

# Configure CMake
echo ""
echo "🔧 Configuring CMake..."
cd build

cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$VITASDK" \
    2>&1 | head -20

# Build
echo ""
echo "🔨 Compiling VitaSpot..."
NPROCS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)
make -j$NPROCS

# Check output
if [ ! -f "VitaSpot.vpk" ]; then
    echo ""
    echo "❌ Build failed: VitaSpot.vpk not created"
    exit 1
fi

cd ..

# Summary
echo ""
echo "╔═════════════════════════════════════════╗"
echo "║      ✓ Build Successful!               ║"
echo "╚═════════════════════════════════════════╝"
echo ""
echo "📦 Output: build/VitaSpot.vpk"
ls -lh build/VitaSpot.vpk
echo ""
echo "Next steps:"
echo "  1. Transfer to PS Vita via FTP:"
echo "     ftp <vita_ip>:1337"
echo "     put build/VitaSpot.vpk ux0:data/"
echo ""
echo "  2. Or copy via USB to ux0:data/"
echo ""
echo "  3. Install via VitaShell"
echo ""
