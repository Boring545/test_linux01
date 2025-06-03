#include <iostream>
#include <thread>

namespace test3
{

    void hello(const int &x)
    {
        std::cout << "hello thread     " <<&x<< std::endl;
    }

}
int main()
{
    int x=0;
    std::cout<<&x<<std::endl;
    std::thread t{test3::hello,x};
    auto n = std::thread::hardware_concurrency();
    try{

    }catch(...){
        t.join();
        throw;
    }

    std::cout<<"SUPPORT THREAD NUM="<<n;
    t.join();


    std::cout<<std::this_thread::get_id()<<std::endl;
    std::thread t2{[]{
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::cout<<std::this_thread::get_id()<<std::endl;
    }};
    t2.join();

    return 0;
}