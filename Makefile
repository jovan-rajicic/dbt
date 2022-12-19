APP_NAME := dbt

BUILD_DIR := build
SOURCE_FILES := src/*.c src/adapters/*.c

CC := clang
CFLAGS := -Wall -Werror -lncurses -ljansson -lpq

MV := mv
CP := cp
MKDIR := mkdir -p


all: prep compile
debug: prep compile_debug

prep:
	[ -d $(BUILD_DIR) ] || $(MKDIR) $(BUILD_DIR)
compile:
	$(CC) -o $(BUILD_DIR)/$(APP_NAME) $(SOURCE_FILES) $(CFLAGS)
compile_debug:
	$(CC) -g -o $(BUILD_DIR)/$(APP_NAME) $(SOURCE_FILES) $(CFLAGS)


run:
	cd $(BUILD_DIR) && ./$(APP_NAME)


clean:
	$(RM) -r $(BUILD_DIR)/*
