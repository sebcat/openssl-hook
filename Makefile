CFLAGS+=-Wall -Werror -g
LDFLAGS+=-lc
RM?=rm -f

.PHONY:clean

all: hook.so.1 print-hooklog

hook.so.1: hook.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $< $(LDFLAGS)

print-hooklog: print-hooklog.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean: 
	$(RM) hook.so.1 print-hooklog
