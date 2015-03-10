#C语言的宏运算符的应用

ba5ag.kai at gmail.com 2012-12-26

在讲到编译预处理指令的宏运算符##时，同学们都不太理解这东西有什么用。今天恰好在Arduino的Ethernet库的头文件里看到了活的例子。在Arduino的Ethernet库的w5100.cpp里有这样的函数调用：

	writeTMSR(0x55);

但是遍寻整个.cpp和对应的w5100.h也找不到这个`writeTMSR()`函数，即使把所有的源代码目录拿来搜索一遍都没有。但是，编译显然是通过了的，那么，这个函数在哪里呢？

在w5100.h，我们发现了这样的代码：

	#define __GP_REGISTER8(name, address)             \
	  static inline void write##name(uint8_t _data) { \
	    write(address, _data);                        \
	  }                                               \
	  static inline uint8_t read##name() {            \
	    return read(address);                         \
	  }

这个宏定义是说，如果你提供一个name和一个address，那么宏`__GP_REGISTER8`就会被展开为两个函数，一个的名字是write后面跟上name，另一个是read后面跟上name。

于是，在w5100.h里接下去的代码：

	__GP_REGISTER8 (TMSR,   0x001B);    // Transmit memory size

在编译预处理后，就会被展开成为：

	  static inline void writeTMSR(uint8_t _data) { 
	    write(0x001B, _data);                        
	  }                                               
	  static inline uint8_t readTMSR() {            
	    return read(0x001B);                         
	  }

看，这就是活生生的宏运算符的例子。