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
    // ����new�����Ķ���
    for (auto* points : this->radials) {
        delete points;
    }
    this->radials.clear();
}

void GRadialSurf::sort()
{
    // �Է�λ�Ǵ�С��������
    std::sort(radials.begin(), radials.end(), [](const GRadialLine* a, const GRadialLine* b) {
        return a->az < b->az;
        });
}

void GRadialSurf::clear()
{
    this->interval = 0;
    this->el = 0;
    this->elRadian = 0;
    // ����new�����Ķ���
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
    // ���֮ǰ������
    surfs.clear();

    siteCode = QString(bdi.siteInfo.code);
    siteName = QString(bdi.siteInfo.name);
    startTime = QDateTime::fromSecsSinceEpoch(bdi.taskConf.startTime).toString("yyyy-MM-dd hh:mm:ss");
    //surfNum = bdi.taskConf.cutNum;

    longitude = bdi.siteInfo.lon;
    latitude = bdi.siteInfo.lat;
    elev = bdi.siteInfo.height;

    // ������ǵ��������ɨ��ֱ�ӷ���
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

        if (it->state == CUT_START || it->state == VOL_START) { // ���ǿ�ʼ����ɨ��ʼ
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
        } else if (it->state == CUT_END || it->state == VOL_END) { // ���ǽ���������ɨ����
            icut++;
        } else if (it->state == CUT_MID) { // �����м�����
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
    //    qDebug() << "Error: ׶��������һ��: " << surfNum << ", " << surfs.size();
    surfNum = surfs.size();
    //}
}

void GRadarVolume::modifyData(basedataImage& bdi, int dataType) {

    //qDebug() << "=========================��ʼ�޸�==========================";

    //�ж�double���������ʱ�õ�
    const double epsilon = 1e-9;

    for (int i = 0; i < SITE_CODE_LENGTH; i++) {
        //��ȡվ����
        bdi.siteInfo.code[i] = siteCode.toLatin1().data()[i];
        //��ȡվ������
        bdi.siteInfo.name[i] = siteName.toLatin1().data()[i];
    }

    //��ȡɨ�迪ʼʱ���
    bdi.taskConf.startTime = QDateTime::fromString(startTime, QString("yyyy-MM-dd hh:mm:ss")).toTime_t();
    //�޸�ʱ���
    //bdi.taskConf.startTime += 10000;
    //��ȡվ�㾭��
    bdi.siteInfo.lon = longitude;
    //��ȡվ��ά��
    bdi.siteInfo.lat = latitude;
    //��ȡվ�㺣�θ߶�
    bdi.siteInfo.height = elev;

    GRadialSurf* pSurf = nullptr;
    int icut = 0;
    int isurf = 0;
    int idata = 0;

    //����bdi��ÿһ���״�����
    for (auto it = bdi.radials.begin(); it != bdi.radials.end(); ++it) {

        //qDebug() << "=================================================";
        //qDebug() << "====================��ʼд��=====================";
        //qDebug() << "cuts_el_" << i << "=" << bdi.cuts.at(i).el;
        //qDebug() << "surfs_el_" << i << "=" << surfs.at(i)->el;
        //qDebug() << "cuts_az_" << i << "=" << bdi.cuts.at(i).az;
        //qDebug() << "surfs_az_" << i << "=" << surfs.at(i)->radials.at(0)->az;
        //qDebug() << "cuts_dopReso_" << i << "=" << bdi.cuts.at(i).dopReso;
        //qDebug() << "surfs_interval_" << i << "=" << surfs.at(i)->interval;

        if (it->state == CUT_START || it->state == VOL_START) {  // ���ǿ�ʼ����ɨ��ʼ
            pSurf = nullptr;
            //����radials��ÿһ�ݾ�������
            for (auto jt = it->mom.begin(); jt != it->mom.end(); ++jt) {
                if (jt->udt.type == dataType) {
                    //Ԥ��������volume�е�׶��
                    pSurf = surfs.at(isurf++);
                    idata = 0;
                    if (pSurf->el > 90)
                        it->el = pSurf->el + 360;
                    else
                        it->el = pSurf->el;
                    //����׶�����ݸ���bdi�е�����
                    bdi.cuts[icut].dopReso = pSurf->interval;  //�����շֱ���
                    bdi.cuts[icut].az = pSurf->radials.at(idata)->az;  //��λ��

                    //���������ϵĵ��б�
                    ubytes& points = (*jt).points;
                    //��ÿһ����������޸�ΪԤ��volume�еĵ�����
                    for (size_t i = 0; i < points.size(); i++) {
                        //��Ԥ��volume�еĵ�ֵΪ-1000ʱ, ��ʾ����ȡ������points[i] < 5, ����ֱ�����㲻Ӱ���´ζ�����
                        if (std::abs(pSurf->radials.at(idata)->points.at(i).value + 1000) < epsilon) {
                            points[i] = 0;
                        }
                        else {
                            if (pSurf->radials.at(idata)->points.at(i).value < 0) {
                                //��Ԥ��volume�еĵ�����ֵΪ��, �Ҳ�Ϊ-1000, ��ȡ��, �������Ա�֤�޸�ǰ���������ֵ����ֵ���С
                                //points[i] = (unsigned short)((*jt).udt.offset - pSurf->radials.at(idata)->points.at(i).value * (*jt).udt.scale);
                                points[i] = 0;
                            }
                            else
                                //���������ֱ�ӹ�ʽ����, ���Ǵ�double���´��unsigned short�����������ʧ
                                points[i] = (unsigned short)((*jt).udt.offset + pSurf->radials.at(idata)->points.at(i).value * (*jt).udt.scale);
                        }
                    }
                    //�״����ݽṹ���еķ�λ��
                    it->az = pSurf->radials.at(idata)->az;
                    //�����޸�
                    if (pSurf->radials.at(idata)->el > 90)
                        bdi.cuts[icut].el = pSurf->radials.at(idata)->el + 360;
                    else
                        bdi.cuts[icut].el = pSurf->radials.at(idata)->el;
                    idata++;
                }
            }
        }
        else if (it->state == CUT_END || it->state == VOL_END) {  // ���ǽ���������ɨ����
            icut++;
        }
        else if (it->state == CUT_MID) {  // �����м�����
            if (pSurf) {
                //��λ��
                it->az = pSurf->radials.at(idata)->az;
                //����
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

// �����״�������߽緶Χ
void GRadarVolume::calcBoundBox(double& minX, double& maxX,
    double& minY, double& maxY,
    double& minZ, double& maxZ) const
{
    if (surfs.size() == 0) {
        minX = maxX = minY = maxY = minZ = maxZ = 0;
        return;
    }

    // el���������
    const GRadialSurf* pSurf = surfs[0];
    double el = toRadian(pSurf->el);
    // ����(�״ﾶ��ĵ������Ծ���ļ��)
    const double arcLen = pSurf->radials[0]->points.size() * pSurf->interval;
    // TODO δ���Ǵ�������
    const double r = cos(el) * arcLen;
    minX = -r; maxX = r;
    minY = -r; maxY = r;

    // el1Ϊ���ǵ��ϰ벿��, �°벿��Ϊel0, el = el0+el1
    const double el1 = arcLen * 0.5 / RN;
    // r_Ϊ�״ﴫ��Բ��·��r���ҳ�
    const double r_ = 2 * RN * sin(el1);
    const double el0 = el - el1;
    minZ = r_ * sin(el0);

    // �������
    pSurf = surfs[surfs.size() - 1];
    el = toRadian(pSurf->el);
    maxZ = arcLen * sin(el);
}

// ���㲢�����������޵�����λ��[0, 2*PI]
inline double calcAzimuth(double x, double y)
{
    // atan2 �����ΧΪ(-PI/2, PI/2]
    double azimuth = atan2(x, y);
    if (azimuth < 0) return azimuth + PI * 2;
    return azimuth;
}

GRadialSurf* GRadarVolume::generateNewSurf(vtkImageData* imageData)
{

}

// ���Ǵ�������������
void GRadarVolume::gridding(double x0, double x1,
    double y0, double y1,
    double z0, double z1,
    int nx, int ny, int nz,
    double invalid, float* output) const
{
    // ����ÿ����Σ�z�᷽�򣩵Ķ�άƽ���ϵ����ݵ�����
    const int nxy = nx * ny;
    // ����״�������û����Ч�ľ���surfs��������������е�����ֵ����Ϊ��Чֵ��invalid����
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

    // �״ﾭγ��
    const double slon = toRadian(this->longitude);
    const double slat = toRadian(this->latitude);
    //double r, r_square, gz_square, gy_square;
    //double h, H, azimuth, el, d, d_square;
    // ������ x��y �� z �����ϵļ�ࡣ
    const double dx = (x1 - x0) / nx;
    const double dy = (y1 - y0) / ny;
    const double dz = (z1 - z0) / nz;
    // �������淽λ��, ��Ϊѭ���������z��, ��ͬһ(x, y)��λ�ǲ���, Ϊ�˲��ظ����㷽λ��, ��Ҫ����
    // ����һ����ά���� cacheAzimuth�����ڻ���ÿ�� (x, y) ��ķ�λ�ǣ��Լ����ظ����㡣
    vector<vector<double>> cacheAzimuth = vector<vector<double>>(ny, vector<double>(nx, 0));
    // ���ѭ������ y �����ϵ�ÿ������㡣
    for (int iy = 0; iy < ny; iy++) {
        // ���㵱ǰ y �����ϵ�������λ�á�
        const double gy = y0 + iy * dy;
        // ��ȡ cacheAzimuth �е�ǰ�е����á�
        auto& ty = cacheAzimuth[iy];
        // �ڲ�ѭ������ x �����ϵ�ÿ������㡣
        for (int ix = 0; ix < nx; ix++) {
            // ���㵱ǰ x �����ϵ�������λ�á�
            const double gx = x0 + ix * dx;
            // ���㵱ǰ�����ķ�λ�ǣ����洢�� ty ��,calcAzimuth �������ڼ��㷽λ�ǣ��� toAngle �����򽫻���ת��Ϊ�Ƕȡ�
            ty[ix] = toAngle(calcAzimuth(gx, gy));
        }
    }
    size_t index = 0;
    //bool isPrint = false;
    
// ʹ�� OpenMP ָ����������ѭ�����Ա��ڶ���߳��ϲ���ִ��ѭ����
#pragma omp parallel for 
    // ѭ������ z �����ϵ�ÿ������㡣  
    for (int iz = 0; iz < nz; iz++) {
        // ���㵱ǰ z �����ϵ�������λ�á�
        const double gz = z0 + iz * dz;

        //double gz_square = gz * gz;
        //// �߶�Ϊz���������ƽ�浽���ľ���(���ǵ���������, ����뾶ȡ��Ч����뾶RE_��������ʵ����뾶RE)
        //r = RM + this->elev + z;
        //r_square = r * r;
        //// �߶�Ϊz����������㵽���ƽ�����
        //// centerToPlane = r * sin(clat);
        //// �״�߶�
        //h = this->elev;

        // �ڲ�ѭ������ y �����ϵ�ÿ������㡣
        for (int iy = 0; iy < ny; iy++) {
            // ���㵱ǰ y �����ϵ�������λ�á�
            const double gy = y0 + iy * dy;
            // ���� y �����ϵ�λ�õ�ƽ����
            const double gy_square = gy * gy;
            // ��ȡԤ�ȼ���ķ�λ�ǵ����á�(�������ѭ�����Ѿ��������)
            const auto& azy = cacheAzimuth[iy];
            // �ٴ��ڲ�ѭ������ x �����ϵ�ÿ������㡣
            for (int ix = 0; ix < nx; ix++) {
                // ���㵱ǰ x �����ϵ�������λ�á�
                const double gx = x0 + ix * dx;
                // �����״ﵽ������ˮƽ���롣
                const double gl = sqrt(gx * gx + gy_square);
                // �������ǵ��ϰ벿�֡�
                const double el0 = atan2(gz, gl);
                // �����״ﵽ���ĵľ��롣
                const double r_ = gl / cos(el0);
                // �������ǵ��°벿�֡�
                const double el1 = asin(0.5 * r_ / RN);
                // ����,�ϲ����������ֵõ��������ǡ�
                const double el = toAngle(el0 + el1);
                // ����·������,���㴫��·��������
                const double r = 2 * el1 * RN;
                // ��λ��,��ȡ��λ�ǡ�
                const double azimuth = azy[ix];
                // ��ֵ�״������Ի�ȡ������ֵ��!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                const double v = interplateRadialData(el, azimuth, r, invalid);
                // ����ֵ���ֵ�洢����������С�
                output[iz * nxy + iy * ny + ix] = v;
            }
        }
    }
}

// ̽�⿼�Ǵ��������������ЧZ����
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

    // �״ﾭγ��
    const double slon = toRadian(this->longitude);
    const double slat = toRadian(this->latitude);
    const double dx = (x1 - x0) / nx;
    const double dy = (y1 - y0) / ny;
    const double dz = (z1 - z0) / nz;
    // �������淽λ��, ��Ϊѭ���������z��, ��ͬһ(x, y)��λ�ǲ���, Ϊ�˲��ظ����㷽λ��, ��Ҫ����
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

    // ���߳����н��
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
                // ����
                const double el = toAngle(el0 + el1);
                // ����·������
                const double r = 2 * el1 * RN;
                // ��λ��
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

// ���������϶�������, ���ֲ����ٶȿ��ܸ���,���ֲ�����������
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

// �����������ٵ������, ˳������ٶȿ��ܸ���,˳�������������
int GRadarVolume::findElRange1(double el) const
{
    // ��ȡ������������������������ĳ��ȼ�ȥ1��
    const int n = surfs.size() - 1;
    // �������������Ƿ������鷶Χ֮�⣬����ǣ��򷵻�-1��
    if (el < surfs[0]->el || el >= surfs[n]->el) {
        return -1;
    }
    // ѭ���������������е�ÿ��Ԫ�أ����˵�һ�������һ������
    for (int i = 1; i <= n - 1; i++) {
        // �������������Ƿ�С�ڵ�ǰ����Ԫ�ص����ǡ�
        if (el < surfs[i]->el) {
            // ����ǣ����ص�ǰ����Ԫ�ص�ǰһ�����������ҵ��˸����������ڵ����䡣
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

// ��ֵͬһ��������ָ����λ�Ǻ;������ֵ
// �����������ҪĿ�������״������ݵľ������Ͻ��в�ֵ���Ի�ȡָ����λ�Ǻ;��봦��ֵ��
// ��������ȷ����ָ����λ����ӽ������������ߣ�Ȼ�����������������Ͻ������Բ�ֵ���Ի�ȡ��ֵ���������ʹ�ö��ֲ��������ٶ�λָ����λ�����ڵľ����ߡ�
double GRadarVolume::interplateElRadialData(int isurf, double az, double r, double invalidValue) const
{
    // ��ȡָ�����ǵ��״�����(��һ��׶���ϲ���)
    const GRadialSurf* pSurf = surfs[isurf];
    // ��ȡ�״����ݵľ���ֱ���
    double dopRes = pSurf->interval;
    // ��ȡ�������ݵĵ���
    size_t nd = pSurf->radials[0]->points.size();
    // �����ѯ�ľ���С�ھ���ֱ��ʻ򳬹����ݷ�Χ��������Чֵ
    if (r < dopRes) return invalidValue;
    if (nd * dopRes < r) return invalidValue;

    // ��ȡ��������о�����
    const vector<GRadialLine*>& radials = pSurf->radials;
    // ��ȡ�����ߵ�����
    int n = pSurf->radials.size();
    // ��ʼ�����ֲ��ҵĿ�ʼ�ͽ�������
    int begin = 0;
    int end = n - 2;
    size_t index;

    // ʹ�ö��ֲ����ҵ���ָ����λ����ӽ�������������
    while (begin <= end) {
        index = (begin + end) / 2;
        //cout << index << "," << begin << "," << end << endl;
        if (radials[index + 1]->az < az) {
            begin = index + 1;
        } else if (radials[index]->az > az) {
            end = index - 1;
        } else break;
    }

    // ����۰����ʧ��, ˵��azλ����ֹɨ���ߵ���ʼɨ����֮�䣬ȡ���һ��������
    if (begin > end) index = pSurf->radials.size() - 1;

    // ������ָ����λ����ӽ������������ߵķ�λ�ǲ�
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
    // ��ȡ������������ָ�����봦��ֵ�����������Բ�ֵ
    GRadialLine* pLine0, * pLine1;
    pLine0 = radials[index];
    pLine1 = radials[(index + 1) % n];

    size_t i0 = (size_t)(r / dopRes);
    size_t i1 = i0 + 1;
    if (i1 >= nd) i1 = i0;

    double v0, v1, vt, vb, s;
    // �����ھ�����0�ϵ�ֵ
    v0 = pLine0->points[i0].value;
    v1 = pLine0->points[i1].value;
    if (v0 == invalidValue || v1 == invalidValue) return invalidValue;
    s = (r - i0 * dopRes) / dopRes;
    vt = v0 + (v1 - v0) * s;
    // �����ھ�����1�ϵ�ֵ
    v0 = pLine1->points[i0].value;
    v1 = pLine1->points[i1].value;
    if (v0 == invalidValue || v1 == invalidValue) return invalidValue;
    s = (r - i0 * dopRes) / dopRes;
    vb = v0 + (v1 - v0) * s;
    // ���ݷ�λ�ǲ���в�ֵ
    s = daz0 / (daz0 + daz1);
    return vt + (vb - vt) * s;
}

// �������ݲ�ֵ,��Ϊ��ͼ�ϵĵ�ܿ��ܲ������״�ľ�����,����Ҫ��ֵ,el��ͨ����ͼ������������۵�,azΪ��λ�ǽǶ�,rΪ����·������,invalidΪ��Чֵ
double GRadarVolume::interplateRadialData(double el, double az, double r, double invalid) const
{
    //  �����ҵ����� el ���ڵķ�Χ
    int iel = findElRange1(el);
    // ����ҵ������Ƿ�Χ��Ч��ֱ�ӷ�����Чֵ��
    if (iel == -1) return invalid;
    double v0 = interplateElRadialData(iel, az, r, invalid);
    double v1 = interplateElRadialData(iel + 1, az, r, invalid);
    if (v0 == invalid || v1 == invalid) return invalid;

    double el0 = surfs[iel]->el;
    double el1 = surfs[iel + 1]->el;
    return v0 + (v1 - v0) * (el - el0) / (el1 - el0);
}