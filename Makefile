# Root Makefile for UDP DNS Resolver Client/Server
# Build both client and server and copy to root directory

.PHONY: all client server clean

# Build both client and server
all: server client

# Build server and copy to root
server:
	@echo "Building server..."
	@cd UDP_Server && $(MAKE)
	@cp UDP_Server/server ./server
	@echo "Server built successfully! (./server)"

# Build client and copy to root
client:
	@echo "Building client..."
	@cd UDP_Client && $(MAKE)
	@cp UDP_Client/client ./client
	@echo "Client built successfully! (./client)"

# Clean both client and server
clean:
	@echo "Cleaning..."
	@cd UDP_Server && $(MAKE) clean
	@cd UDP_Client && $(MAKE) clean
	@rm -f ./server ./client
	@echo "Clean complete!"

# Rebuild everything
rebuild: clean all
