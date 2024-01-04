#include<atomic>
class spinlock_mutex
{
    std::atomic_flag flag;
    public:
        spinlock_mutex():flag(ATOMIC_FLAG_INIT){};
        /*当首个线程调用lock时，while循环会返回false，循环退出
        当其他线程再lock时，条件就会返回true，只要没有线程
        调用unlock就会一直循环判断*/
        void lock()
        {
            while(flag.test_and_set(std::memory_order_acquire));
        }
        void unlock()
        {
            flag.clear(std::memory_order_release);
        }
};