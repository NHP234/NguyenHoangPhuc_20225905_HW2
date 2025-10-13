# Root Makefile for UDP DNS Resolver Client/Server
# Build both client and server

.PHONY: all client server clean

# Build both client and server
all: server client

# Build server
server:
	@echo "Building server..."
	@cd UDP_Server && $(MAKE)
	@echo "Server built successfully!"

# Build client
client:
	@echo "Building client..."
	@cd UDP_Client && $(MAKE)
	@echo "Client built successfully!"

# Clean both client and server
clean:
	@echo "Cleaning server..."
	@cd UDP_Server && $(MAKE) clean
	@echo "Cleaning client..."
	@cd UDP_Client && $(MAKE) clean
	@echo "Clean complete!"

# Rebuild everything
rebuild: clean all
