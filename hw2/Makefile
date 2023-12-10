LIBS = lib/*.c
CFLAGS = -Wall -DFENCING
APP_MAIN = main.c
CACHE_DIR = cache
CACHE_FILE = log
all:
	gcc -o app $(APP_MAIN) $(LIBS) $(CFLAGS) -lpthread
all_long: CFLAGS += -DOPERATIONS=0xfffff
all_long: all

speak: CFLAGS += -DAUDIT=if\(1\)
speak: all
play:
	./app

play_log:
	./app > ./$(CACHE_DIR)/$(CACHE_FILE)

