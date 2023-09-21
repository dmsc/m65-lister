CC=gcc
CFLAGS=-O2 -Wall

# Build folder
BUILD=build

all: $(BUILD)/m65

.PHONY: clean
clean:
	rm -f $(BUILD)/m65 $(BUILD)/gettab
	-rmdir $(BUILD)

src/tokens.h: $(BUILD)/gettab mac65.bin | $(BUILD)
	$< < mac65.bin > $@ || (rm -f $@ && false)

# Used to generate tokens.h
mac65.bin:

# Dependencies
$(BUILD)/gettab: src/gettab.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD)/m65: src/m65.c src/tokens.h | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD):
	mkdir -p $(BUILD)
