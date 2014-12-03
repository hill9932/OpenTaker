#ifndef __VP_MATH_INCLUDE_H__
#define __VP_MATH_INCLUDE_H__

#include "common.h"

/**
 * @Function: return the sector size align value
 **/
size_t GetAlignedValue(size_t unaligned);

int GetRandomNumber(int _begin, int _end);

/**
 * @Function: Use KB/MB/GB to indicate the number.
 *  If is used for network, it will be kbps/mbps/gbps
 **/
CStdString ChangeUnit(u_int64 _count, u_int32 _unit, const char* _suffix, int _maxLevel = 255);

/**
 * @Function: get the x value of 2^x = value
 **/
int  Sqrt2(u_int64 _value);

/**
 * _value1 is the start value, _value2 is the stop value
 **/
u_int32 Distance(u_int32 _value1, u_int32 _value2, u_int32 _total);

#endif
