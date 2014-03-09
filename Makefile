SRCS=compile.c trie.c query.c

CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=gnu99 -O2 -fPIC
LDFLAGS=
DEPS=$(SRCS:.c=.dep)

ifeq ($V,1)
cmd = $1
else
cmd = @printf " %s\t%s\n" $2 $3; $1
endif

all : list-compile list-query libtrie.so

list-compile : compile.o trie.o
	$(call cmd,$(CC) $^ -o $@ $(LDFLAGS),CCLD,$@)

list-query : query.o trie.o
	$(call cmd,$(CC) $^ -o $@ $(LDFLAGS),CCLD,$@)

libtrie.so : trie.o
	$(call cmd,$(CC) $^ -o $@ -shared,CCLD,$@)

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
	rm -f *.o *.dep
	rm -f list-compile list-query
