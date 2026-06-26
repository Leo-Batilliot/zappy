GREEN	= \033[1;32m
YELLOW	= \033[1;33m
RED	= \033[1;31m
RESET	= \033[0m

NAME	= zappy_ai
LAUNCHER = zappy_team
PYTHON	= python3

AI_MAIN      = $(abspath src/main.py)
LAUNCHER_MAIN = $(abspath src/launcher.py)

all: $(NAME) $(LAUNCHER)

$(NAME): src/main.py
	@printf '#!/bin/sh\nexec $(PYTHON) "%s" "$$@"\n' '$(AI_MAIN)' > $(NAME)
	@chmod +x $(NAME)
	@printf "$(GREEN)  GEN  $(NAME)$(RESET)\n"

$(LAUNCHER): src/launcher.py
	@printf '#!/bin/sh\nexec $(PYTHON) "%s" "$$@"\n' '$(LAUNCHER_MAIN)' > $(LAUNCHER)
	@chmod +x $(LAUNCHER)
	@printf "$(GREEN)  GEN  $(LAUNCHER)$(RESET)\n"

clean:
	@find . -type d -name __pycache__ -exec rm -rf {} + 2>/dev/null || true

fclean: clean
	@rm -f $(NAME) $(LAUNCHER)

re: fclean all

.PHONY: all clean fclean re
