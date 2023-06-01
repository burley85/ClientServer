CXX       := gcc
CXX_FLAGS := -ggdb

BIN     := bin
SRC     := src
INCLUDE := include

LIBRARIES   := -lwsock32
EXECUTABLE  := server


all: $(BIN)/$(EXECUTABLE)

run: clean all
	clear
	./$(BIN)/$(EXECUTABLE)

$(BIN)/$(EXECUTABLE): $(SRC)/*.c
	$(CXX) $(CXX_FLAGS) -I$(INCLUDE) $^ -o $@ $(LIBRARIES)

clean:
	-rm $(BIN)/*