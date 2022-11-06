SRCS := $(shell find ./ -name '*.c')
OBJS := $(patsubst %.c,%.o,$(SRCS))
TARGET := cnmonline
_FRAMEWORKS := SDL2 SDL2_net SDL2_mixer
FRAMEWORKS := $(addprefix -framework ,$(_FRAMEWORKS))

$(TARGET): $(OBJS)
	$(CC) $< -o $@ $(LDFLAGS) -F/Library/Frameworks $(FRAMEWORKS)

$(OBJS): $(SRCS)
	$(CC) $(CFLAGS) $(FRAMEWORKS) -c $< -o $@

.PHONY: clean
clean:
	rm -r $(OBJS) $(TARGET)
