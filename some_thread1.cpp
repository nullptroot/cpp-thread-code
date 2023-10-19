#include <iostream>
#include <thread>
using std::thread;
using std::cout;
void f()
{
	//while(1)
		//std::cout<<"a";
	cout<<std::this_thread::get_id();
	cout<<"\n";
}
void function()
{
	std::cout<<"I am a thread\n";
	cout<<std::this_thread::get_id();
	cout<<"\n";
	thread My(f);
	My.join();

}
int main()
{
	thread MyThread(function);
	MyThread.join();
	//while(1)
	{

	};
	return 0;
}
