#include <thread>
#include <vector>
#include <iostream>
#include <unistd.h>
#include <exception>
#include <algorithm>
#include <list>
#include <mutex>
using std::mutex;
using std::list;
using std::cout;
using std::endl;
using std::thread;

list<int> some_list;
mutex some_mutex;

void add_to_list(int new_value)
{
    // std::lock_guard<std::mutex> guard(some_mutex);
    // sleep(1);
    
    some_list.push_back(new_value);
    cout<<std::this_thread::get_id()<<"  "<<"添加了一个元素 : "<<new_value<<endl;
    // sleep(1);
}
bool list_contains(int value_to_find)
{
    std::lock_guard<std::mutex> guard(some_mutex);
    // return std::find(some_list.begin(),some_list.end(),value_to_find) != some_list.end();
    for(auto i:some_list)
        cout<<i<<" ";
    cout<<endl;
    return 1;
}

int main()
{
    /*加锁会阻塞， 但是不加锁，也没有出错，
    可能是 添加和查询的冲突  擦就是不会出错*/
    std::vector<thread> threads(1000);
    // std::vector<thread> threads1(20);
    int c = 0;
    for(int i = 0; i < 1000; ++i)
    {
        threads[i] = thread(add_to_list,i);
        // threads1[i] = thread(list_contains,0);
    }
    
    for(int i = 0; i < 1000; ++i)
    {
        threads[i].detach();
        // threads1[i].detach();
    }
    sleep(3);
    cout<<some_list.size()<<endl;
    return 0;
}