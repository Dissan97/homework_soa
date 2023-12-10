LIBS = lib/*.c
CFLAGS = -Wall
APP_MAIN = main.c
CACHE_DIR = cache
CACHE_FILE = log
all:
	gcc -o app $(APP_MAIN) $(LIBS) $(CFLAGS)

play:
	./app

play_log:
	./app > ./$(CACHE_DIR)/$(CACHE_FILE)