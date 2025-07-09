.DEFAULT_GOAL := all
# var
CC := gcc
SAN := -fsanitize=thread -fno-omit-frame-pointer
CFLAGS := -Wall -O2 -g $(SAN) 
LDLIBS := -lpthread
LDTSAN := $(SAN)

# file list
COMMON_SRC   := bloom.c
IPC_SRC      := bench_IPC.c
MUTEX_SRC    := bench_mutex.c
# header files
HEADERS := bloom.h xxhash.h
# object files
OBJ := $(SRC:.c=.o)
COMMON_OBJ   := $(COMMON_SRC:.c=.o)
IPC_OBJ      := $(IPC_SRC:.c=.o)
MUTEX_OBJ    := $(MUTEX_SRC:.c=.o)


TARGET_IPC := bench_IPC
TARGET_MUTEX := bench_mutex

# rule
## generic pattern
%.o: %.c bloom.h xxhash.h
	$(CC) $(CFLAGS) -c $< -o $@
## final executables
$(TARGET_IPC): $(COMMON_OBJ) $(IPC_OBJ)
	$(CC) $(LDTSAN) $^ $(LDLIBS) -o $@

$(TARGET_MUTEX): $(COMMON_OBJ) $(MUTEX_OBJ)
	$(CC) $(LDTSAN) $^ $(LDLIBS) -o $@
# convenience targets
.PHONY: all check clean
all: $(TARGET_IPC) $(TARGET_MUTEX)

check: $(TARGET)
	@echo "Running IPC version:" && ./$(TARGET_IPC)
	@echo "Running MUTEX version:" && ./$(TARGET_MUTEX)
clean:
	$(RM) $(COMMON_OBJ) $(IPC_OBJ) $(MUTEX_OBJ) $(TARGET_IPC) $(TARGET_MUTEX)

