SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))

ifeq ($(CC), cc)
ifeq ($(origin CC),default)
undefine CC
endif
else ifeq ($(if $(or $(findstring gcc,$(CC)),$(findstring clang,$(CC))),t), t)
override CC += -std=c99
endif

ifeq ($(CC),)
override CC = $(shell command -v c99 >/dev/null 2>&1 && echo c99)
endif
ifeq ($(CC),)
override CC = $(shell command -v gcc >/dev/null 2>&1 && echo gcc -std=c99)
endif
ifeq ($(CC),)
override CC = $(shell command -v clang >/dev/null 2>&1 && echo clang -std=c99)
endif
ifeq ($(CC),)
$(warning Could not find a known C99 compiler. Hoping "cc" works.)
override CC = cc
endif

override CPPFLAGS += -D_GNU_SOURCE -D_ALL_SOURCE -D_XOPEN_SOURCE=700
export CPPFLAGS CC LDLIBS LDFLAGS LOADLIBES CFLAGS

diceware: $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(OBJS): $(wildcard *.h) features.h

features.h: has_feature.sh features/ $(wildcard features/*.feature)
	./has_feature.sh >features.h

clean:
	-rm -rf $(OBJS) features.h diceware .test
