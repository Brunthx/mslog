CC := gcc
CFLAGS := -Wall -Wextra -O2
LDFLAGS := -lpthread #connect pthread

TARGET := mslog_demo

SRC := test_mslog.c mslog.c mslog_mem_pool.c mslog_utils.c mslog_thread.c

OBJ := $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	@echo "connect all module, build .o"
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)
	@echo "build finish! file: ./$(TARGET)"

%.o: %.c
	@echo "build c file: $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	@echo -e "\n===================================="
	@echo "test_mslog is running..."
	@echo "======================================="
	./$(TARGET)
	@echo -e "\n test is finish!"

clean:
	@echo "clean build output & test output"
	rm -rf $(OBJ) $(TARGET)
	rm -rf mslog.log*
	@echo "clean finish!"

rebuild: clean all
	@echo "rebuild finish"

.PHONY: all run clean rebuild
