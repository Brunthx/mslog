# ========================= 全局配置（可按需修改）=========================

TARGET      := mslog_demo

CC          := gcc

CFLAGS      := -Wall -g -O2

LDFLAGS     := -lpthread

SRC_FILES   := test_mslog.c mslog.c mslog_utils.c mslog_thread.c mslog_mem_pool.c

OBJ_FILES   := $(SRC_FILES:.c=.o)
# log file path
LOG_FILES   := ./mslog.log* /tmp/mslog.log*

# ========================= 核心指令（无需修改）=========================
all: $(TARGET)
	@echo -e "\033[32m✅ build success! o file:./$(TARGET)\033[0m"

$(TARGET): $(OBJ_FILES)
	$(CC) $(OBJ_FILES) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: all
#@echo -e "\033[33m🔧 clean before mslog run：查杀残留进程 + 删除旧0字节日志\033[0m"
	@pkill -9 $(TARGET) 2>/dev/null || true
	@rm -rf $(LOG_FILES) 2>/dev/null || true
	@echo -e "\033[33m🚀 mslog start running：./$(TARGET)\033[0m"
	@./$(TARGET)
	@echo -e "\033[32m✅ mslog run finish\033[0m"
	@echo -e "\033[32m📄 log file: \033[0m"
	@ls -lh /tmp/mslog.log 2>/dev/null || echo -e "\033[31m⚠️  log file isn't exist!\033[0m"

clean:
	@echo -e "\033[33m🧹 执行清理操作...\033[0m"
	@pkill -9 $(TARGET) 2>/dev/null || true
	@rm -rf $(OBJ_FILES) $(TARGET) core.* 2>/dev/null || true
	@rm -rf $(LOG_FILES) 2>/dev/null || true
	@echo -e "\033[32m✅ clean finsih！\033[0m"


rebuild: clean all
	@echo -e "\033[32m✅ rebuild finish！all src code rebuild033[0m"

.PHONY: all run clean rebuild