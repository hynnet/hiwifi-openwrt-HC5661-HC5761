#if !defined(__BARRIER_H__)
#define __BARRIER_H__

static inline void rmb(void)
{
    asm volatile ("" : : : "memory");
}

/*
 * wmb() Would normally need to ensure shared memory regions are marked as
 *       non-cacheable and non-bufferable, then the work to be done by wmb() is
 *       to ensure the compiler and any possible CPU out of order writes are
 *       flushed to memory, however we have no data cache and as far as I'm
 *       aware we can't use the MMU to set page properties, so in our case wmb()
 *       must cause the compiler to flush.
 */

static inline void wmb(void)
{
    // Cause the compiler to flush any registers containing pending write data
    // to memory
    asm volatile ("" : : : "memory");

}
#endif        //  #if !defined(__BARRIER_H__)
