#include <atomic>
#include <thread>
#include <assert.h>
std::atomic<bool> x,y;
std::atomic<int> z;

void write_x()
{
    x.store(true,std::memory_order_seq_cst);
}

void write_y()
{
    y.store(true,std::memory_order_seq_cst);
}

void read_x_then_y()
{
    /*如果y.load返回false，那么x的存储操作先于y，下面那个函数read_y_then_x  x.load就为true
    因为只有y存储完后，才会退出死循环进行if判断，y存储完后x肯定已经存储完了
    如果y.load返回true，和前面一样的只不过这次z加了两次 这里这个函数如果有下面的if判断
    那必然是x已经存储完了  下面那个read_y_then_x函数是一样的道理
    这两个函数确定了其中一个存储顺序先于另一个存储顺序，本函数是
    x先于y  下面的函数时y先于x，总有一个是对的，那么z就肯定不为0了
    
    两个存储操作在进行if判断前都存储完了z就是2*/
    while(!x.load(std::memory_order_seq_cst));
    if(y.load(std::memory_order_seq_cst))
        ++z;
}
void read_y_then_x()
{
    while(!y.load(std::memory_order_seq_cst));
    if(x.load(std::memory_order_seq_cst))
        ++z;
}
/*先后一致顺序要求全局总操作序列一致
下面的断言一定不会发生 z不可能等于0，
这里x和y的存储操作肯定先行关系发生  这里x和y存储操作不确定谁会先发生
*/
int main()
{
    x = false;
    y = false;
    z = 0;
    std::thread a(write_x);
    std::thread b(write_y);
    std::thread c(read_x_then_y);
    std::thread d(read_y_then_x);
    a.join();
    b.join();
    c.join();
    d.join();
    assert(z.load() != 0);
}