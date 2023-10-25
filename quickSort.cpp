#include <iostream>
#include <list>
#include <algorithm>

template <typename T>
std::list<T> sequential_quick_sort(std::list<T> input)
{
    if(input.empty())
        return input;
    std::list<T> result;
    /*从input链表中分离出第一个元素，赋给result的第一个元素
    作为比较的基准*/
    result.splice(result.begin(),input,input.begin());
    T const &pivot = *result.begin();
    /*partition操作*/
    auto divide_point = std::partition(input.begin(),input.end(),[&](T const &t){return t < pivot;});
    std::list<T> lower_part;
    /*把input从begin到分割点divide_point分离给lower_part*/
    lower_part.splice(lower_part.end(),input,input.begin(),divide_point);
    /*递归的去做partition，分割*/
    auto new_lower(sequential_quick_sort(std::move(lower_part)));
    auto new_higher(sequential_quick_sort(std::move(input)));
    /*把分割后的两个串进行拼接，大串放后面，小串放前面*/
    result.splice(result.end(),new_higher);
    result.splice(result.begin(),new_lower);
    return result;
}

int main()
{
    std::list<int> test;
    test.emplace_back(9);
    test.emplace_back(2);
    test.emplace_back(4);
    test.emplace_back(10);
    test.emplace_back(2);
    for(auto i:test)
        std::cout<<i<<" ";
    std::cout<<std::endl;
    auto result = sequential_quick_sort(test);
    for(auto i:result)
        std::cout<<i<<" ";
    std::cout<<std::endl;
}