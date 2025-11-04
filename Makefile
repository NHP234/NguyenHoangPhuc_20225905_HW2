all:
	cd TCP_Server && $(MAKE)
	cd TCP_Client && $(MAKE)
	@echo "Build completed! Run: ./server <port> and ./client <ip> <port>"

clean:
	cd TCP_Server && $(MAKE) clean
	cd TCP_Client && $(MAKE) clean
	rm -f server client

server:
	cd TCP_Server && $(MAKE)

client:
	cd TCP_Client && $(MAKE)

.PHONY: all clean server client

