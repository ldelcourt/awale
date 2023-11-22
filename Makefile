CC = gcc
CFLAGS = -Wall -Werror
SRC_DIR = .
SERVER_DIR = Serveur
CLIENT_DIR = Client
OBJ_DIR = obj
BIN_DIR = bin

SOURCES = $(wildcard $(SRC_DIR)/*.c)
SERVER_SOURCES = $(wildcard $(SERVER_DIR)/*.c)
CLIENT_SOURCES = $(wildcard $(CLIENT_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SOURCES))
SERVER_OBJECTS = $(patsubst $(SERVER_DIR)/%.c, $(OBJ_DIR)/%.o, $(SERVER_SOURCES))
CLIENT_OBJECTS = $(patsubst $(CLIENT_DIR)/%.c, $(OBJ_DIR)/%.o, $(CLIENT_SOURCES))
EXECUTABLE = $(BIN_DIR)/awale

.PHONY: all clean

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) $(SERVER_OBJECTS) $(CLIENT_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(SERVER_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(CLIENT_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ_DIR)/*.o
	rm -f $(EXECUTABLE)
