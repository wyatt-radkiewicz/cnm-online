# Sources/build dirs
TARGET_NAME := cnmonline
BUILD_DIR := ./build
SOURCE_DIR := ./src
SRCS := $(wildcard $(SOURCE_DIR)/*.c)
OBJS := $(addprefix $(BUILD_DIR)/,$(patsubst %.c,%.o,$(SRCS)))
#OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(SRCS)))

# Flags
CFLAGS := $(CFLAGS) $(shell sdl2-config --cflags) -I$(SOURCE_DIR)
SDL_LDFLAGS := $(shell sdl2-config --libs) -lSDL2_mixer -lSDL2_net
LDFLAGS := $(LDFLAGS) $(SDL_LDFLAGS) -lm

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
runrel: CFLAGS += -O2
runrel: $(BUILD_DIR)/$(TARGET_NAME)
	$(BUILD_DIR)/$(TARGET_NAME)
rundbg: CFLAGS += -DDEBUG -g -O0
rundbg: $(BUILD_DIR)/$(TARGET_NAME)
	$(BUILD_DIR)/$(TARGET_NAME)
$(BUILD_DIR)/$(TARGET_NAME): $(OBJS)
	mkdir -p $(dir $@)
	$(CC) $^ $(LDFLAGS) -o $@
tools_dbg:
	$(CC) $(SOURCE_DIR)/ibapply/ibapply.c -DDEBUG -g -O0 -o $(BUILD_DIR)/ibapply
tools:
	$(CC) $(SOURCE_DIR)/ibapply/ibapply.c -o $(BUILD_DIR)/ibapply

define BUILD_SRC
#$(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(1))): $(1) $(addprefix $(SOURCE_DIR)/,$(2))
#$$(warning $(BUILD_DIR)/$(dir $(1))$(2))
$$(BUILD_DIR)/$(dir $(1))$(2)
	mkdir -p $$(dir $$@)
	$$(CC) $$(CFLAGS) $$(CPPFLAGS) -c $(1) -o $$@
endef

#$(foreach src,$(SRCS),$(eval $(call BUILD_SRC,$(shell $(CC) -$(src),$(shell grep "#include \"[:alnum:|.|\"]*" $(src) | awk -F ' ' '{print $$2 }' | awk -F '\"' '{print $$2 }'))))
$(foreach src,$(SRCS),$(eval $(call BUILD_SRC,$(src),$(shell $(CC) $(CFLAGS) -M $(src) | tr -d '\n' | tr '\\' ' '))))

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
