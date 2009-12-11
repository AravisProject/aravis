CC=gcc
LD=gcc

CFLAGS=`pkg-config --cflags glib-2.0`
CFLAGS+=`pkg-config --cflags gobject-2.0`
CFLAGS+=-Wall -g

LDFLAGS=`pkg-config --libs glib-2.0`
LDFLAGS+=`pkg-config --libs gio-2.0`
LDFLAGS+=`pkg-config --libs gobject-2.0`

OBJS=main.o

PROG=aravis

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(PROG)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

all: $(PROG)

default: $(PROG)

clean:
	rm -f $(OBJS) $(PROG)
