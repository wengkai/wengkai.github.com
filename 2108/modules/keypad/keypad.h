#ifndef _KEYPAD_
#define _KEYPAD_

#define KEYPAD_MAJOR 126
#define NR_KEYS 18
//  delay time to detect key release: 500ms
#define RELEASE_DELAY (jiffies + HZ/6)
#define RECHARGE_DELAY (jiffies + HZ)
#define BUFFER_SIZE 32

extern int keypadBufLoc;
extern char* keypadBuffer;

extern wait_queue_head_t mwq;

void initInterrupt();
void keypadInterrupt(int irq, void* dev_id, struct pt_regs* regs);

int keypadInit();
void keypadClose();

void keypadPressed(unsigned int scanCode);

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif

#endif