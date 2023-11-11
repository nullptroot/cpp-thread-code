#include <atomic>
#include <memory>
#include <unordered_map>

/*采用atomix_flag实现自旋锁，
atomix_flag只能且必须由ATOMIC_FLAG_INIT初始化为置零状态
且atomix_flag，仅由两个操作，clear置零，test_and_set,获取原值
并设置有效

下面是用atomix_flag实现的自旋锁，刚开始加锁的时候会返回0，并置一
后续再调用lock就阻塞住了，只能等待unlock释放*/
class spinlock_mutex
{
        std::atomic_flag flag;
    public:
        spinlock_mutex():flag(ATOMIC_FLAG_INIT){};
        void lock()
        {
            while(flag.test_and_set(std::memory_order_acquire));
        }
        void unlock()
        {
            flag.clear(std::memory_order_release);
        }
};
int main()
{
    std::atomic<bool> b;
    /*获取原址 载入操作*/
    bool x = b.load(std::memory_order_acquire);
    /*存储操作*/
    b.store(true);
    /*获取原值，并设置false*/
    x = b.exchange(false,std::memory_order_acq_rel);

    bool expected = false;
    /*compare_exchange_weak 表示如果expectd和b值相等就把b置为true，
    否则就把expectd置为b的值，如果不成功会佯败，就是不相等，也没有改变expectd的值*/
    /*下面compare_exchange_weak 成功和失败内存次序可设置的范围是不一样的，
    因此需要设置两种，或采用默认，失败没有存储操作，不能比成功次序严格*/
    while(!b.compare_exchange_weak(expected,true) && !expected);
    int a[5];
    std::atomic<int *> p(a);
    /*整形和指针可以有下面的函数，可以指定内存次序*/
    auto x = p.fetch_add(2);/*返回旧址，也就是p[0]*/
    x = (p-=1);/*返回新址，也就是p[1]*/
    std::unordered_map<int,std::atomic<int>> maps;

    /*对shared_ptr有非成员函数的原子操作函数*/
    std::shared_ptr<int> other;

    std::shared_ptr<int> local = std::atomic_load(&other);

    std::atomic_store(&other,local);

}