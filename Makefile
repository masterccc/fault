CC = g++
CFLAGS = -std=c++17 -O2
TARGET = fault
SRC = fault.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
