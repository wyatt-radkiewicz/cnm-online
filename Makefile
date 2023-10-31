# Sources/build dirs
TARGET_NAME := cnmonline
BUILD_DIR := ./build
SOURCE_DIR := ./src
SRCS := $(wildcard $(SOURCE_DIR)/*.c)
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(SRCS)))

# Flags
CFLAGS := $(CFLAGS) $(shell sdl2-config --cflags) -I$(SOURCE_DIR)
SDL_LDFLAGS := $(shell sdl2-config --libs) -lSDL2_mixer -lSDL2_net
LDFLAGS := $(LDFLAGS) $(SDL_LDFLAGS)

# Build modes
debug: CFLAGS += -DDEBUG -g -O0
debug: $(BUILD_DIR)/$(TARGET_NAME)
release: CFLAGS += -O2
release: $(BUILD_DIR)/$(TARGET_NAME)
release_with_debug: CFLAGS += -g -DDEBUG -O2
release_with_debug: $(BUILD_DIR)/$(TARGET_NAME)

# Build commands
run: $(BUILD_DIR)/$(TARGET_NAME)
	$(BUILD_DIR)/$(TARGET_NAME)
$(BUILD_DIR)/$(TARGET_NAME): $(OBJS)
	mkdir -p $(dir $@)
	$(CC) $^ $(LDFLAGS) -o $@

define BUILD_SRC
$(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(1))): $(1) $(addprefix $(SOURCE_DIR)/,$(2))
	mkdir -p $$(dir $$@)
	$$(CC) $$(CFLAGS) $$(CPPFLAGS) -c $$< -o $$@
endef

$(foreach src,$(SRCS),$(eval $(call BUILD_SRC,$(src),$(shell grep "#include \"[:alnum:|.|\"]*" $(src) | awk -F ' ' '{print $$2 }' | awk -F '\"' '{print $$2 }'))))

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
