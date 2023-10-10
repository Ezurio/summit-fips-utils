CFLAGS_LOCAL += -Wall -Werror -Isrc/include -Isrc/

ifdef DEBUG
CFLAGS_LOCAL += -ggdb
CFLAGS_LOCAL += -DENGINEERING_RELEASE
endif

OBJS = ./src/ossl-fipsload.o

LIBS += -lcrypto

.PHONY: all clean
.DEFAULT: all

all: ossl-fipsload

%.o: %.c
	$(CC) $(CFLAGS) $(CFLAGS_LOCAL) -c $^ -o $@

ossl-fipsload : $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	rm -f src/*.o ossl-fipsload
