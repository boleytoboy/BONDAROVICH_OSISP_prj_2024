CXX=gcc
RM=rm -f
CPPFLAGS=-g -I$(SRC)
LDFLAGS=-g -ltar -lz
SRC=src
BUILD=build
DIST=bin

SRCS=$(shell find $(SRC) -name '*.c')
OBJS=$(patsubst $(SRC)/%.c, $(BUILD)/%.o, $(SRCS))

all: app

$(OBJS): $(SRCS)
	mkdir -p $(@D)
	@echo > $@
	$(CXX) $(CPPFLAGS) -o $@ -c $(patsubst $(BUILD)/%.o, $(SRC)/%.c, $@)

app: $(OBJS)
	mkdir -p $(DIST)
	$(CXX) $(LDFLAGS) -o $(DIST)/app $(OBJS)

run: app
	./$(DIST)/app

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) $(DIST)/app
