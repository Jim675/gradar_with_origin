#ifndef GPS_CORRECT_H
#define GPS_CORRECT_H

#include "gmapper2d_global.h"

// GPS经纬度加偏和纠偏坐标转换类
class GMAPPER2D_EXPORT GGPSCorrect
{
private:
    static double A;
    static double EE;

public:
    // WGS84转换为火星坐标(加偏)
    static void wgsTogcj2(double wgLon, double wgLat, double& lon, double& lat);

    // 火星坐标转换为WGS84(纠偏)
    static void gcj2Towgs(double lon, double lat, double& wglon, double& wglat);

private:
    static bool outOfChina(double lon, double lat);

    static double transformLat(double x, double y);

    static double transformLon(double x, double y);
};

#endif