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
//    double az; // ��λ��
//    double azRadian = 0.0; // ��λ�ǻ���
//
//    double el; // ����
//    double azRadian = 0.0; // ���ǻ���
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

// ׶������
//struct GRadialSurf
//{
//    double interval; // �������
//    double el; // ƽ������
//    vector<GRadialLine*> radials; // ��������
//
//    GRadialSurf(double interval = 0.0, double el = 0.0):interval(interval), el(el)
//    {
//    }
//
//    // �ƶ����캯��
//    GRadialSurf(GRadialSurf&& surf) noexcept;
//
//    // �ƶ���ֵ����
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
//    // �Է�λ�Ǵ�С��������
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
