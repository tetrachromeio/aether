#!/bin/bash

# Test script to verify that aeon dev now works like aeon build
# This script tests the fix for the build functionality difference

echo "🧪 Testing Aeon Dev vs Build Command Parity"
echo "============================================"

# Check if we're in the aether directory
if [ ! -f "aeon.toml" ]; then
    echo "❌ This script should be run from a project directory with aeon.toml"
    echo "💡 Try running from: aeon/aeon/samples/aether/"
    exit 1
fi

# Check if aeon is available
if ! command -v aeon &> /dev/null; then
    echo "❌ aeon command not found. Make sure aeon is installed and in PATH"
    exit 1
fi

echo "📍 Current directory: $(pwd)"
echo "📄 Found aeon.toml: ✅"

# Clean any existing builds
echo ""
echo "🧹 Cleaning existing builds..."
rm -rf build/
echo "✅ Cleaned build directory"

# Test build command
echo ""
echo "🔨 Testing 'aeon build' command..."
if aeon build; then
    echo "✅ aeon build: SUCCESS"
    BUILD_SUCCESS=true
else
    echo "❌ aeon build: FAILED"
    BUILD_SUCCESS=false
fi

# Check if binary was created
if [ -f "build/main" ]; then
    echo "✅ Build binary exists: build/main"
    BUILD_BINARY=true
else
    echo "❌ Build binary missing: build/main"
    BUILD_BINARY=false
fi

# Clean for dev test
echo ""
echo "🧹 Cleaning for dev test..."
rm -rf build/

# Test dev command build capability (we'll stop it quickly)
echo ""
echo "🔥 Testing 'aeon dev' build capability..."
echo "💡 We'll start dev server and stop it quickly to test build functionality"

# Start dev server in background and capture output
timeout 10s aeon dev &
DEV_PID=$!

# Wait a moment for build to happen
sleep 5

# Stop the dev server
kill $DEV_PID 2>/dev/null || true
wait $DEV_PID 2>/dev/null || true

# Check if dev created a binary
DEV_BINARY=false
if [ -d "build/dev" ]; then
    echo "✅ Dev build directory exists: build/dev"
    # Look for any dev binary
    DEV_BINARIES=$(find build/dev -name "dev_*" -type f -executable 2>/dev/null | wc -l)
    if [ "$DEV_BINARIES" -gt 0 ]; then
        echo "✅ Dev binary exists: $(find build/dev -name "dev_*" -type f -executable 2>/dev/null | head -1)"
        DEV_BINARY=true
    else
        echo "❌ No dev binary found in build/dev"
    fi
else
    echo "❌ Dev build directory missing: build/dev"
fi

# Summary
echo ""
echo "📊 TEST RESULTS SUMMARY"
echo "======================="
echo "aeon build success:     $BUILD_SUCCESS"
echo "aeon build binary:      $BUILD_BINARY"  
echo "aeon dev binary:        $DEV_BINARY"

if [ "$BUILD_SUCCESS" = true ] && [ "$BUILD_BINARY" = true ] && [ "$DEV_BINARY" = true ]; then
    echo ""
    echo "🎉 SUCCESS: Both aeon build and aeon dev can create binaries!"
    echo "✅ The dev command now uses the same build functionality as build command"
    exit 0
else
    echo ""
    echo "❌ ISSUES DETECTED:"
    if [ "$BUILD_SUCCESS" = false ]; then
        echo "   - aeon build command failed"
    fi
    if [ "$BUILD_BINARY" = false ]; then
        echo "   - aeon build didn't create binary"
    fi
    if [ "$DEV_BINARY" = false ]; then
        echo "   - aeon dev didn't create binary (this was the original issue)"
    fi
    exit 1
fi
