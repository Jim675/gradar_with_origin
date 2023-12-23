#include "gradarvolume.h"

#include <algorithm>
#include <memory>

#include <qdatetime.h>
#include <qlocale.h>
#include <qdebug.h>

#include <gmapconstant.h>

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


// 计算雷达数据体边界范围
void GRadarVolume::calcBoundBox(double& minX, double& maxX,
    double& minY, double& maxY,
    double& minZ, double& maxZ) const
{
    if (surfs.size() == 0) {
        minX = maxX = minY = maxY = minZ = maxZ = 0;
        return;
    }

    // 最低仰角
    const GRadialSurf* pSurf = surfs[0];
    double el = toRadian(pSurf->el);
    // 弧长
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

// 考虑大气折射后的网格化
void GRadarVolume::gridding(double x0, double x1,
    double y0, double y1,
    double z0, double z1,
    int nx, int ny, int nz,
    double invalid, float* output) const
{
    const int nxy = nx * ny;

    if (surfs.size() <= 0)
    {
        for (int iz = 0; iz < nz; iz++)
        {
            for (int iy = 0; iy < ny; iy++)
            {
                for (int ix = 0; ix < nx; ix++)
                {
                    output[iz * nxy + iy * ny + ix] = invalid;
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
    //bool isPrint = false;
    
#pragma omp parallel for
    for (int iz = 0; iz < nz; iz++) {
        const double gz = z0 + iz * dz;
        //double gz_square = gz * gz;
        //// 高度为z网格点所在平面到地心距离(考虑到大气折射, 地球半径取等效地球半径RE_而不是真实地球半径RE)
        //r = RM + this->elev + z;
        //r_square = r * r;
        //// 高度为z的中心网格点到赤道平面距离
        //// centerToPlane = r * sin(clat);
        //// 雷达高度
        //h = this->elev;
        for (int iy = 0; iy < ny; iy++) {
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
                const double v = interplateRadialData(el, azimuth, r, invalid);
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

// 在数据量较多的情况下, 二分查找速度可能更快
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

// 在数据量较少的情况下, 顺序查找速度可能更快
int GRadarVolume::findElRange1(double el) const
{
    const int n = surfs.size() - 1;
    if (el < surfs[0]->el || el >= surfs[n]->el) {
        return -1;
    }
    for (int i = 1; i <= n - 1; i++) {
        if (el < surfs[i]->el) {
            return i - 1;
        }
    }
    return n - 1;
}

double GRadarVolume::interplateElRadialData(int isurf, double az, double r, double invalidValue) const
{
    const GRadialSurf* pSurf = surfs[isurf];
    double dopRes = pSurf->interval;
    size_t nd = pSurf->radials[0]->points.size();
    if (r < dopRes) return invalidValue;
    if (nd * dopRes < r) return invalidValue;

    const vector<GRadialLine*>& radials = pSurf->radials;
    int n = pSurf->radials.size();
    int begin = 0;
    int end = n - 2;
    size_t index;

    while (begin <= end) {
        index = (begin + end) / 2;
        //cout << index << "," << begin << "," << end << endl;
        if (radials[index + 1]->az < az) {
            begin = index + 1;
        } else if (radials[index]->az > az) {
            end = index - 1;
        } else break;
    }

    // 如果折半查找失败, 说明az位于终止扫描线到起始扫描线之间
    if (begin > end) index = pSurf->radials.size() - 1;

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

    GRadialLine* pLine0, * pLine1;
    pLine0 = radials[index];
    pLine1 = radials[(index + 1) % n];

    size_t i0 = (size_t)(r / dopRes);
    size_t i1 = i0 + 1;
    if (i1 >= nd) i1 = i0;

    double v0, v1, vt, vb, s;

    v0 = pLine0->points[i0].value;
    v1 = pLine0->points[i1].value;
    if (v0 == invalidValue || v1 == invalidValue) return invalidValue;
    s = (r - i0 * dopRes) / dopRes;
    vt = v0 + (v1 - v0) * s;

    v0 = pLine1->points[i0].value;
    v1 = pLine1->points[i1].value;
    if (v0 == invalidValue || v1 == invalidValue) return invalidValue;
    s = (r - i0 * dopRes) / dopRes;
    vb = v0 + (v1 - v0) * s;

    s = daz0 / (daz0 + daz1);
    return vt + (vb - vt) * s;
}

double GRadarVolume::interplateRadialData(double el, double az, double r, double invalid) const
{
    int iel = findElRange1(el);
    if (iel == -1) return invalid;
    double v0 = interplateElRadialData(iel, az, r, invalid);
    double v1 = interplateElRadialData(iel + 1, az, r, invalid);
    if (v0 == invalid || v1 == invalid) return invalid;

    double el0 = surfs[iel]->el;
    double el1 = surfs[iel + 1]->el;
    return v0 + (v1 - v0) * (el - el0) / (el1 - el0);
}