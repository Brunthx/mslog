# mslog
#1.综述  

  一款轻量级的C语言日志库，目前测试过Linux Ubuntu平台

#2.如何使用  

  可参考test_mslog.c

#3.功能概要  

  日志库支持5个级别，分别为  
  
  DEBUG, INFO, WARN, ERROR, FATAL  
  
  日志库支持多线程安全(刷盘线程与log线程相互独立)；  
  
  日志库支持tag & level过滤；  
  
  日志库支持输出日志所在文件行和函数名；  
  
  日志库支持标准中断输出；  
  

#update 2025/12/28：  

  新增控制台颜色输出；  
  
  线程单独封装；  

  工具函数单独封装；
  
  内存池单独封装。

#update 2026/01/01：  

  新增宏MULTI_THREAD对单线程/多线程进行卡控；  
  
  新增批量刷盘；  

  重写测试用例。  

  #update 2026/01/02：  

  新增单线程压力测试功能，具体可参考：test_mslog_stress_single.c  
  
  新增单线程压力测试指令：make stress_single；  
  
  新增多线程压力测试功能，具体可参考：test_mslog_stress_multi.c  
  
  新增多线程压力测试指令：make stress_multi。  

  #update 2026/01/03：

  新增mslog_keep_alive接口（无数据时更新文件属性，专为tail -f保活设计）。
