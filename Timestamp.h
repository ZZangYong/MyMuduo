#pragma once

#include <iostream>
#include <string>

// ʱ����
class Timestamp
{
public:
    Timestamp();
    // explicit��ֹ���캯��������ʽת������
    explicit Timestamp(int64_t microSecondsSinceEpoch); 

    static Timestamp now(); 
    
    std::string toString() const; // ��������ֻ������
private:
    int64_t microSecondsSinceEpoch_;
};