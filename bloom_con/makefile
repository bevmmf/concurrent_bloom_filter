.DEFAULT_GOAL := all
# var
CC := gcc
SAN := -fsanitize=thread -fno-omit-frame-pointer
CFLAGS := -Wall -O2 -g $(SAN) 
LDLIBS := -lpthread
LDTSAN := $(SAN)

# file list
SRC   := bloom_con.c bench.c
# header files
HEADERS := bloom_con.h xxhash.h
# object files
OBJ := $(SRC:.c=.o)


TARGET := bench
# rule
## generic pattern
%.o: %.c bloom_con.h xxhash.h
	$(CC) $(CFLAGS) -c $< -o $@
## final executables
$(TARGET): $(OBJ)
	$(CC) $(LDTSAN) $^ $(LDLIBS) -o $@

# convenience targets
.PHONY: all check clean
all: $(TARGET) 

check: $(TARGET)
	./$(TARGET)
	
clean:
	$(RM) $(OBJ) $(TARGET)

