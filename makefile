CSOURCE = $(shell find . -name "*.c")
COBJS = $(patsubst */%.c,build/%.o,$(CSOURCE))

EXE := build/soundchip_demo
CFLAGS := -Wall -Wextra -pedantic -I./src
LDFLAGS := -lm -lSDL2

CC := gcc

.PHONY: all clean run

all: $(EXE)

run: $(EXE) $(CSOURCE)
	@echo -e "\nRunning ... " $< "\n"
	@$<

$(EXE): $(COBJS) | build
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ 

build/%.o: demo/%.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build:
	mkdir -p build
	echo "*" > build/.gitignore

clean: 
	rm -rf build

