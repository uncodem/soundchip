CSOURCE = $(wildcard src/*.c)
COBJS = $(patsubst src/%.c,build/%.o,$(CSOURCE))

EXE := build/soundchip
CFLAGS := -Wall -Wextra -pedantic
LDFLAGS := -lm -lSDL2

CC := gcc

.PHONY: all clean run

all: $(EXE)

run: $(EXE) $(CSOURCE)
	@echo -e "\nRunning ... " $< "\n"
	@$<

$(EXE): $(COBJS) | build
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ 

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build:
	mkdir -p build
	echo "*" > build/.gitignore

clean: 
	rm -rf build

