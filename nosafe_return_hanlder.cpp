#include <thread>
#include <vector>
#include <iostream>
#include <unistd.h>
#include <exception>
#include <algorithm>
#include <list>
#include <mutex>
#include <string>
using std::string;
using std::mutex;
using std::list;
using std::cout;
using std::endl;
using std::thread;

class some_data
{
        int a;
        string b;
    public:
        void do_something()
        {
            a = 0;
            b = "wqwqwqw";
        }
};
class data_wrapper
{
    private:
        some_data data;
        mutex m;
    public:
        template <typename Function>
        void process_data(Function func)
        {
            std::lock_guard<mutex> l(m);
            func(data);//向使用者传递受保护共享数据
        }
};

some_data * unprotected;
void malicious_function(some_data &protected_data)
{
    unprotected = &protected_data;
}

data_wrapper x;


int main()
{
    x.process_data(malicious_function); //这个函数就把x的成员赋值给其他作用于的变量了
    unprotected->do_something();//这里就数据就不被保护了
    return 0;
}