# mslog
1.综述
一款轻量级的C语言日志库，目前测试过Linux Ubuntu平台

2.如何使用
可参考test_mslog.c

3.功能概要
5个级别，分别为
  DEBUG, INFO, WARN, ERROR, FATAL
  日志库支持多线程安全(刷盘线程与log线程相互独立)；
  日志库支持tag & level过滤；
  日志库支持输出日志所在文件行和函数名；
  日志库支持标准中断输出。
