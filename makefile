TARGET        := mslog_demo

TARGET_MULTI  := $(TARGET)_multi

CC            := gcc

CFLAGS        := -Wall -g -O2 -Wno-unused-variable -Wno-format-truncation

LDFLAGS       := -lpthread

SRC_FILES     := test_mslog.c mslog.c mslog_utils.c mslog_thread.c mslog_mem_pool.c

OBJ_FILES     := $(SRC_FILES:.c=.o)

OBJ_MULTI     := $(SRC_FILES:.c=.multi.o)

LOG_FILES     := ./mslog.log* /tmp/mslog.log*

MULTI_DEFINE  := -DMULTI_THREAD=1

COLOR_GREEN   := \033[32m
COLOR_YELLOW  := \033[33m
COLOR_RED     := \033[31m
COLOR_RESET   := \033[0m

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo -e "$(COLOR_YELLOW)🔨 编译单线程目标文件：$@$(COLOR_RESET)"

%.multi.o: %.c
	@$(CC) $(CFLAGS) $(MULTI_DEFINE) -c $< -o $@
	@echo -e "$(COLOR_YELLOW)🔨 编译多线程目标文件：$@ [开启MULTI_THREAD]$(COLOR_RESET)"

all: $(TARGET)
	@echo -e "$(COLOR_GREEN)✅ 【单线程模式】编译成功！可执行文件：./$(TARGET)$(COLOR_RESET)"

$(TARGET): $(OBJ_FILES)
	$(CC) $(OBJ_FILES) -o $(TARGET) $(LDFLAGS)

multi_all: $(TARGET_MULTI)
	@echo -e "$(COLOR_GREEN)✅ 【多线程模式】编译成功！可执行文件：./$(TARGET_MULTI)$(COLOR_RESET)"

$(TARGET_MULTI): $(OBJ_MULTI)
	$(CC) $(OBJ_MULTI) -o $(TARGET_MULTI) $(LDFLAGS)

run_single: all
	@echo -e "$(COLOR_YELLOW)🔧 单线程运行前置清理：查杀残留进程 + 删除旧日志$(COLOR_RESET)"
	@pkill -9 $(TARGET) 2>/dev/null || true
	@rm -rf $(LOG_FILES) 2>/dev/null || true
	@echo -e "$(COLOR_YELLOW)🚀 启动【单线程模式】：./$(TARGET)$(COLOR_RESET)"
	@./$(TARGET)
	@echo -e "$(COLOR_GREEN)✅ 单线程运行完成！$(COLOR_RESET)"
	@echo -e "$(COLOR_GREEN)📄 日志文件位置：$(COLOR_RESET)"
	@ls -lh /tmp/mslog.log 2>/dev/null || echo -e "$(COLOR_RED)⚠️  日志文件未生成！$(COLOR_RESET)"

run_multi: multi_all
	@echo -e "$(COLOR_YELLOW)🔧 多线程运行前置清理：查杀残留进程 + 删除旧日志$(COLOR_RESET)"
	@pkill -9 $(TARGET_MULTI) 2>/dev/null || true
	@rm -rf $(LOG_FILES) 2>/dev/null || true
	@echo -e "$(COLOR_YELLOW)🚀 启动【多线程模式】：./$(TARGET_MULTI) [开启统计锁+内存池]$(COLOR_RESET)"
	@./$(TARGET_MULTI)
	@echo -e "$(COLOR_GREEN)✅ 多线程运行完成！$(COLOR_RESET)"
	@echo -e "$(COLOR_GREEN)📄 日志文件位置：$(COLOR_RESET)"
	@ls -lh /tmp/mslog.log 2>/dev/null || echo -e "$(COLOR_RED)⚠️  日志文件未生成！$(COLOR_RESET)"

clean:
	@echo -e "$(COLOR_YELLOW)🧹 执行基础清理操作...$(COLOR_RESET)"
	@pkill -9 $(TARGET) $(TARGET_MULTI) 2>/dev/null || true
	@rm -rf $(OBJ_FILES) $(OBJ_MULTI) $(TARGET) $(TARGET_MULTI) core.* 2>/dev/null || true
	@rm -rf $(LOG_FILES) 2>/dev/null || true
	@echo -e "$(COLOR_GREEN)✅ clean完成！清理所有编译产物+日志+残留进程$(COLOR_RESET)"

rebuild: clean all
	@echo -e "$(COLOR_GREEN)✅ 【单线程模式】重建完成！所有源码已重新编译$(COLOR_RESET)"

multi_rebuild: clean multi_all
	@echo -e "$(COLOR_GREEN)✅ 【多线程模式】重建完成！所有源码已重新编译$(COLOR_RESET)"

full_clean: clean
	@echo -e "$(COLOR_YELLOW)🧹 执行深度清理...$(COLOR_RESET)"
	@rm -rf ./*.log ./*.o ./*~ 2>/dev/null || true
	@echo -e "$(COLOR_GREEN)✅ full_clean完成！目录已恢复纯净$(COLOR_RESET)"

.PHONY: all multi_all run run_multi clean rebuild multi_rebuild full_clean