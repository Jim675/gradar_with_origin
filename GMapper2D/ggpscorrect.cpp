#include "ggpscorrect.h"
#include "gmath.h"
#include <cstdio>

double GGPSCorrect::A = 6378245.0;
double GGPSCorrect::EE = 0.00669342162296594323;

bool GGPSCorrect::outOfChina(double lon, double lat)
{
    if (lon < 72.004 || lon > 137.8347)
        return true;
    if (lat < 0.8293 || lat > 55.8271)
        return true;
    return false;
}

double GGPSCorrect::transformLat(double x, double y)
{
    double ret = -100.0 + 2.0 * x + 3.0 * y + 0.2 * y * y + 0.1 * x * y + 0.2 * sqrt(fabs(x));
    ret += (20.0 * sin(6.0 * x * PI) + 20.0 * sin(2.0 * x * PI)) * 2.0 / 3.0;
    ret += (20.0 * sin(y * PI) + 40.0 * sin(y / 3.0 * PI)) * 2.0 / 3.0;
    ret += (160.0 * sin(y / 12.0 * PI) + 320 * sin(y * PI / 30.0)) * 2.0 / 3.0;
    return ret;
}

double GGPSCorrect::transformLon(double x, double y)
{
    double ret = 300.0 + x + 2.0 * y + 0.1 * x * x + 0.1 * x * y + 0.1 * sqrt(fabs(x));
    ret += (20.0 * sin(6.0 * x * PI) + 20.0 * sin(2.0 * x * PI)) * 2.0 / 3.0;
    ret += (20.0 * sin(x * PI) + 40.0 * sin(x / 3.0 * PI)) * 2.0 / 3.0;
    ret += (150.0 * sin(x / 12.0 * PI) + 300.0 * sin(x / 30.0 * PI)) * 2.0 / 3.0;
    return ret;
}

// WGS84×ª»»Îª»ðÐÇ×ø±ê(¼ÓÆ«)
void GGPSCorrect::wgsTogcj2(double wgLon, double wgLat, double& lon, double& lat)
{
    if (outOfChina(wgLon, wgLat)) {
        lat = wgLat;
        lon = wgLon;
        return;
    }
    double dLat = transformLat(wgLon - 105.0, wgLat - 35.0);
    double dLon = transformLon(wgLon - 105.0, wgLat - 35.0);
    double radLat = wgLat / 180.0 * PI;
    double magic = sin(radLat);
    magic = 1 - EE * magic * magic;
    double sqrtMagic = sqrt(magic);
    dLat = (dLat * 180.0) / ((A * (1 - EE)) / (magic * sqrtMagic) * PI);
    dLon = (dLon * 180.0) / (A / sqrtMagic * cos(radLat) * PI);
    lat = wgLat + dLat;
    lon = wgLon + dLon;
}

// »ðÐÇ×ø±ê×ª»»ÎªWGS84(¾ÀÆ«)
void GGPSCorrect::gcj2Towgs(double lon, double lat, double& wgLon, double& wgLat)
{
    if (outOfChina(lon, lat)) {
        wgLon = lon;
        wgLat = lat;
        return;
    }

    int nstep = 0;
    double wgLon0, wgLat0;
    static double eps = 1e-8;
    double epsLon, epsLat;
    wgLon0 = lon;
    wgLat0 = lat;

    do {
        double dLat = transformLat(wgLon0 - 105.0, wgLat0 - 35.0);
        double dLon = transformLon(wgLon0 - 105.0, wgLat0 - 35.0);
        double radLat = wgLat0 / 180.0 * PI;
        double magic = sin(radLat);
        magic = 1 - EE * magic * magic;
        double sqrtMagic = sqrt(magic);
        dLat = (dLat * 180.0) / ((A * (1 - EE)) / (magic * sqrtMagic) * PI);
        dLon = (dLon * 180.0) / (A / sqrtMagic * cos(radLat) * PI);

        wgLon = lon - dLon;
        wgLat = lat - dLat;
        epsLon = fabs(wgLon - wgLon0);
        epsLat = fabs(wgLat - wgLat0);
        wgLon0 = wgLon;
        wgLat0 = wgLat;
        nstep++;
        //printf("epsLon = %.8f, epsLat=%.8f\n", epsLon, epsLat);
    } while ((epsLon > eps || epsLat > eps) && nstep < 100);

    if (nstep >= 100) {
        printf("gcj2Towgs Error!\n");
    }
}
