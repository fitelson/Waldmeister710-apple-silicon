#!/bin/bash
# Build and install Waldmeister on macOS/Apple Silicon
# Run this script from the Source directory

set -e

echo "=== Building Waldmeister for Apple Silicon ==="

# Force Darwin architecture
echo "ARCH=DARWIN" > .architecture

# Clean previous build
echo "Cleaning previous build..."
for subdir in lib/std lib/fast lib/prof lib/power lib/comp; do
    if [ -d "$subdir" ]; then
        rm -f "$subdir"/.depend "$subdir"/.version "$subdir"/*.o "$subdir"/*.d 2>/dev/null || true
    fi
done
rm -f bin/WaldmeisterII* bin/cce* 2>/dev/null || true

# Build the fast (optimized) version
echo "Building optimized version..."
make fast

# Check if build succeeded
if [ ! -f "bin/WaldmeisterII.fast" ]; then
    echo "ERROR: Build failed - binary not found"
    exit 1
fi

echo "Build successful!"

# Install globally
echo ""
echo "=== Installing to /usr/local/bin ==="

# Create /usr/local/bin if it doesn't exist
if [ ! -d "/usr/local/bin" ]; then
    echo "Creating /usr/local/bin (requires sudo)..."
    sudo mkdir -p /usr/local/bin
fi

# Copy the binary
echo "Installing waldmeister (requires sudo)..."
sudo cp bin/WaldmeisterII.fast /usr/local/bin/waldmeister
sudo chmod +x /usr/local/bin/waldmeister

# Verify installation
if command -v waldmeister &> /dev/null; then
    echo ""
    echo "=== Installation Complete ==="
    echo "Waldmeister installed successfully!"
    echo "You can now run: waldmeister"
    echo ""
    waldmeister -h 2>&1 | head -20 || true
else
    echo ""
    echo "Installation complete. You may need to add /usr/local/bin to your PATH."
    echo "Add this to your ~/.zshrc or ~/.bash_profile:"
    echo '  export PATH="/usr/local/bin:$PATH"'
fi
