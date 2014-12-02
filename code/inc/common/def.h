/**
* @Function:
*  This file define some const variables.
* @Memo:
*  Created by hill, 4/11/2014
**/

#ifndef __VP_DEF_INCLUDE_H__
#define __VP_DEF_INCLUDE_H__

#include "platform.h"

#define MAXPATH                 1024
#define SECTOR_ALIGNMENT        512
#define PAGE_SIZE               4096
#define CHUNK_ALIGNMENT_MASK    (~(SECTOR_ALIGNMENT - 1))
#define MY_INT32_MAX            0x7FFFFFFF
#define MY_INT64_MAX            0x7FFFFFFFFFFFFFFF


const u_int64 ONE_KB = 1024;
const u_int64 ONE_MB = ONE_KB * ONE_KB;
const u_int64 ONE_GB = ONE_KB * ONE_MB;
const u_int64 ONE_TB = ONE_MB * ONE_MB;
const u_int64 ONE_THOUSAND  = 1000;
const u_int64 ONE_MILLION   = 1000000;
const u_int64 NS_PER_SECOND = 1000000000;
const u_int64 MS_PER_SECOND = ONE_THOUSAND;


#endif

