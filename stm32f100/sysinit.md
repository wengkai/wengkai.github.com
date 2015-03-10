# 100C8的SystemInit

ba5ag.kai at gmail.com 2014-04-25

	Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x/system_stm32f10x.c

这是startup.s里调用的第一个函数SystemInit所在的地方。我们这样来编译它：

	CC = $(GNUARM)/bin/arm-none-eabi-gcc
	DEFS = -DSTM32F10X_MD_VL
	FLAG = -mcpu=cortex-m3 -mthumb -Wall
	system_stm32f10x.o: $(STM)/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x/system_stm32f10x.c
	        $(CC) $(INC) $(DEFS) $(FLAG) -c $<

我们来看看这个.c里面有什么。

	#include "stm32f10x.h"

暂时还不清楚它具体为什么要用到这个头文件，先看下去。

	#if defined (STM32F10X_LD_VL) || (defined STM32F10X_MD_VL) || (defined STM32F10X_HD_VL)
	/* #define SYSCLK_FREQ_HSE    HSE_VALUE */
	#define SYSCLK_FREQ_24MHz  24000000

既然我们-D了`STM32F10X_MD_VL`，那么这个宏`SYSCLK_FREQ_24MHz`就会被定义，显然后面会有用。

	#define VECT_TAB_OFFSET  0x0

中断向量表起始地址。这个宏在我们这块芯片的代码中没有用到。

	#elif defined SYSCLK_FREQ_24MHz
	  uint32_t SystemCoreClock         = SYSCLK_FREQ_24MHz; 

看出来了吧，因为我们-D了`STM32F10X_MD_VL`，那么就有了宏`SYSCLK_FREQ_24MHz`，于是`SystemCoreClock`这个全局变量就有了24000000的值。这个全局变量是开放的，所以在你的应用程序里是可以使用的，以此可以知道当前的芯片的时钟频率。

	__I uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};

这是AHB外设总线上的预分频器的分频表。这是用在下面一个叫做`SystemCoreClockUpdate`的函数里的。这个`SystemCoreClockUpdate`函数是用来根据时钟类寄存器的值重新刷新`SystemCoreClock`这个全局变量的。

接下来就是这个`SystemInit`函数了，这个反而很好懂，我把不相关的条件编译的部分去掉，整理如下：

	void SystemInit (void)
	{
	  /* Reset the RCC clock configuration to the default reset state(for debug purpose) */
	  /* Set HSION bit */
	  RCC->CR |= (uint32_t)0x00000001;

	  /* Reset SW, HPRE, PPRE1, PPRE2, ADCPRE and MCO bits */
	  RCC->CFGR &= (uint32_t)0xF0FF0000;
	  
	  /* Reset HSEON, CSSON and PLLON bits */
	  RCC->CR &= (uint32_t)0xFEF6FFFF;

	  /* Reset HSEBYP bit */
	  RCC->CR &= (uint32_t)0xFFFBFFFF;

	  /* Reset PLLSRC, PLLXTPRE, PLLMUL and USBPRE/OTGFSPRE bits */
	  RCC->CFGR &= (uint32_t)0xFF80FFFF;

	  /* Disable all interrupts and clear pending bits  */
	  RCC->CIR = 0x009F0000;

	  /* Reset CFGR2 register */
	  RCC->CFGR2 = 0x00000000;      
	    
	  /* Configure the System clock frequency, HCLK, PCLK2 and PCLK1 prescalers */
	  /* Configure the Flash Latency cycles and enable prefetch buffer */
	  SetSysClock();

	  SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal FLASH. */
	}

它调用了`SetSysClock()`，这个函数就在它下面（中间隔了另一个函数），这个函数由一堆的条件编译，最后走去调用具体和芯片相关的一个时钟配置函数，我们的F100C8是`SetSysClockTo24`：

	  __IO uint32_t StartUpCounter = 0, HSEStatus = 0;
	  /* SYSCLK, HCLK, PCLK2 and PCLK1 configuration ---------------------------*/    
	  /* Enable HSE */    
	  RCC->CR |= ((uint32_t)RCC_CR_HSEON);
	 
	  /* Wait till HSE is ready and if Time out is reached exit */
	  do
	  {
	    HSEStatus = RCC->CR & RCC_CR_HSERDY;
	    StartUpCounter++;  
	  } while((HSEStatus == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT));

	  if ((RCC->CR & RCC_CR_HSERDY) != RESET)
	  {
	    HSEStatus = (uint32_t)0x01;
	  }
	  else
	  {
	    HSEStatus = (uint32_t)0x00;
	  }  

	  if (HSEStatus == (uint32_t)0x01)
	  {
	    /* HCLK = SYSCLK */
	    RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;
	      
	    /* PCLK2 = HCLK */
	    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1;
	    
	    /* PCLK1 = HCLK */
	    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV1;
	    
	    /*  PLL configuration:  = (HSE / 2) * 6 = 24 MHz */
	    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL));
	    RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_PREDIV1 | RCC_CFGR_PLLXTPRE_PREDIV1_Div2 | RCC_CFGR_PLLMULL6);

	    /* Enable PLL */
	    RCC->CR |= RCC_CR_PLLON;

	    /* Wait till PLL is ready */
	    while((RCC->CR & RCC_CR_PLLRDY) == 0)
	    {
	    }

	    /* Select PLL as system clock source */
	    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
	    RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;    

	    /* Wait till PLL is used as system clock source */
	    while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x08)
	    {
	    }
	  }
	  else
	  { /* If HSE fails to start-up, the application will have wrong clock 
	         configuration. User can add here some code to deal with this error */
	  } 
	}

这两个函数都是通过读写RCC组的寄存器来做基础的时钟配置的，具体要读懂每一次读写，需要去看详细的芯片数据手册了。我们先暂且放过，但是，这里的RCC是什么？
在stm32f10x.h里（这就是为什么它要include这个头文件），我们找到了：

	#define RCC                 ((RCC_TypeDef *) RCC_BASE)

而这个`RCC_BASE`:

	#define RCC_BASE              (AHBPERIPH_BASE + 0x1000)

而这个`AHBPERIPH_BASE`：

	#define AHBPERIPH_BASE        (PERIPH_BASE + 0x20000)

而这个`PERIPH_BASE`：

	#define PERIPH_BASE           ((uint32_t)0x40000000)

好嘛，绕了一大圈，RCC是RCC组寄存器的第一个寄存器的地址0x40021000，那么`RCC_TypeDef`是什么？

	typedef struct
	{
	  __IO uint32_t CR;
	  __IO uint32_t CFGR;
	  __IO uint32_t CIR;
	  __IO uint32_t APB2RSTR;
	  __IO uint32_t APB1RSTR;
	  __IO uint32_t AHBENR;
	  __IO uint32_t APB2ENR;
	  __IO uint32_t APB1ENR;
	  __IO uint32_t BDCR;
	  __IO uint32_t CSR;

	  uint32_t RESERVED0;
	  __IO uint32_t CFGR2;
	} RCC_TypeDef;

看到了吧，就是这一组寄存器，所以，`SystemInit`的第一句：

	RCC->CR |= (uint32_t)0x00000001;

就是CR寄存器或上了1。