## 参考
1. [野火-在`SRAM`中调试代码](https://doc.embedfire.com/mcu/stm32/f103mini/std/zh/latest/book/SRAM.html)
2. `ST`官方例程：`STM32Cube_FW_F1_V1.8.5\Projects\STM32F103RB-Nucleo\Examples\FLASH\FLASH_WriteProtection`
3. [野火-`Flash`的读写保护及解除](https://doc.embedfire.com/mcu/stm32/f103mini/std/zh/latest/book/FLASH_protect.html#)

> 说明：
>	1. 修改`dbger.h`文件，通过串口输出日志，而不是`RTT`;
>	2. `WRP`和`RDP`状态可以通过`STM32CubeProgrammer`直接读取及设置，也可以在程序中设置；
>	3. 此工程主要操作的是`MCU Option Bytes`；而这个`Option Bytes`还有其他控制功能，比如和`IWDG`，低功耗相关；还有两个用户可写的字节，可以保存任意状态数据，可以和升级功能搭配使用！

> 注意：
> 
> 在`STM32F103`上经过反复测试，有如下问题及疑问：
> 1.  设置读保护时，数据字段可以通过备份后再保存的方式维持原来的数据，但之前设置的写保护设置都将丢失(恢复成默认值：所有页面的写保护都未开启)（不正常）；而且必须上电复位后才能运行；（应对方案：在需要读保护时，先使能读保护，再设置写保护）
> 2. 清除读保护时，整个固件都将被擦除，必须重新烧录；（正常）
> 3. 写数据字段时，读保护设置可以维持，写保护设置将恢复到默认值（不正常），需复位；
> 4. 设置写保护时，读保护设置可以维持，数据字段可以维持；可以使能指定页的写保护，需复位；（正常）
> 5. 清除写保护时，读保护设置可以维持，数据字段可以维持；只能一次性清除所有页的写保护设置（不正常），需复位；

### 1. 读保护`RDP`
1. 使能读保护后，每次尝试通过`JLink`读取固件或者通过`RTT Viewer`读取日志等方式去读`MCU`内容时，都会导致`MCU`进入`Hard_Fault`，必须上电复位`POR`后才能重新运行！
2. 因此，在通过`STM32CubeProgrammer`使能读保护后，程序不会运行，需要重新上电重启后才会运行（必须断电再上电，直接按`RESET`不行！）；
3. 而每次尝试通过`RTT Viewer`读取日志时，也会进入`Hard_Fault`；这就说明开启读保护后，只能通过串口输出日志了；

### 2. 写保护`WRP`
1. 使能写保护后，通过`MDK`还是可以直接下载固件，并不会报错！
2. 使能写保护后，通过`J-Link`连接时会出现提示，但是可以选择`unLock`；`unLock`后可以正常下载固件，如果不进行`unLock`下载固件会失败，原固件被保护；

### 3. 从`SRAM`启动运行
> 注意：必须先通过`STM32CubeMx`生成完整工程，再新建`target`，否则会导入出错！
1. `MDK`可以很方便地设置多个`Project targets`，每个`target`的功能一样，但是可以设置完全不同的工程配置；
2. 必须在新建`target`之前，通过`STM32CubeMx`生成完整的代码；实测在新建`target`后，再通过`STM32CubeMx`生成代码会导致工程导入失败！
3. 从`Flash`启动时，代码放在`Flash`，运行时需使用`RAM`；当设置从`SRAM`启动，也需要将`SRAM`分成两个区域，一个类似`Flash`存储代码，一个用于程序运行的`RAM`；
4. 切换`target`时，需要点击`MDK`的`rebuild`全部重新编译，不然有些文件不会被更新，导致程序启动失败；
5. 工程通过[`INI`文件](./MDK-ARM/JLinkSettings_SRAM.ini)强制设置了`MSP`和`PC`指针，所以在使用`SRAM target`启动时，即使没有设置对应的`BOOT0/1`值也可以启动成功；详情可见`参考1`；