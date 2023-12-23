#ifndef GPS_CORRECT_H
#define GPS_CORRECT_H

#include "gmapper2d_global.h"

// GPS��γ�ȼ�ƫ�;�ƫ����ת����
class GMAPPER2D_EXPORT GGPSCorrect
{
private:
    static double A;
    static double EE;

public:
    // WGS84ת��Ϊ��������(��ƫ)
    static void wgsTogcj2(double wgLon, double wgLat, double& lon, double& lat);

    // ��������ת��ΪWGS84(��ƫ)
    static void gcj2Towgs(double lon, double lat, double& wglon, double& wglat);

private:
    static bool outOfChina(double lon, double lat);

    static double transformLat(double x, double y);

    static double transformLon(double x, double y);
};

#endif