#!/bin/bash
# Script to help test the UDP DNS Resolver Client/Server

echo "=========================================="
echo "UDP DNS Resolver - Build and Test Script"
echo "=========================================="
echo ""

# Build everything
echo "Step 1: Building server and client..."
make clean
make all

if [ $? -ne 0 ]; then
    echo "Error: Build failed!"
    exit 1
fi

echo ""
echo "Build successful!"
echo ""
echo "=========================================="
echo "Setup complete! You can now test the application."
echo ""
echo "To run the server (in Terminal 1):"
echo "  cd UDP_Server"
echo "  ./server 5500"
echo ""
echo "To run the client (in Terminal 2):"
echo "  cd UDP_Client"
echo "  ./client 127.0.0.1 5500"
echo ""
echo "Test queries you can try:"
echo "  - google.com"
echo "  - 8.8.8.8"
echo "  - facebook.com"
echo "  - 1.1.1.1"
echo "  - (press Enter to quit)"
echo ""
echo "To view server logs:"
echo "  cat UDP_Server/log_20225905.txt"
echo "=========================================="

