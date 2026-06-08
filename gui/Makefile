GREEN	= \033[1;32m
BLUE	= \033[1;34m
RED		= \033[1;31m
RESET	= \033[0m

NAME	= zappy_gui

CXX			= g++
CXXFLAGS	= -std=c++17 -Wall -Wextra \
			  -Isrc \
			  -Isrc/app \
			  -Isrc/network \
			  -Isrc/parser \
			  -Isrc/renderer \
			  -Isrc/world
LDFLAGS		= -lsfml-graphics -lsfml-window -lsfml-system

# make DEBUG=1
ifeq ($(DEBUG),1)
CXXFLAGS	+= -g3 -DDEBUG
endif

OBJDIR	= obj
SRC		= src/main.cpp \
		  src/app/App.cpp \
		  src/network/Network.cpp \
		  src/parser/Parser.cpp \
		  src/renderer/Renderer.cpp \
		  src/world/World.cpp
OBJ		= $(SRC:src/%.cpp=$(OBJDIR)/%.o)
DEP		= $(OBJ:.o=.d)

all: $(NAME)

$(NAME): $(OBJ)
	@$(CXX) $(OBJ) -o $(NAME) $(LDFLAGS)
	@printf "$(GREEN)  LD   $(NAME)$(RESET)\n"

$(OBJDIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	@printf "$(BLUE)  CC   $<$(RESET)\n"
	@$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

clean:
	@rm -rf $(OBJDIR)

fclean: clean
	@rm -f $(NAME)

re: fclean all

-include $(DEP)

.PHONY: all clean fclean re
