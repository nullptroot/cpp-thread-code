#include <chrono>
#include <future>
#include <iostream>
using namespace std::chrono_literals;
/*可以直接这样声明*/
auto one_day = 24h;
auto half_an_hour = 30min;
auto max_time_between_messages = 30ms;

/*高向低 直接可以隐式转换，低到高需要显示转换
duration_cast既是显示转换*/
std::chrono::milliseconds msb(54802);
std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(msb);
auto a = msb.count();/*a == 54802*/

int main()
{
    auto some_task = [](){return 2;};
    std::future<int> f = std::async(some_task);
    /*下面future f等待35毫秒  准备就绪下面的if条件句就成立*/
    if(f.wait_for(std::chrono::milliseconds(35)) == std::future_status::ready)
    {
        std::cout<<f.get();
    }
}
