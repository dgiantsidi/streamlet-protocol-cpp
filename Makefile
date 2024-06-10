
export CXX=g++

target: main

SRC_DIR=$(realpath .)/src
SOURCES:= $(SRC_DIR)/networking.cpp $(SRC_DIR)/block.cpp $(SRC_DIR)/crypto.cpp

CXX_FLAGS=-Wall -g -std=c++17
DEBUG_FLAGS=-fsanitize=address -O0

ifeq ($(DEBUG),1)
	CXX_FLAGS+=$(DEBUG_FLAGS)
endif

INCLUDES=-I$(SRC_DIR)/../include
LINKER_FLAGS=-lpthread -lfmt -ldl -lcrypto

main: clean
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(SOURCES) $@.cpp -o $@ $(LINKER_FLAGS)

clean:
	rm -rf main
