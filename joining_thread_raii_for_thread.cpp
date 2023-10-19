#include <thread>
#include <iostream>
#include <unistd.h>
#include <exception>
using std::cout;
using std::endl;
using std::thread;
/*一个管理 线程的类*/
class joining_thread
{
    private:
        thread t;
    public:
        joining_thread() noexcept = default;
        template<typename Callable,typename ... Args>
        explicit joining_thread(Callable &&func,Args && ... args):t(std::forward<Callable>(func),std::forward<Args>(args)...){}
        explicit joining_thread(thread t_) noexcept :t(std::move(t_)){}
        joining_thread(joining_thread &&other) noexcept:t(std::move(other.t)){};
        joining_thread &operator=(joining_thread &&other) noexcept
        {
            if(joinable())
                join();
            /*类成员函数可以访问任何同类对象的私有成员
            因为其他类调用成员函数和本类调用的成员函数
            调用的是一个*/
            t = std::move(other.t);
            return *this;

        }
        joining_thread &operator=(thread other) noexcept
        {
            if(joinable())
                join();
            t = std::move(other);
            return *this;

        }
        ~joining_thread()
        {
            t.join();
        }
        void swap(joining_thread &other) noexcept
        {
            t.swap(other.t);
        }
        thread::id get_id() const noexcept{return t.get_id();};
        void join(){t.join();};
        void detach(){t.detach();};
        bool joinable() const noexcept
        {
            return t.joinable();
        }
        thread & as_thread() noexcept
        {
            return t;
        }
        const thread & as_thread() const noexcept
        {
            return t;
        }
};
void func()
{
    while(1)
    {
        sleep(1);
        cout<<"runing"<<endl;
    }
}

int main()
{
    thread t(func);
    joining_thread t1(std::move(t));
    joining_thread t2 = std::move(t1);
    // t2.t.detach();
    return 0;
}