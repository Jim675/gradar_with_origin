#include "gmapcoordconvert.h"
#include "gmapconstant.h"

#include <cmath>

// 经纬度(角度)转Web墨卡托
void GMapCoordConvert::lonLatToMercator(double lon, double lat, double* mx, double* my)
{
    double x = lon * EHC / 180.0;
    double y = log(tan((90.0 + lat) * PI / 360.0)) / PI * EHC;
    *mx = x;
    *my = y;
}

//Web墨卡托转经纬度(角度)
void GMapCoordConvert::mercatorToLonLat(double mx, double my, double* lon, double* lat)
{
    double x = mx / EHC * 180.0;
    double y = atan(exp(my * PI / EHC)) / PI * 360.0 - 90.0;
    *lon = x;
    *lat = y;
}
