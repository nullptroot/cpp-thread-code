#include <thread>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
void do_something(int &i)
{
    std::cout<<i<<std::endl;
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
            sleep(3);
        }
    }
};
void oops()
{
    int some_local_state = 0;
    func myFunc(some_local_state);
    func myFunc1(some_local_state);
    std::thread myThread(myFunc);
    std::thread myThread1(myFunc1);
    myThread.detach();
    myThread1.detach();
    // sleep(1);
}
int main()
{
    // oops();
    std::cout<<"sss"<<std::endl;
    int a;
    int some_local_state = 0;
    
    func myFunc(some_local_state);
    std::thread myThread(myFunc);
    myThread.detach();
    std::cin>>a;
    return 0;
}