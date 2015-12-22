SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))

ifeq ($(CC), cc)
override CC = c99
else ifeq ($(if $(or $(findstring gcc,$(CC)),$(findstring clang,$(CC))),t), t)
override CC += -std=c99
endif

override CPPFLAGS += -D_GNU_SOURCE -D_ALL_SOURCE -D_XOPEN_SOURCE=700
export CPPFLAGS CC LDLIBS LDFLAGS LOADLIBES CFLAGS

diceware: $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(OBJS): $(wildcard *.h) features.h

features.h: has_feature.sh $(wildcard features/*.features)
	./has_feature.sh >features.h

clean:
	-rm -rf $(OBJS) features.h diceware .test
