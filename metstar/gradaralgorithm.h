#pragma once

#include <QRect>
#include "gradarvolume.h"
#include "gradialbody.h"

class vtkColorTransferFunction;

// 定义图像无效值
constexpr double INVALID_VALUE = -1000.0;

class GRadarAlgorithm
{
public:
    // 不生成的一些无用函数, 减少程序大小
    GRadarAlgorithm() = delete;
    GRadarAlgorithm(const GRadarAlgorithm&) = delete;
    GRadarAlgorithm(const GRadarAlgorithm&&) = delete;
    ~GRadarAlgorithm() = delete;

    /*
    * 两个经纬度计算圆心角
    *
    * arcLen: 电磁波弧长
    * el: 电磁波发射仰角, 单位是弧度
    * return: 弦长
    */
    //static double centerRadian(const double lon1, const double lat1, const double lon2, const double lat2);

    /*
    * 把网格上相对于雷达的坐标(gx, gy, gx)转为经纬度(lon, lat)
    *
    * gx, gy, gx: 网格上相对于雷达的坐标
    * radarLon, radarLat, radarElev: 网格上相对于雷达经纬度和海拔
    * lon, lat: 计算得到的经纬度
    */
    static void gridToLonLat(
        const double gx, const double gy, const double gz,
        const double radarLon, const double radarLat, const double radarElev,
        double* lon, double* lat);

    /*
    * 给定经纬度(lon, lat)和gz 转为网格上相对于雷达的坐标(gx, gy)
    *
    * lon, lat: 经纬度
    * gz: 网格上相对于雷达的z坐标
    * radarLon, radarLat, radarElev: 网格上相对于雷达经纬度(角度)和海拔
    * gx, gy: 网格上相对于雷达的x, y坐标
    */
    static void GRadarAlgorithm::lonLatToGrid(
        const double lon, const double lat, const double gz,
        const double radarLon, const double radarLat, const double radarElev,
        double* gx, double* gy);

    /*
    * 标准大气下, 将雷达电磁波弧长转为弦长
    *
    * arcLen: 电磁波弧长
    * el: 电磁波发射仰角, 单位是弧度
    * return: 弦长
    */
    static double arcToChord(const double arcLen, const double el);


    /*
    * 把锥面坐标转换为墨卡托投影后的坐标
    *
    * surface: 要转换的锥面
    * longitude: 雷达经度 角度
    * latitude: 雷达维度 角度
    * elev: 雷达海拔高度
    *
    * return 转换后的坐标
    */
    static void surfToMercator(GRadialSurf* surface,
                               const double longitude, const double latitude,
                               const double elev);

    /*
    * 计算锥面坐标转换墨卡托投影后的坐标范围
    *
    * surface: 要计算的锥面
    * longitude: 雷达经度 角度
    * latitude: 雷达维度 角度
    * elev: 雷达海拔高度
    * return 转换后的坐标范围
    */
    static QRectF GRadarAlgorithm::calcMercatorBound(const GRadialSurf* surface,
                                                    const double longitude, const double latitude,
                                                    const double elev);

    /*
    * 在径向上进行平滑 平滑核大小为5
    */
    static void smoothRadial(GRadarVolume* body);

    /*
    * 在径向上对数插值
    */
    static double interplateElRadialData(const GRadialSurf* pSurf, const double az, const double r, const double invalid);

    /*
    * 使用OMP库利用CPU多线程运算
    *
    * surface: 要转换的锥面
    * width height: 生成图像宽高
    * longitude latitude: 雷达经纬度 角度
    * elev: 雷达海报高度
    * bound: 锥面墨卡托投影后的边界（可通过surfToMercator函数获取边界）
    *
    * points: 输出指针, 需要调用者分配内存空间, 一维数组, 长度为 width*height, 用于保存插值后的点
    */
    static void interpolateImageOMP(const GRadialSurf* surface, const size_t width, const size_t height,
                                    const double longitude, const double latitude, const double elev,
                                    const QRectF& bound, vtkColorTransferFunction* mColorTF,
                                    double* points, uchar* imageData);
};

