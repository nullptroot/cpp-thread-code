#include <thread>
#include <iostream>
#include <unistd.h>
#include <exception>
using std::cout;
using std::endl;
using std::thread;
/*RAII手法 对象管理资源，对象管理线程 析构自动调用join，来进行会和线程
不出异常，f会等线程执行完后才返回的，才会析构掉some_local_state*/
class scoped_thread
{
        thread t;
    public:
        explicit scoped_thread(thread t_):t(std::move(t_))
        {
            if(!t.joinable())
                throw std::logic_error("No thread");
        }
        ~scoped_thread()
        {
            t.join();
        }
        scoped_thread(scoped_thread const &) = delete;
        scoped_thread & operator=(scoped_thread const &) = delete;
};
void do_something(int i)
{
    cout<<i<<endl;
}
struct func
{
    int &i;
    func(int &i_):i(i_){};
    void operator()()
    {
        for(unsigned j = 0; j < 1000000;++j)
        {
            do_something(i);
            sleep(1);
        }
    }
};
void f()
{
    int some_local_state = 1;
    scoped_thread t{thread(func(some_local_state))};
    // sleep(10);
}
int main()
{
    f();
    cout<<"will end"<<endl;
    return 0;
}
