
#ifndef __BUSYBIT_H__
#define __BUSYBIT_H__

#include "mypthread.h"

namespace srch2
{
namespace instantsearch
{

class BusyBit {
private:
    mutable pthread_spinlock_t m_spinlock;
    bool busyBit;

    void setBusyBit(bool in)
    {
        pthread_spin_lock(&m_spinlock);
        this->busyBit = in;
        pthread_spin_unlock(&m_spinlock);
    }

    bool getBusyBit() const
    {
        bool out;
        pthread_spin_lock(&m_spinlock);
        out = this->busyBit;
        pthread_spin_unlock(&m_spinlock);
        return out;
    }

public:
    BusyBit()
    {
        pthread_spin_init(&m_spinlock, 0);
        this->busyBit = 0;
    }

    ~BusyBit() {
        pthread_spin_destroy(&m_spinlock);
    }

    bool isFree() const
    {
        return (getBusyBit() == 0);
    }

    bool isBusy() const
    {
        return (getBusyBit() == 1);
    }

    void setFree()
    {
        setBusyBit(0);
    }

    void setBusy()
    {
        setBusyBit(1);
    }
};

}}

#endif /* __BUSYBIT_H__ */
