.POSIX:

include config.mk

out = dale
src = src/main.c src/util.c src/parse.c src/variable.c src/host/$(HOST).c
obj = $(src:.c=.o)

$(out): $(obj)
	$(LD) $(LDFLAGS) -o $@ $(obj) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(out) $(obj)
