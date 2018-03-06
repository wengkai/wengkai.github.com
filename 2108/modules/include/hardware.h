#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include <asm/io.h>
#include <asm/arch/hardware.h>
#include <asm/hardware/clps7111.h>

#define SERSEL	0x00000001
#define SYSCON3	0x2200
#define DAISEL	0x00000008
#define CDENTX	0x00002000  /* Codec interface enable Tx */
#define CDENRX	0x00004000  /* Codec interface enable Rx */

#endif