#pragma once

#include <iostream>
#include <string>

// 时间类
class Timestamp
{
public:
    Timestamp();
    // explicit禁止构造函数进行隐式转换对象
    explicit Timestamp(int64_t microSecondsSinceEpoch); 

    static Timestamp now(); 
    
    std::string toString() const; // 常方法，只读方法
private:
    int64_t microSecondsSinceEpoch_;
};