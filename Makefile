ALL_CSRCS:= snake.c snake_game.c
ALL_COBJS:= $(ALL_CSRCS:.c=.o)
ALL_CDEPS:= $(ALL_CSRCS:.c=.d)

CC:=gcc
CFLAGS:=
LDFLAGS:=

.PHONY: all
all: snake

.PHONY: snake
snake: $(ALL_COBJS)
	@echo "Linking $@ ..."
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

%.d:%.c
	@echo "Making dependencies for $(notdir $<) ..."
	@echo -n "$@ " > $@
	@$(CC) -M -E $(CFLAGS) $< >> $@

%.o: %.c
	@echo "Compiling $@ ..."
	@$(CC) $(CFLAGS) $(LDFLAGS) -c $< -o $@

ifneq ($(MAKECMDGOALS),clean)
-include $(ALL_CDEPS)
endif

.PHONY: clean
clean:
	rm -f $(ALL_CDEPS)
	rm -f $(ALL_COBJS)
	rm -f snake
