CC = g++
CFLAGS = -g
CFLAGS_PROD = -static
EXE = main.exe

all: $(EXE)

main.exe: main.cpp node.cpp
	$(CC) $(CFLAGS) $^ -o $@

prod:
	$(CC) $(CFLAGS_PROD) main.cpp node.cpp -o main.exe

.phony: clean
clean:
	del $(EXE)
