GREEN	= \033[1;32m
CYAN	= \033[1;36m
RED		= \033[1;31m
RESET	= \033[0m

NAME	= zappy_server

CXX			= g++
CXXFLAGS	= -std=c++20 -Wall -Wextra -Iinclude -Isrc
LDFLAGS		=

# make DEBUG=1
ifeq ($(DEBUG),1)
CXXFLAGS	+= -g3 -DDEBUG
endif

OBJDIR	= obj
SRC	= 	src/main.cpp			\
		src/args/Parser.cpp		\
		src/server/Server.cpp	\
		src/server/Client.cpp	\
		src/time/Scheduler.cpp	\
		src/game/World.cpp		\
		src/game/GameState.cpp	\
		src/protocol/GuiProtocol.cpp	\

OBJ	= $(SRC:src/%.cpp=$(OBJDIR)/%.o)
DEP	= $(OBJ:.o=.d)

all: $(NAME)

$(NAME): $(OBJ)
	@$(CXX) $(OBJ) -o $(NAME) $(LDFLAGS)
	@printf "$(GREEN)  LD   $(NAME)$(RESET)\n"

$(OBJDIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	@printf "$(CYAN)  CC   $<$(RESET)\n"
	@$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

clean:
	@rm -rf $(OBJDIR)

fclean: clean
	@rm -f $(NAME)

re: fclean all

-include $(DEP)

.PHONY: all clean fclean re
