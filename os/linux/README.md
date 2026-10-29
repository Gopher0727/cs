MBR：第一个扇区 512 字节，最大只支持 2TB 磁盘，搭配固件 BIOS
- 引导程序 446 + 分区表 64（最多四个主分区） + 结束标志 2（55AA）

开机启动流程：
1. BIOS/UEFI 加电自检
2. 读引导程序 MBR/ESP
3. GRUB2 引导程序显示启动菜单，把内核文件 vmlinuz 和临时根文件系统 initramfs 载入内存
4. 内核初始化硬件，借助 initramfs 驱动挂载真正的根文件系统
5. 启动 PID 1 第一个用户进程 systemd
6. systemd 并行启动各种服务
7. 最后出现登录界面

- 内部命令：Shell 自带的，如 cd echo pwd export
- 外部命令：磁盘上的程序文件，如 ls