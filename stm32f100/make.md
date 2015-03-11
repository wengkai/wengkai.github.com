#给100C8配编译环境  

ba5ag.kai at gmail.com 2014-04-22

首先从[launchpad.net](https://launchpad.net/gcc-arm-embedded)下载最新的gcc-arm-none-eabi，我这次是`gcc-arm-none-eabi-4_8-2014q1-20140314-mac.tar.bz2`，解开后放在`/usr/local/gnuarm`下。因为解开后的目录带时间戳，所以专门做一个-s链接给它，这样以后万一有新版本出来可以方便点：

	gcc-arm-none-eabi -> gcc-arm-none-eabi-4_7-2012q4/

然后把`/usr/local/gnuarm/gcc-arm-none-eabi/bin`加到你的PATH里去。

接着从stm.com下载3.5版的外设库，具体URL太深了，而且看上去会变动的，我就不记录了。

把外设库解开，也放到`/usr/local/stm32`下。很多教程让你把这个库里的某些文件拷贝到自己的工程目录里，这不是CS的人干的事。CS的人应该把这种库啊什么的不变的东西放到`/usr/local`等高大上的地方，然后配各种参数来使用。只有和自己的工程相关的，必须修改的东西才能放自己的工程的目录。和GNUARM类似，我也给这个带版本号的目录做了个-s链接：

	library -> STM32F10x_StdPeriph_Lib_V3.5.0/

这里没有可执行的东西，都是.h、.c，不需要放PATH。

接着要做Makefile了。先来搞定单个.c文件的编译，然后再考虑如何链接，以及还有多少东西要编译链接到一起。

为了能编译单个.c文件，需要先定义两个宏：

	-DUSE_STDPERIPH_DRIVER -DSTM32F10X_MD_VL

前者表示要使用标准外设库，后者给出CPU的具体型号，MD是中等密度，指的是flash大小为64或128kB，VL指的是Value Line，就是这种F100的系列。

然后需要定义一堆的INCLUDE：

	GNUARM = /usr/local/gnuarm/gcc-arm-none-eabi
	STM = /usr/local/stm32/library

	INC1 = $(GNUARM)/arm-none-eabi/include
	INC2 = $(GNUARM)/lib/gcc/arm-none-eabi/4.7.3/include
	INC3 = $(GNUARM)/lib/gcc/arm-none-eabi/4.7.3/include-fixed
	INC4 = $(STM)/Libraries/CMSIS/CM3/CoreSupport
	INC5 = $(STM)/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x
	INC6 = $(STM)/Libraries/STM32F10x_StdPeriph_Driver/inc
	INC7 = .

最后一个很难接受，但是不得以。这是因为你必须`#include <stm32f10x.h>`，这个文件在`Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x`下，它里面有这样一段：

	#ifdef USE_STDPERIPH_DRIVER
	  #include "stm32f10x_conf.h"
	#endif

所以它要#include这个`"stm32f10x_conf.h"`，但是这个头文件是要你自己写的！这个头文件里面#include了各种外设的头文件，之所以要你自己写，实际上是要你自己改，用它给的模板，把你不用到的头文件的#include注释掉。所以这个头文件需要拷贝到自己的工程目录下，然后修改。然后，它的库里的头文件再来#include你这个头文件。这是多么地反CS的设计。但是我们的原则是不能改它的库代码，所以...，就必须再设一个到当前目录的INCLUDE。

或者...，其实这个INCLUDE和这个宏是可以去掉的。

因为在那个stm32f10x.h里，是需要`USE_STDPERIPH_DRIVER`宏才`#include "stm32f10x_conf.h"`，而这个宏只在这一个地方起作用，所以，很简单，只要不定义`USE_STDPERIPH_DRIVER`宏，然后把makefile里的`.`这个INCLUDE也去掉。在main.c里，在`#include <stm32f10x.h>`之后，自己`#include "stm32f10x_conf.h"`，就好了。