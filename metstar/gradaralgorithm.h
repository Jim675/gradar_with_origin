#pragma once

#include <QRect>
#include "gradarvolume.h"
#include "gradialbody.h"

class vtkColorTransferFunction;

// ����ͼ����Чֵ
constexpr double INVALID_VALUE = -1000.0;

class GRadarAlgorithm
{
public:
    // �����ɵ�һЩ���ú���, ���ٳ����С
    GRadarAlgorithm() = delete;
    GRadarAlgorithm(const GRadarAlgorithm&) = delete;
    GRadarAlgorithm(const GRadarAlgorithm&&) = delete;
    ~GRadarAlgorithm() = delete;

    /*
    * ������γ�ȼ���Բ�Ľ�
    *
    * arcLen: ��Ų�����
    * el: ��Ų���������, ��λ�ǻ���
    * return: �ҳ�
    */
    //static double centerRadian(const double lon1, const double lat1, const double lon2, const double lat2);

    /*
    * ��������������״������(gx, gy, gx)תΪ��γ��(lon, lat)
    *
    * gx, gy, gx: ������������״������
    * radarLon, radarLat, radarElev: ������������״ﾭγ�Ⱥͺ���
    * lon, lat: ����õ��ľ�γ��
    */
    static void gridToLonLat(
        const double gx, const double gy, const double gz,
        const double radarLon, const double radarLat, const double radarElev,
        double* lon, double* lat);

    /*
    * ������γ��(lon, lat)��gz תΪ������������״������(gx, gy)
    *
    * lon, lat: ��γ��
    * gz: ������������״��z����
    * radarLon, radarLat, radarElev: ������������״ﾭγ��(�Ƕ�)�ͺ���
    * gx, gy: ������������״��x, y����
    */
    static void GRadarAlgorithm::lonLatToGrid(
        const double lon, const double lat, const double gz,
        const double radarLon, const double radarLat, const double radarElev,
        double* gx, double* gy);

    /*
    * ��׼������, ���״��Ų�����תΪ�ҳ�
    *
    * arcLen: ��Ų�����
    * el: ��Ų���������, ��λ�ǻ���
    * return: �ҳ�
    */
    static double arcToChord(const double arcLen, const double el);


    /*
    * ��׶������ת��Ϊī����ͶӰ�������
    *
    * surface: Ҫת����׶��
    * longitude: �״ﾭ�� �Ƕ�
    * latitude: �״�ά�� �Ƕ�
    * elev: �״ﺣ�θ߶�
    *
    * return ת���������
    */
    static void surfToMercator(GRadialSurf* surface,
                               const double longitude, const double latitude,
                               const double elev);

    /*
    * ����׶������ת��ī����ͶӰ������귶Χ
    *
    * surface: Ҫ�����׶��
    * longitude: �״ﾭ�� �Ƕ�
    * latitude: �״�ά�� �Ƕ�
    * elev: �״ﺣ�θ߶�
    * return ת��������귶Χ
    */
    static QRectF GRadarAlgorithm::calcMercatorBound(const GRadialSurf* surface,
                                                    const double longitude, const double latitude,
                                                    const double elev);

    /*
    * �ھ����Ͻ���ƽ�� ƽ���˴�СΪ5
    */
    static void smoothRadial(GRadarVolume* body);

    /*
    * �ھ����϶�����ֵ
    */
    static double interplateElRadialData(const GRadialSurf* pSurf, const double az, const double r, const double invalid);

    /*
    * ʹ��OMP������CPU���߳�����
    *
    * surface: Ҫת����׶��
    * width height: ����ͼ����
    * longitude latitude: �״ﾭγ�� �Ƕ�
    * elev: �״ﺣ���߶�
    * bound: ׶��ī����ͶӰ��ı߽磨��ͨ��surfToMercator������ȡ�߽磩
    *
    * points: ���ָ��, ��Ҫ�����߷����ڴ�ռ�, һά����, ����Ϊ width*height, ���ڱ����ֵ��ĵ�
    */
    static void interpolateImageOMP(const GRadialSurf* surface, const size_t width, const size_t height,
                                    const double longitude, const double latitude, const double elev,
                                    const QRectF& bound, vtkColorTransferFunction* mColorTF,
                                    double* points, uchar* imageData);
};

