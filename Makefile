NAME	:= smenu
SRC		:= main.c
OBJ		:= $(SRC:%.c=%.o)
TAGS	:= tags

.DEFAULT_GOAL := all

CFLAGS	:= -Wall -Wextra -Werror

all: $(NAME) $(TAGS)

$(TAGS): $(SRC)
	ctags --recurse -f $@ .

clean:
	$(RM) $(NAME) $(OBJ) $(TAGS)

$(NAME): $(OBJ)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
