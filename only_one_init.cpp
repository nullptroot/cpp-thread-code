#include <mutex>
#include <iostream>
#include <memory>
std::unique_ptr<int>


std::shared_ptr<int> resource_ptr;
std::once_flag resource_flag; //仅初始化时加锁

void init_resource()
{
    std::cout<<"sasasa\n";
    resource_ptr.reset(new int);
}
int main()
{
    std::call_once(resource_flag,init_resource);
    std::call_once(resource_flag,init_resource);
    /*上面的函数只调用一次  且线程安全
    call_once可以传递成员函数，然后第三个参数传递this指针*/
    /*所有线程调用此函数可以保证只初始化一次*/
    return 0;
}