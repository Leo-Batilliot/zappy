GREEN	= \033[1;32m
YELLOW	= \033[1;33m
CYAN	= \033[1;36m
BLUE	= \033[1;34m
RED		= \033[1;31m
RESET	= \033[0m

SERVER	= zappy_server
GUI	= zappy_gui
AI	= zappy_ai
TEAM	= zappy_team

SERVER_DIR	= server
GUI_DIR		= gui
AI_DIR		= ai

SUBMAKE	= $(MAKE) --no-print-directory

all: server gui ai
	@printf "$(GREEN)[zappy] build complete: $(SERVER) $(GUI) $(AI) $(TEAM)$(RESET)\n"

server:
	@printf "$(CYAN)[zappy] building server$(RESET)\n"
	@$(SUBMAKE) -C $(SERVER_DIR)
	@mv -f $(SERVER_DIR)/$(SERVER) ./$(SERVER)

gui:
	@printf "$(BLUE)[zappy] building gui$(RESET)\n"
	@$(SUBMAKE) -C $(GUI_DIR)
	@mv -f $(GUI_DIR)/$(GUI) ./$(GUI)

ai:
	@printf "$(YELLOW)[zappy] building ai$(RESET)\n"
	@$(SUBMAKE) -C $(AI_DIR)
	@mv -f $(AI_DIR)/$(AI) ./$(AI)
	@chmod +x ./$(AI)
	@mv -f $(AI_DIR)/$(TEAM) ./$(TEAM)
	@chmod +x ./$(TEAM)

clean:
	@$(SUBMAKE) -C $(SERVER_DIR) clean
	@$(SUBMAKE) -C $(GUI_DIR) clean
	@$(SUBMAKE) -C $(AI_DIR) clean
	@printf "$(YELLOW)[zappy] clean done$(RESET)\n"

fclean:
	@$(SUBMAKE) -C $(SERVER_DIR) fclean
	@$(SUBMAKE) -C $(GUI_DIR) fclean
	@$(SUBMAKE) -C $(AI_DIR) fclean
	@rm -f $(SERVER) $(GUI) $(AI) $(TEAM)
	@printf "$(RED)[zappy] fclean done$(RESET)\n"

re: fclean all

.PHONY: all server gui ai clean fclean re
