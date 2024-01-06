#pragma once

#include "generic_basedata_cv.h"
#include "radar_dt.h"
#include <QVector>
#include <vector>
#include <string>

#include <qstring.h>
#include <qrect.h>

using std::vector;
using std::string;

// 径向数据线上一个点的数据
class GRadialPoint
{
public:
    double x = 0.0;         // 存储二维绘图时的Web墨卡托X坐标
    double y = 0.0;         // 存储二维绘图时的Web墨卡托Y坐标
    double value = 0.0;     // 雷达扫描值

    GRadialPoint(double value) :x(0.0), y(0.0), value(value)
    {

    }
};

// 一根径向数据线上的数据
class GRadialLine
{
public:
    double el = 0.0;                    // 仰角角度
    double elRadian = 0.0;              // 仰角弧度 π=180° π/2=90°

    double az = 0.0;                    // 方位角角度
    double azRadian = 0.0;              // 方位角弧度

    vector<GRadialPoint> points;        // 雷达扫描到的数据点

    GRadialLine() = default;

    // 移动构造函数
    GRadialLine(GRadialLine&& line) noexcept;

    // 移动赋值函数
    GRadialLine& operator= (GRadialLine&& surf) noexcept;
};

// 锥面数据
class GRadialSurf
{
public:
    double interval = 0.0;              // 分辨率,每个数据点的间隔
    double el = 0.0;                    // 平均仰角角度
    double elRadian = 0.0;              // 平均仰角弧度
    vector<GRadialLine*> radials;       // 径向数据

    bool isConvert = false; // 坐标系是否是Web墨卡托坐标
    QRectF bound; // 锥面Web墨卡托范围边界

    GRadialSurf() = default;

    // 移动构造函数
    GRadialSurf(GRadialSurf&& surf) noexcept;

    // 移动赋值函数
    GRadialSurf& operator= (GRadialSurf&& surf) noexcept;

    ~GRadialSurf();

    void sort();

    void clear();
};


// 雷达数据体
class GRadarVolume
{
public:
    QString siteCode; // 站点编号
    QString siteName; // 站点名称
    QString startTime; // 扫描开始时间

    int surfNum = 0; // 扫描锥面数量

    double longitude = 0; // 站点经度
    double latitude = 0; // 站点纬度
    double elev = 0; // 站点海拔高度

    double minEl = 0; // 最小仰角
    double maxEl = 0; // 最大仰角
    int	maxPointCount = 0; // 最大采样点数
    QVector<GRadialSurf*> surfs; // 锥面列表

    QRect bound; // Web墨卡托范围边界
    QString path; // 文件路径


    GRadarVolume() = default;

    ~GRadarVolume();

    //// 抽取信息
    //void extractInfo(const basedataImage& bdi);

    // 抽取指定的数据
    void extractData(const basedataImage& bdi, int dataType, double invalidValue);

    //按照预测volume修改最后一帧雷达数据
    void modifyData(basedataImage& bdi, int dataType);

    // 统计数据体范围
    void calcBoundBox(double& minX, double& minY, double& minZ, double& maxX, double& maxY, double& maxZ) const;

    // 等效地球半径法网格化
    //void gridding1(double x0, double y0, double z0,
    //               int nx, int ny, int nz,
    //               double dx, double dy, double dz,
    //               double invalidValue, double* output) const;

    // 考虑大气折射后的网格化
    void gridding(double x0, double x1,
        double y0, double y1,
        double z0, double z1,
        int nx, int ny, int nz,
        double invalidValue, float* output) const;

    // 探测考虑大气折射后的最高有效Z坐标
    double detectMaxValidZ(double x0, double x1,
        double y0, double y1,
        double z0, double z1,
        int nx, int ny, int nz,
        double min, double max) const;

    void clear();
    // 生成新的锥面
    GRadialSurf* generateNewSurf(vtkImageData* imageData);
private:

    //// 填充径向数据
    //void fillRadialData(GRadialLine* pData, cv_geneMom& mom, double invalidValue);

    // 二分查找仰角区间
    int findElRange(double el) const;

    // 顺序查找仰角区间
    int findElRange1(double el) const;

    // 径向数据插值
    double interplateRadialData(double el, double az, double d, double invalidValue) const;

    // 插值同一仰角面上指定方位角和距离的数值
    double interplateElRadialData(int isurf, double az, double d, double invalidValue) const;
};

