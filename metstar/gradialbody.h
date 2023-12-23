#pragma once

#include "generic_basedata_cv.h"
#include "radar_dt.h"

#include <vector>
#include <algorithm>

//#include <vtkDoubleArray.h>
#include <qstring.h>

//using namespace std;


//struct GRadialLine
//{
//    double az; // 方位角
//    double azRadian = 0.0; // 方位角弧度
//
//    double el; // 仰角
//    double azRadian = 0.0; // 仰角弧度
//    vector<double> points;
//
//    GRadialLine(double az = 0, double el = 0)
//    {
//        this->az = az;
//        this->el = el;
//    }
//
//    void clear()
//    {
//        points.clear();
//    }
//};

// 锥面数据
//struct GRadialSurf
//{
//    double interval; // 间隔距离
//    double el; // 平均仰角
//    vector<GRadialLine*> radials; // 径向数据
//
//    GRadialSurf(double interval = 0.0, double el = 0.0):interval(interval), el(el)
//    {
//    }
//
//    // 移动构造函数
//    GRadialSurf(GRadialSurf&& surf) noexcept;
//
//    // 移动赋值函数
//    GRadialSurf& operator= (GRadialSurf&& surf) noexcept;
//
//    ~GRadialSurf()
//    {
//        for (size_t i = 0; i < radials.size(); i++) {
//            delete radials[i];
//        }
//        radials.clear();
//    }
//
//    // 对方位角从小到大排序
//    void sort();
//
//    void clear()
//    {
//        for (size_t i = 0; i < radials.size(); i++) {
//            delete radials[i];
//        }
//        radials.clear();
//    }
//};
