# Makefile for Pluck-GTK
#
# Targets:
#   all     Build the binary (default).
#   clean   Remove compiled objects and the output binary.
#   install Install the binary to PREFIX/bin (default: /usr/local/bin).
#
# Variables you can override on the command line:
#   CC      C compiler          (default: gcc)
#   PREFIX  Installation prefix (default: /usr/local)

CC      := gcc
PREFIX  := /usr/local

# ── pkg-config flags ──────────────────────────────────────────────────────────
PKG_DEPS := gtk4 gtk4-layer-shell-0
CFLAGS   := -Wall -Wextra -O2 $(shell pkg-config --cflags $(PKG_DEPS))
LDFLAGS  := $(shell pkg-config --libs $(PKG_DEPS))

# ── Source & output paths ─────────────────────────────────────────────────────
SRCS   := $(wildcard src/*.c)
OBJS   := $(SRCS:.c=.o)
TARGET := lib/pluck-gtk

# ── Default target ─────────────────────────────────────────────────────────────
.PHONY: all
all: $(TARGET)

# ── Link ───────────────────────────────────────────────────────────────────────
$(TARGET): $(OBJS)
	@mkdir -p lib
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# ── Compile (pattern rule picks up every src/*.c) ─────────────────────────────
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# ── Install ────────────────────────────────────────────────────────────────────
.PHONY: install
install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/pluck-gtk

# ── Clean ──────────────────────────────────────────────────────────────────────
.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)
