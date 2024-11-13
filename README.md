## 参考
1. [野火-在`SRAM`中调试代码](https://doc.embedfire.com/mcu/stm32/f103mini/std/zh/latest/book/SRAM.html)
2. `ST`官方例程：`STM32Cube_FW_F1_V1.8.5\Projects\STM32F103RB-Nucleo\Examples\FLASH\FLASH_WriteProtection`
3. [野火-`Flash`的读写保护及解除](https://doc.embedfire.com/mcu/stm32/f103mini/std/zh/latest/book/FLASH_protect.html#)

## 说明
> 注意：
>	1. 修改`dbger.h`文件，通过串口输出日志，而不是`RTT`;
>	2. `WRP`和`RDP`状态可以通过`STM32CubeProgrammer`直接读取及设置，也可以在程序中设置；

### 1. 读保护`RDP`
1. 使能读保护后，每次尝试通过`JLink`读取固件或者通过`RTT Viewer`读取日志等方式去读`MCU`内容时，都会导致`MCU`进入`Hard_Fault`，必须上电复位`POR`后才能重新运行！
2. 因此，在通过`STM32CubeProgrammer`使能读保护后，程序不会运行，需要重新上电重启后才会运行（必须断电再上电，直接按`RESET`不行！）；
3. 而每次尝试通过`RTT Viewer`读取日志时，也会进入`Hard_Fault`；这就说明开启读保护后，只能通过串口输出日志了；
```c
/**
 *	@brief	set Flash RDP state
 * 	@param	level ref FLASHEx_OB_Read_Protection in stm32f1xx_hal_flash_ex.h
 */
void set_flash_read_protect(uint8_t level)
{
	HAL_StatusTypeDef hal_sta = HAL_OK;
	FLASH_OBProgramInitTypeDef OBInit = {0};
	
	HAL_FLASHEx_OBGetConfig(&OBInit);
	LOG_DBG("set RDP(L%c) to RDP(L%c)\n", (OBInit.RDPLevel==OB_RDP_LEVEL_0)?'0':'1', (level==OB_RDP_LEVEL_0)?'0':'1');
	
	if(OBInit.RDPLevel == OB_RDP_LEVEL_0 && level != OB_RDP_LEVEL_0) {
		// Flash RDP is disable, need enable it
		OBInit.OptionType = OPTIONBYTE_RDP;
		OBInit.RDPLevel = OB_RDP_LEVEL_1;	// any value except 0xA5 is OK
		HAL_FLASH_Unlock();
		HAL_FLASH_OB_Unlock();
		hal_sta = HAL_FLASHEx_OBProgram(&OBInit);
		if(hal_sta == HAL_OK) {
			LOG_DBG("Enable Flash RDP OK, must Power-On Reset to run the program!\n");	// block output
			HAL_FLASH_OB_Launch();
		} else {
			LOG_ERR("Enable Flash RDP err[%d]\n", hal_sta); 
		}
	} else if(OBInit.RDPLevel != OB_RDP_LEVEL_0 && level == OB_RDP_LEVEL_0) {
		// Flash RDP is enable, need disable it
		LOG_DBG("Disable Flash RDP, the Flash will be Erase.\n");
		LOG_DBG("Because the Flash be Erase, you must download the program again\n");
		OBInit.OptionType = OPTIONBYTE_RDP;
		OBInit.RDPLevel = OB_RDP_LEVEL_0;
		HAL_FLASH_Unlock();
		HAL_FLASH_OB_Unlock();
		hal_sta = HAL_FLASHEx_OBProgram(&OBInit);
		if(hal_sta != HAL_OK) {
			LOG_ERR("Disable Flash RDP err[%d]\n", hal_sta);
		}
	}
}
```

### 2. 写保护`WRP`
1. 使能写保护后，通过`MDK`还是可以直接下载固件，并不会报错！
2. 使能写保护后，通过`J-Link`连接时会出现提示，但是可以选择`unLock`；`unLock`后可以正常下载固件，如果不进行`unLock`下载固件会失败，原固件被保护；
```c
/**
 *	@brief	set Flash pages WRP state
 * 	@param	pages	ref FLASHEx_OB_Write_Protection in stm32f1xx_hal_flash_ex.h
 *	@param	wrpSta	ref FLASHEx_OB_WRP_State in stm32f1xx_hal_flash_ex.h
 *	@note	each bit in WRPPage means one page: 1 is mean this page's WRP is disable, and 0 is mean enable
 */
void set_flash_write_protection(uint32_t pages, uint32_t wrpSta)
{
	HAL_StatusTypeDef hal_sta = HAL_OK;
	FLASH_OBProgramInitTypeDef OBInit = {0};
	
	HAL_FLASHEx_OBGetConfig(&OBInit);
	LOG_DBG("pagesSta[0x%08X](1=WRP-OFF), change to WRP-%s pages[0x%08X]\n", OBInit.WRPPage, wrpSta?"ON":"OFF", pages);
	
	if((OBInit.WRPPage & pages) != 0 && wrpSta == OB_WRPSTATE_ENABLE) {
		OBInit.OptionType = OPTIONBYTE_WRP;
		OBInit.WRPState = OB_WRPSTATE_ENABLE;
		OBInit.WRPPage = pages;
		HAL_FLASH_Unlock();
		HAL_FLASH_OB_Unlock();
		hal_sta = HAL_FLASHEx_OBProgram(&OBInit);
		if(hal_sta == HAL_OK) {
			LOG_DBG("set Flash WRP OK\n");
			// HAL_Delay(20);		// LOG_DBG() is blocking, no need delay
			HAL_FLASH_OB_Launch();	// RESET is enough, no need POR
		} else {
			LOG_ERR("set Flash WRP err[%d]\n", hal_sta);
		}
	} else if((OBInit.WRPPage & pages) != pages && wrpSta == OB_WRPSTATE_DISABLE) {
		OBInit.OptionType = OPTIONBYTE_WRP;
		OBInit.WRPState = OB_WRPSTATE_DISABLE;
		OBInit.WRPPage = pages;
		HAL_FLASH_Unlock();
		HAL_FLASH_OB_Unlock();
		hal_sta = HAL_FLASHEx_OBProgram(&OBInit);
		if(hal_sta == HAL_OK) {
			LOG_DBG("set Flash WRP OK\n");	
			// HAL_Delay(20);		// LOG_DBG() is blocking, no need delay
			HAL_FLASH_OB_Launch();	// RESET is enough, no need POR
		} else {
			LOG_ERR("set Flash WRP err[%d]\n", hal_sta);
		}
	}
}
```

### 3. 从`SRAM`启动运行
> 注意：必须先通过`STM32CubeMx`生成完整工程，再新建`target`，否则会导入出错！
1. `MDK`可以很方便地设置多个`Project targets`，每个`target`的功能一样，但是可以设置完全不同的工程配置；
2. 必须在新建`target`之前，通过`STM32CubeMx`生成完整的代码；实测在新建`target`后，再通过`STM32CubeMx`生成代码会导致工程导入失败！
3. 从`Flash`启动时，代码放在`Flash`，运行时需使用`RAM`；当设置从`SRAM`启动，也需要将`SRAM`分成两个区域，一个类似`Flash`存储代码，一个用于程序运行的`RAM`；