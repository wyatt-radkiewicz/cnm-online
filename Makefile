# Sources/build dirs
TARGET_NAME := cnmonline
BUILD_DIR := ./build
SRCS := $(wildcard *.c)
OBJS := $(addprefix $(BUILD_DIR)/,$(SRCS:%.c=%.o))

# Flags
CFLAGS := $(CFLAGS)
LDFLAGS := $(LDFLAGS)

# OS Specific
ifneq ($(OS),Windows_NT)
	UNAME := $(shell uname -s)
	ifeq ($(UNAME),Darwin)
		CFLAGS += $(shell sdl2-config --cflags)
		SDL_LDFLAGS := $(shell sdl2-config --libs)
		LDFLAGS += $(SDL_LDFLAGS) -lSDL2_mixer -lSDL2_net
    endif
else
	$(error Please use WSL or MINGW)
endif

# Build modes
debug: CFLAGS += -DDEBUG -g -O0
debug: $(BUILD_DIR)/$(TARGET_NAME)
release: CFLAGS += -O2
release: $(BUILD_DIR)/$(TARGET_NAME)

# Build commands
$(BUILD_DIR)/$(TARGET_NAME): $(OBJS)
	mkdir -p $(dir $@)
	$(CC) $^ $(LDFLAGS) -o $@

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $^ -o $@

.PHONY: run
run:
	$(BUILD_DIR)/$(TARGET_NAME)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
