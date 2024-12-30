NAME    := dmenu-cli
SRC     := main.c
OBJ     := $(SRC:%.c=%.o)
TAGS    := tags
DESTDIR := /usr/local/bin/

CFLAGS  := -Wall -Wextra -Werror

all: $(NAME) $(TAGS)

$(TAGS): $(SRC)
	ctags --recurse -f $@ .

clean:
	$(RM) $(NAME) $(OBJ) $(TAGS)

install: $(DESTDIR)$(NAME)

uninstall:
	$(RM) $(DESTDIR)$(NAME)

$(DESTDIR)$(NAME): $(NAME)
	cp $^ $@
	chmod 755 $@

$(NAME): $(OBJ)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
