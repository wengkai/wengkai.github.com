/*
 * Copyright (c) 2000 Blue Mug, Inc.  All Rights Reserved.
 */

#ifndef _HERMIT_TARGET_HTYPES_H_
#define _HERMIT_TARGET_HTYPES_H_

//typedef unsigned int uint32_t;

typedef unsigned long addr_t;
//typedef unsigned long size_t;
//typedef unsigned long word_t;

/* for 32 bit target; add #ifdef here if adding 64 bit support */
#define UNALIGNED_MASK (3)

/* number of bits to right-shift for byte count => word count */
#define BYTE_TO_WORD_SHIFT (2)

#endif /* _HERMIT_TARGET_HTYPES_H_ */
