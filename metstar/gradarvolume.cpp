#include "gradarvolume.h"

#include <algorithm>
#include <memory>

#include <qdatetime.h>
#include <qlocale.h>
#include <qdebug.h>

#include <gmapconstant.h>
#include <cmath>

// GQRadialLine
GRadialLine::GRadialLine(GRadialLine&& line) noexcept :
    az(line.az),
    azRadian(line.azRadian),
    points(std::move(line.points))
{
}

GRadialLine& GRadialLine::operator=(GRadialLine&& surf) noexcept
{
    this->az = surf.az;
    this->azRadian = surf.azRadian;
    this->points.clear();
    this->points = std::move(surf.points);
    return *this;
}

// GRadialSurf
GRadialSurf::GRadialSurf(GRadialSurf&& surf) noexcept :
    interval(surf.interval),
    el(surf.el),
    elRadian(surf.elRadian),
    radials(std::move(surf.radials))
{
}

GRadialSurf& GRadialSurf::operator=(GRadialSurf&& surf) noexcept
{
    clear();
    this->interval = surf.interval;
    this->el = surf.el;
    this->elRadian = surf.elRadian;
    this->radials = std::move(surf.radials);
    return *this;
}

GRadialSurf::~GRadialSurf()
{
    // 清理new出来的对象
    for (auto* points : this->radials) {
        delete points;
    }
    this->radials.clear();
}

void GRadialSurf::sort()
{
    // 对方位角从小到大排序
    std::sort(radials.begin(), radials.end(), [](const GRadialLine* a, const GRadialLine* b) {
        return a->az < b->az;
        });
}

void GRadialSurf::clear()
{
    this->interval = 0;
    this->el = 0;
    this->elRadian = 0;
    // 清理new出来的对象
    for (auto* points : this->radials) {
        delete points;
    }
    this->radials.clear();
}

void fillRadialData(GRadialLine* line, const cv_geneMom& mom, double invalidValue)
{
    const ubytes& points = mom.points;
    for (size_t i = 0; i < points.size(); i++) {
        if (points[i] < 5) {
            line->points.push_back(GRadialPoint(invalidValue));
        } else {
            double v = ((double)points[i] - mom.udt.offset) / mom.udt.scale;
            line->points.push_back(GRadialPoint(v));
        }
    }
}

GRadarVolume::~GRadarVolume()
{
    clear();
}

void GRadarVolume::extractData(const basedataImage& bdi, int dataType, double invalidValue)
{
    // 清除之前的数据
    surfs.clear();

    siteCode = QString(bdi.siteInfo.code);
    siteName = QString(bdi.siteInfo.name);
    startTime = QDateTime::fromSecsSinceEpoch(bdi.taskConf.startTime).toString("yyyy-MM-dd hh:mm:ss");
    //surfNum = bdi.taskConf.cutNum;

    longitude = bdi.siteInfo.lon;
    latitude = bdi.siteInfo.lat;
    elev = bdi.siteInfo.height;

    // 如果不是单层或者体扫就直接返回
    if (bdi.taskConf.scanType != SP_PPI && bdi.taskConf.scanType != SP_VOL) {
        return;
    }

    GRadialSurf* pSurf = nullptr;
    int icut = 0;
    minEl = 360;
    maxEl = -360;

    for (auto it = bdi.radials.begin(); it != bdi.radials.end(); ++it) {
        double el = it->el;
        if (el > 90) el -= 360;
        const double elRadian = toRadian(el);

        if (it->state == CUT_START || it->state == VOL_START) { // 仰角开始或体扫开始
            pSurf = nullptr;
            for (auto jt = it->mom.begin(); jt != it->mom.end(); ++jt) {
                if (jt->udt.type == dataType) {
                    double cel = bdi.cuts[icut].el;
                    if (cel > 90) cel -= 360;

                    pSurf = new GRadialSurf();
                    pSurf->interval = bdi.cuts[icut].dopReso;
                    pSurf->el = el;
                    pSurf->elRadian = elRadian;

                    GRadialLine* pData = new GRadialLine();
                    pData->az = it->az;
                    pData->azRadian = toRadian(it->az);
                    pData->el = cel;
                    pData->elRadian = toRadian(cel);

                    fillRadialData(pData, *jt, invalidValue);

                    pSurf->radials.push_back(pData);
                    surfs.push_back(pSurf);

                    if (minEl > el) minEl = el;
                    if (maxEl < el) maxEl = el;
                    break;
                }
            }
        } else if (it->state == CUT_END || it->state == VOL_END) { // 仰角结束或者体扫结束
            icut++;
        } else if (it->state == CUT_MID) { // 仰角中间数据
            if (pSurf) {
                for (auto jt = it->mom.begin(); jt != it->mom.end(); ++jt) {
                    if (jt->udt.type == dataType) {
                        GRadialLine* pData = new GRadialLine();
                        pData->az = it->az;
                        pData->azRadian = toRadian(it->az);
                        pData->el = el;
                        pData->elRadian = toRadian(el);

                        pSurf->radials.push_back(pData);
                        fillRadialData(pData, *jt, invalidValue);
                        break;
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < surfs.size(); i++) {
        surfs[i]->sort();
    }
    //if (surfNum != surfs.size()) {
    //    qDebug() << "Error: 锥面数量不一致: " << surfNum << ", " << surfs.size();
    surfNum = surfs.size();
    //}
}

void GRadarVolume::modifyData(basedataImage& bdi, int dataType) {

    //qDebug() << "=========================开始修改==========================";

    //判断double型数据相等时用到
    const double epsilon = 1e-9;

    for (int i = 0; i < SITE_CODE_LENGTH; i++) {
        //获取站点编号
        bdi.siteInfo.code[i] = siteCode.toLatin1().data()[i];
        //获取站点名称
        bdi.siteInfo.name[i] = siteName.toLatin1().data()[i];
    }

    //获取扫描开始时间戳
    bdi.taskConf.startTime = QDateTime::fromString(startTime, QString("yyyy-MM-dd hh:mm:ss")).toTime_t();
    //修改时间戳
    //bdi.taskConf.startTime += 10000;
    //获取站点经度
    bdi.siteInfo.lon = longitude;
    //获取站点维度
    bdi.siteInfo.lat = latitude;
    //获取站点海拔高度
    bdi.siteInfo.height = elev;

    GRadialSurf* pSurf = nullptr;
    int icut = 0;
    int isurf = 0;
    int idata = 0;

    //遍历bdi中每一份雷达数据
    for (auto it = bdi.radials.begin(); it != bdi.radials.end(); ++it) {

        //qDebug() << "=================================================";
        //qDebug() << "====================开始写入=====================";
        //qDebug() << "cuts_el_" << i << "=" << bdi.cuts.at(i).el;
        //qDebug() << "surfs_el_" << i << "=" << surfs.at(i)->el;
        //qDebug() << "cuts_az_" << i << "=" << bdi.cuts.at(i).az;
        //qDebug() << "surfs_az_" << i << "=" << surfs.at(i)->radials.at(0)->az;
        //qDebug() << "cuts_dopReso_" << i << "=" << bdi.cuts.at(i).dopReso;
        //qDebug() << "surfs_interval_" << i << "=" << surfs.at(i)->interval;

        if (it->state == CUT_START || it->state == VOL_START) {  // 仰角开始或体扫开始
            pSurf = nullptr;
            //遍历radials中每一份径向数据
            for (auto jt = it->mom.begin(); jt != it->mom.end(); ++jt) {
                if (jt->udt.type == dataType) {
                    //预测数据体volume中的锥面
                    pSurf = surfs.at(isurf++);
                    idata = 0;
                    if (pSurf->el > 90)
                        it->el = pSurf->el + 360;
                    else
                        it->el = pSurf->el;
                    //根据锥面数据更改bdi中的数据
                    bdi.cuts[icut].dopReso = pSurf->interval;  //多普勒分辨率
                    bdi.cuts[icut].az = pSurf->radials.at(idata)->az;  //方位角

                    //径向数据上的点列表
                    ubytes& points = (*jt).points;
                    //将每一个点的数据修改为预测volume中的点数据
                    for (size_t i = 0; i < points.size(); i++) {
                        //当预测volume中的点值为-1000时, 表示在提取数据是points[i] < 5, 所以直接置零不影响下次读数据
                        if (std::abs(pSurf->radials.at(idata)->points.at(i).value + 1000) < epsilon) {
                            points[i] = 0;
                        }
                        else {
                            if (pSurf->radials.at(idata)->points.at(i).value < 0) {
                                //若预测volume中的点数据值为负, 且不为-1000, 则将取正, 这样可以保证修改前后读出来的值绝对值误差小
                                //points[i] = (unsigned short)((*jt).udt.offset - pSurf->radials.at(idata)->points.at(i).value * (*jt).udt.scale);
                                points[i] = 0;
                            }
                            else
                                //其他情况则直接公式反推, 但是从double重新存回unsigned short会造成数据损失
                                points[i] = (unsigned short)((*jt).udt.offset + pSurf->radials.at(idata)->points.at(i).value * (*jt).udt.scale);
                        }
                    }
                    //雷达数据结构体中的方位角
                    it->az = pSurf->radials.at(idata)->az;
                    //仰角修改
                    if (pSurf->radials.at(idata)->el > 90)
                        bdi.cuts[icut].el = pSurf->radials.at(idata)->el + 360;
                    else
                        bdi.cuts[icut].el = pSurf->radials.at(idata)->el;
                    idata++;
                }
            }
        }
        else if (it->state == CUT_END || it->state == VOL_END) {  // 仰角结束或者体扫结束
            icut++;
        }
        else if (it->state == CUT_MID) {  // 仰角中间数据
            if (pSurf) {
                //方位角
                it->az = pSurf->radials.at(idata)->az;
                //仰角
                it->el = pSurf->radials.at(idata)->el;
                for (auto jt = it->mom.begin(); jt != it->mom.end(); ++jt) {
                    if (jt->udt.type == dataType) {
                        ubytes& points = (*jt).points;
                        for (size_t i = 0; i < points.size(); i++) {
                            if (std::abs(pSurf->radials.at(idata)->points.at(i).value + 1000) < epsilon) {
                                points[i] = 0;
                            }
                            else {
                                if (pSurf->radials.at(idata)->points.at(i).value < 0) {
                                    //points[i] = (unsigned short)((*jt).udt.offset - pSurf->radials.at(idata)->points.at(i).value * (*jt).udt.scale);
                                    points[i] = 0;
                                }
                                else
                                    points[i] = (unsigned short)((*jt).udt.offset + pSurf->radials.at(idata)->points.at(i).value * (*jt).udt.scale);
                            }

                            //qDebug() << "after_point_" << i << points[i];
                            //qDebug() << "========================================================";
                        }
                        idata++;
                    }
                }
            }
        }
    }
}

// 计算雷达数据体边界范围
void GRadarVolume::calcBoundBox(double& minX, double& maxX,
    double& minY, double& maxY,
    double& minZ, double& maxZ) const
{
    if (surfs.size() == 0) {
        minX = maxX = minY = maxY = minZ = maxZ = 0;
        return;
    }

    // el是最低仰角
    const GRadialSurf* pSurf = surfs[0];
    double el = toRadian(pSurf->el);
    // 弧长(雷达径向的点数乘以径向的间隔)
    const double arcLen = pSurf->radials[0]->points.size() * pSurf->interval;
    // TODO 未考虑大气折射
    const double r = cos(el) * arcLen;
    minX = -r; maxX = r;
    minY = -r; maxY = r;

    // el1为仰角的上半部分, 下半部分为el0, el = el0+el1
    const double el1 = arcLen * 0.5 / RN;
    // r_为雷达传播圆弧路径r的弦长
    const double r_ = 2 * RN * sin(el1);
    const double el0 = el - el1;
    minZ = r_ * sin(el0);

    // 最高仰角
    pSurf = surfs[surfs.size() - 1];
    el = toRadian(pSurf->el);
    maxZ = arcLen * sin(el);
}

// 计算并根据所在象限调整方位角[0, 2*PI]
inline double calcAzimuth(double x, double y)
{
    // atan2 输出范围为(-PI/2, PI/2]
    double azimuth = atan2(x, y);
    if (azimuth < 0) return azimuth + PI * 2;
    return azimuth;
}

GRadialSurf* GRadarVolume::generateNewSurf(vtkImageData* imageData)
{

}

// 考虑大气折射后的网格化
void GRadarVolume::gridding(double x0, double x1,
    double y0, double y1,
    double z0, double z1,
    int nx, int ny, int nz,
    double invalid, float* output) const
{
    // 计算每个层次（z轴方向）的二维平面上的数据点数。
    const int nxy = nx * ny;
    // 如果雷达数据中没有有效的径向（surfs），则将输出数组中的所有值设置为无效值（invalid）。
    if (surfs.size() <= 0)
    {
        for (int iz = 0; iz < nz; iz++)
        {
            for (int iy = 0; iy < ny; iy++)
            {
                for (int ix = 0; ix < nx; ix++)
                {
                    output[iz * nxy + iy * ny + ix] = invalid; //-1000
                }
            }
        }
        return;
    }

    // 雷达经纬度
    const double slon = toRadian(this->longitude);
    const double slat = toRadian(this->latitude);
    //double r, r_square, gz_square, gy_square;
    //double h, H, azimuth, el, d, d_square;
    // 计算在 x、y 和 z 方向上的间距。
    const double dx = (x1 - x0) / nx;
    const double dy = (y1 - y0) / ny;
    const double dz = (z1 - z0) / nz;
    // 用来保存方位角, 因为循环最外层是z轴, 而同一(x, y)方位角不变, 为了不重复计算方位角, 需要缓存
    // 创建一个二维数组 cacheAzimuth，用于缓存每个 (x, y) 点的方位角，以减少重复计算。
    vector<vector<double>> cacheAzimuth = vector<vector<double>>(ny, vector<double>(nx, 0));
    // 外层循环遍历 y 方向上的每个网格点。
    for (int iy = 0; iy < ny; iy++) {
        // 计算当前 y 方向上的网格点的位置。
        const double gy = y0 + iy * dy;
        // 获取 cacheAzimuth 中当前行的引用。
        auto& ty = cacheAzimuth[iy];
        // 内层循环遍历 x 方向上的每个网格点。
        for (int ix = 0; ix < nx; ix++) {
            // 计算当前 x 方向上的网格点的位置。
            const double gx = x0 + ix * dx;
            // 计算当前网格点的方位角，并存储在 ty 中,calcAzimuth 函数用于计算方位角，而 toAngle 函数则将弧度转换为角度。
            ty[ix] = toAngle(calcAzimuth(gx, gy));
        }
    }
    size_t index = 0;
    //bool isPrint = false;
    
// 使用 OpenMP 指令启动并行循环，以便在多个线程上并行执行循环。
#pragma omp parallel for 
    // 循环遍历 z 方向上的每个网格点。  
    for (int iz = 0; iz < nz; iz++) {
        // 计算当前 z 方向上的网格点的位置。
        const double gz = z0 + iz * dz;

        //double gz_square = gz * gz;
        //// 高度为z网格点所在平面到地心距离(考虑到大气折射, 地球半径取等效地球半径RE_而不是真实地球半径RE)
        //r = RM + this->elev + z;
        //r_square = r * r;
        //// 高度为z的中心网格点到赤道平面距离
        //// centerToPlane = r * sin(clat);
        //// 雷达高度
        //h = this->elev;

        // 内层循环遍历 y 方向上的每个网格点。
        for (int iy = 0; iy < ny; iy++) {
            // 计算当前 y 方向上的网格点的位置。
            const double gy = y0 + iy * dy;
            // 计算 y 方向上的位置的平方。
            const double gy_square = gy * gy;
            // 获取预先计算的方位角的引用。(在上面的循环中已经计算过了)
            const auto& azy = cacheAzimuth[iy];
            // 再次内层循环遍历 x 方向上的每个网格点。
            for (int ix = 0; ix < nx; ix++) {
                // 计算当前 x 方向上的网格点的位置。
                const double gx = x0 + ix * dx;
                // 计算雷达到网格点的水平距离。
                const double gl = sqrt(gx * gx + gy_square);
                // 计算仰角的上半部分。
                const double el0 = atan2(gz, gl);
                // 计算雷达到地心的距离。
                const double r_ = gl / cos(el0);
                // 计算仰角的下半部分。
                const double el1 = asin(0.5 * r_ / RN);
                // 仰角,合并上下两部分得到最终仰角。
                const double el = toAngle(el0 + el1);
                // 传播路径弧长,计算传播路径弧长。
                const double r = 2 * el1 * RN;
                // 方位角,获取方位角。
                const double azimuth = azy[ix];
                // 插值雷达数据以获取网格点的值。!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                const double v = interplateRadialData(el, azimuth, r, invalid);
                // 将插值后的值存储在输出数组中。
                output[iz * nxy + iy * ny + ix] = v;
            }
        }
    }
}

// 探测考虑大气折射后的最高有效Z坐标
double GRadarVolume::detectMaxValidZ(double x0, double x1,
    double y0, double y1,
    double z0, double z1,
    int nx, int ny, int nz,
    double min, double max) const
{
    if (surfs.size() <= 0)
    {
        return max;
    }

    // 雷达经纬度
    const double slon = toRadian(this->longitude);
    const double slat = toRadian(this->latitude);
    const double dx = (x1 - x0) / nx;
    const double dy = (y1 - y0) / ny;
    const double dz = (z1 - z0) / nz;
    // 用来保存方位角, 因为循环最外层是z轴, 而同一(x, y)方位角不变, 为了不重复计算方位角, 需要缓存
    vector<vector<double>> cacheAzimuth = vector<vector<double>>(ny, vector<double>(nx, 0));
    for (int iy = 0; iy < ny; iy++) {
        const double gy = y0 + iy * dy;
        auto& ty = cacheAzimuth[iy];
        for (int ix = 0; ix < nx; ix++) {
            const double gx = x0 + ix * dx;
            ty[ix] = toAngle(calcAzimuth(gx, gy));
        }
    }
    size_t index = 0;
    const int nxy = nx * ny;

    // 多线程运行结果
    vector<bool> find(nz, false);
    vector<double> result(nz, 0);

#pragma omp parallel for
    for (int iz = nz - 1; iz >= 0; iz--) {
        const double gz = z0 + iz * dz;
        for (int iy = 0; iy < ny; iy++) {
            if (find[iz]) break;
            const double gy = y0 + iy * dy;
            const double gy_square = gy * gy;
            const auto& azy = cacheAzimuth[iy];
            for (int ix = 0; ix < nx; ix++) {
                const double gx = x0 + ix * dx;
                const double gl = sqrt(gx * gx + gy_square);
                const double el0 = atan2(gz, gl);
                const double r_ = gl / cos(el0);
                const double el1 = asin(0.5 * r_ / RN);
                // 仰角
                const double el = toAngle(el0 + el1);
                // 传播路径弧长
                const double r = 2 * el1 * RN;
                // 方位角
                const double azimuth = azy[ix];
                const double v = interplateRadialData(el, azimuth, r, -1000);
                if (v >= min && v <= max) {
                    find[iz] = true;
                    result[iz] = gz;
                    break;
                }
            }
        }
    }
    for (int i = result.size() - 1; i >= 0; i--) {
        if (result[i] != 0) {
            return result[i];
        }
    }
    return z1;
}

void GRadarVolume::clear()
{
    siteCode.clear();
    siteName.clear();
    startTime.clear();
    surfNum = 0;

    longitude = 0;
    latitude = 0;

    for (auto* item : surfs) {
        delete item;
    }
    surfs.clear();
}

// 在数据量较多的情况下, 二分查找速度可能更快,二分查找仰角区间
int GRadarVolume::findElRange(double el) const
{
    int begin = 0, end = surfs.size() - 2;
    int index;
    while (begin <= end) {
        index = (begin + end) / 2;
        if (el >= surfs[index + 1]->el) {
            begin = index + 1;
        } else if (el < surfs[index]->el) {
            end = index - 1;
        } else {
            return index;
        }
    }
    return -1;
}

// 在数据量较少的情况下, 顺序查找速度可能更快,顺序查找仰角区间
int GRadarVolume::findElRange1(double el) const
{
    // 获取仰角数组的最大索引，即数组的长度减去1。
    const int n = surfs.size() - 1;
    // 检查给定的仰角是否在数组范围之外，如果是，则返回-1。
    if (el < surfs[0]->el || el >= surfs[n]->el) {
        return -1;
    }
    // 循环遍历仰角数组中的每个元素（除了第一个和最后一个）。
    for (int i = 1; i <= n - 1; i++) {
        // 检查给定的仰角是否小于当前数组元素的仰角。
        if (el < surfs[i]->el) {
            // 如果是，返回当前数组元素的前一个索引，即找到了给定仰角所在的区间。
            return i - 1;
        }
    }
     return n - 1;
    
    /*
    double Els[9] = { 0.65,1.34,2.26,3.22,4.15,5.84,9.84,13.84,19.33 };
    if (el < Els[0]|| el >= Els[8]) {
        return -1;
    }
    for (int i = 1; i <= 8 - 1; i++) {
        if (el < Els[i]) {
            return i - 1;
        }
    }
    return 8 - 1;
    */
   
}

// 插值同一仰角面上指定方位角和距离的数值
// 这个函数的主要目的是在雷达体数据的径向方向上进行插值，以获取指定方位角和距离处的值。
// 函数首先确定与指定方位角最接近的两条径向线，然后在这两条径向线上进行线性插值，以获取插值结果。函数使用二分查找来快速定位指定方位角所在的径向线。
double GRadarVolume::interplateElRadialData(int isurf, double az, double r, double invalidValue) const
{
    // 获取指定仰角的雷达数据(在一个锥形上操作)
    const GRadialSurf* pSurf = surfs[isurf];
    // 获取雷达数据的径向分辨率
    double dopRes = pSurf->interval;
    // 获取径向数据的点数
    size_t nd = pSurf->radials[0]->points.size();
    // 如果查询的距离小于径向分辨率或超过数据范围，返回无效值
    if (r < dopRes) return invalidValue;
    if (nd * dopRes < r) return invalidValue;

    // 获取扇面的所有径向线
    const vector<GRadialLine*>& radials = pSurf->radials;
    // 获取径向线的数量
    int n = pSurf->radials.size();
    // 初始化二分查找的开始和结束索引
    int begin = 0;
    int end = n - 2;
    size_t index;

    // 使用二分查找找到与指定方位角最接近的两条径向线
    while (begin <= end) {
        index = (begin + end) / 2;
        //cout << index << "," << begin << "," << end << endl;
        if (radials[index + 1]->az < az) {
            begin = index + 1;
        } else if (radials[index]->az > az) {
            end = index - 1;
        } else break;
    }

    // 如果折半查找失败, 说明az位于终止扫描线到起始扫描线之间，取最后一条径向线
    if (begin > end) index = pSurf->radials.size() - 1;

    // 计算与指定方位角最接近的两条径向线的方位角差
    double daz0 = az - radials[index]->az;
    if (daz0 < 0) daz0 += 360;
    double daz1 = radials[(index + 1) % n]->az - az;
    if (daz1 < 0) daz1 += 360;

    /*
    GRadialData *pData;
    if (daz0 < daz1) pData = radials[index];
    else pData = radials[(index + 1) % n];

    int i0, i1;
    double v0, v1, s;
    i0 = (int)(d / dopRes);
    i1 = i0 + 1;
    if (i1 >= nd) i1 = i0;
    v0 = pData->data[i0];
    v1 = pData->data[i1];
    if (v0 == invalidValue || v1 == invalidValue) return invalidValue;
    s = (d - i0*dopRes) / dopRes;
    return v0 + (v1 - v0)*s;
    */
    // 获取两条径向线上指定距离处的值，并进行线性插值
    GRadialLine* pLine0, * pLine1;
    pLine0 = radials[index];
    pLine1 = radials[(index + 1) % n];

    size_t i0 = (size_t)(r / dopRes);
    size_t i1 = i0 + 1;
    if (i1 >= nd) i1 = i0;

    double v0, v1, vt, vb, s;
    // 计算在径向线0上的值
    v0 = pLine0->points[i0].value;
    v1 = pLine0->points[i1].value;
    if (v0 == invalidValue || v1 == invalidValue) return invalidValue;
    s = (r - i0 * dopRes) / dopRes;
    vt = v0 + (v1 - v0) * s;
    // 计算在径向线1上的值
    v0 = pLine1->points[i0].value;
    v1 = pLine1->points[i1].value;
    if (v0 == invalidValue || v1 == invalidValue) return invalidValue;
    s = (r - i0 * dopRes) / dopRes;
    vb = v0 + (v1 - v0) * s;
    // 根据方位角差进行插值
    s = daz0 / (daz0 + daz1);
    return vt + (vb - vt) * s;
}

// 径向数据插值,因为地图上的点很可能不落在雷达的径向上,所以要插值,el是通过地图计算出来的理论点,az为方位角角度,r为传播路径弧长,invalid为无效值
double GRadarVolume::interplateRadialData(double el, double az, double r, double invalid) const
{
    //  函数找到仰角 el 所在的范围
    int iel = findElRange1(el);
    // 如果找到的仰角范围无效，直接返回无效值。
    if (iel == -1) return invalid;
    double v0 = interplateElRadialData(iel, az, r, invalid);
    double v1 = interplateElRadialData(iel + 1, az, r, invalid);
    if (v0 == invalid || v1 == invalid) return invalid;

    double el0 = surfs[iel]->el;
    double el1 = surfs[iel + 1]->el;
    return v0 + (v1 - v0) * (el - el0) / (el1 - el0);
}