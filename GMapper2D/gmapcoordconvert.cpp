#include "gmapcoordconvert.h"
#include "gmapconstant.h"

#include <cmath>

// ��γ��(�Ƕ�)תWebī����
void GMapCoordConvert::lonLatToMercator(double lon, double lat, double* mx, double* my)
{
    double x = lon * EHC / 180.0;
    double y = log(tan((90.0 + lat) * PI / 360.0)) / PI * EHC;
    *mx = x;
    *my = y;
}

//Webī����ת��γ��(�Ƕ�)
void GMapCoordConvert::mercatorToLonLat(double mx, double my, double* lon, double* lat)
{
    double x = mx / EHC * 180.0;
    double y = atan(exp(my * PI / EHC)) / PI * 360.0 - 90.0;
    *lon = x;
    *lat = y;
}
