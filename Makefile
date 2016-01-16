CXX = c++
CFLAGS += -std=c++11
LDFLAGS += $(shell pkg-config --libs sfml-all)

SRC = $(wildcard src/*.cpp)
HEADERS = $(wildcard src/*.h)
OBJ = $(SRC:.cpp=.o)

all: debug

release: CFLAGS += -O2
release: bin/release

debug: CFLAGS += -g -Wall -Wextra -Wpedantic
debug: bin/debug

bin/%: $(OBJ)
	@mkdir -p bin
	@echo "  LD    $@"
	@$(CXX) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

$(OBJ): %.o: %.cpp $(HEADERS)
	@echo "  C++   $@"
	@$(CXX) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf $(OBJ)

.PHONY: all clean
