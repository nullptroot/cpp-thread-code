#include <thread>
#include <vector>
#include <iostream>
#include <unistd.h>
#include <exception>
#include <algorithm>
#include <list>
#include <mutex>
#include <stack>
#include <string>
using std::stack;
using std::string;
using std::mutex;
using std::list;
using std::cout;
using std::endl;
using std::thread;

void do_something(int value)
{
    cout<<"do something"<<value<<endl;
}
int main()
{
    stack<int> s;
    if(s.empty())
    {
        int const value = s.top();//存在数据竞争  两个线程同时进入到了这里  都读取了栈顶  读取的内容是一样的
        s.pop();//两个线程都pop一次，导致第二个栈顶元素还未读就被pop，
        do_something(value);
    }
    return 0;
}