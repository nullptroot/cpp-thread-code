#include <mutex>
#include <thread>
std::mutex m_mutex;

// std::lock_guard<std::mutex> get_lock()
// {
// 	std::lock_guard<std::mutex> lock(mutex);
// 	return lock;
// }
/*unique_lock有移动语义，可以转换管理权，赋值和复制都是删除函数
因此下面的函数是对的，上面的函数是不对的，因为没有复制和赋值时
删除函数，且没有移动语义*/

std::lock_guard<std::mutex> s;
std::unique_lock<std::mutex> get_lock()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	return lock;
}
/*unique_lock 比较灵活，不是构造完就锁上了（lock_guard），锁上后
还可以解锁 如下代码*/
int main()
{
	std::unique_lock<std::mutex> my_lock(m_mutex);
	/*...   处理共享数据 因此加上锁了*/

	my_lock.unlock();
	/*... 下面的操作不用加锁了 就可以先unlock*/

	my_lock.lock();
	/*又需要加锁了，在加锁*/

	return 0;
}