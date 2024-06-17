# Makefile for compiling GTK application client.c and server.c with SQLite

# Compiler
CC = gcc

# Compiler flags
CFLAGS = $(shell pkg-config --cflags gtk+-3.0)

# Linker flags for GTK and pthread
LIBS_GTK = $(shell pkg-config --libs gtk+-3.0)
LIBS_THREAD = -lpthread

# Target executables
CLIENT_TARGET = client
SERVER_TARGET = server

# Source files
CLIENT_SRC = client.c
SERVER_SRC = server.c

# Default target
all: $(CLIENT_TARGET) $(SERVER_TARGET)

# Linking rules
$(CLIENT_TARGET): $(CLIENT_SRC)
	$(CC) $(CFLAGS) $^ $(LIBS_GTK) $(LIBS_THREAD) -o $@ -g

$(SERVER_TARGET): $(SERVER_SRC)
	$(CC) $(SERVER_SRC) -o $@ -lsqlite3 $(LIBS_THREAD) -g

# Clean rule
clean:
	rm -f $(CLIENT_TARGET) $(SERVER_TARGET)
