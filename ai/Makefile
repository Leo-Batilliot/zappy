GREEN	= \033[1;32m
YELLOW	= \033[1;33m
RED	= \033[1;31m
RESET	= \033[0m

NAME	= zappy_ai
PYTHON	= python3

AI_MAIN	= $(abspath src/main.py)

all: $(NAME)

$(NAME): src/main.py
	@printf '#!/bin/sh\nexec $(PYTHON) "%s" "$$@"\n' '$(AI_MAIN)' > $(NAME)
	@chmod +x $(NAME)
	@printf "$(GREEN)  GEN  $(NAME)$(RESET)\n"

clean:
	@find . -type d -name __pycache__ -exec rm -rf {} + 2>/dev/null || true

fclean: clean
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
