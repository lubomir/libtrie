PROGRAM=list-compile
SRCS=compile.c trie.c

CC=gcc
CFLAGS=-Wall -Wextra -g -pedantic -std=gnu99
LDFLAGS=
OBJS=$(SRCS:.c=.o)
DEPS=$(SRCS:.c=.dep)

ifeq ($V,1)
cmd = $1
else
cmd = @printf " %s\t%s\n" $2 $3; $1
endif

$(PROGRAM) : $(OBJS)
	$(call cmd,$(CC) $^ -o $@ $(LDFLAGS),CCLD,$@)

%.o : %.c
	$(call cmd,$(CC) $(CFLAGS) -c -o $@ $<,CC,$@)
%.dep : %.c
	@$(CC) $(CFLAGS) -MM -o $@ $<

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif

.PHONY : clean watch

watch :
	while true; do inotifywait -qe close_write .; $(MAKE); done
clean :
	rm -f $(OBJS) $(DEPS)
	rm -f $(PROGRAM)

