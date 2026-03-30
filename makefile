CSOURCE = demo/demo.c demo/piano_ui.c src/soundchip.c 

EXE := build/soundchip_demo
CFLAGS := -Wall -Wextra -pedantic -I./src
LDFLAGS := -lm -lSDL2

CC := gcc

.PHONY: all clean run

all: $(EXE)

run: $(EXE) $(CSOURCE)
	@echo -e "\nRunning ... " $< "\n"
	@$<

$(EXE): $(CSOURCE) | build
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

build:
	mkdir -p build
	echo "*" > build/.gitignore

clean: 
	rm -rf build

