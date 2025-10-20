.PHONY: all server client clean

all: server client

server:
	$(MAKE) -C TCP_Server
	@cp TCP_Server/server ./server
	@echo "Server executable: ./server"

client:
	$(MAKE) -C TCP_Client
	@cp TCP_Client/client ./client
	@echo "Client executable: ./client"

clean:
	$(MAKE) -C TCP_Server clean
	$(MAKE) -C TCP_Client clean
	rm -f server client

test: all
	@echo "Build completed successfully!"
	@echo "Server executable: ./server"
	@echo "Client executable: ./client"

