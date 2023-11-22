CC = gcc
CFLAGS = -Wall
BIN_DIR = bin

SERVER_SOURCES = Serveur/server2.c
CLIENT_SOURCES = Client/client2.c

SERVER_EXECUTABLE = $(BIN_DIR)/server2.exe
CLIENT_EXECUTABLE = $(BIN_DIR)/client2.exe

.PHONY: all clean

all: $(SERVER_EXECUTABLE) $(CLIENT_EXECUTABLE)

$(SERVER_EXECUTABLE): $(SERVER_SOURCES)
	$(CC) $(CFLAGS) -o $@ $<

$(CLIENT_EXECUTABLE): $(CLIENT_SOURCES)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(BIN_DIR)/*.exe
