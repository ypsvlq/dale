.POSIX:

include config.mk

out = dale
src = src/main.c src/util.c src/parse.c src/variable.c
obj = $(src:.c=.o)

$(out): $(obj)
	$(LD) $(LDFLAGS) -o $@ $(obj)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(out) $(obj)
