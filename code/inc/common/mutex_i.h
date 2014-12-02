/**
 * @Function:
 *  Define the interface related to mutex
 * @Memo:
 *  Created by hill, 4/17/2014
 **/
#ifndef __VP_MUTEX_I_INCLUDE_H__
#define __VP_MUTEX_I_INCLUDE_H__

class ILockable
{
protected:
    ILockable() {};

public:
    virtual ~ILockable() {};
    virtual void lock() = 0;
    virtual void unlock() = 0;
    virtual void readlock() { lock(); }
    virtual void writelock() { lock() ; }
    virtual void init() {}
};


#endif
