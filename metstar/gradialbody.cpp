//#include "gradialbody.h"
//#include <Constant.h>
//
//#include <math.h>
//
////bool radiaDataLessThan(GRadialData* pData1, GRadialData* pData2)
////{
////    return pData1->az < pData2->az;
////}
//
//
//void GRadialSurf::sort()
//{
//    // �Է�λ�Ǵ�С��������
//    std::sort(radials.begin(), radials.end(), [](const GRadialLine* a, const GRadialLine* b) {
//        return a->az < b->az;
//    });
//}
//
//GRadarVolume::GRadarVolume()
//{
//    longitude = latitude = 0;
//    elev = 0;
//    maxPointCount = 0;
//    minEl = 0;
//    maxEl = 0;
//}
//
//void GRadarVolume::clear()
//{
//    for (size_t i = 0; i < surfList.size(); i++) {
//        delete surfList[i];
//    }
//    surfList.clear();
//}
//
//void GRadarVolume::fillRadialData(GRadialLine* pData, cv_geneMom& mom, double invalidValue)
//{
//    ubytes& points = mom.points;
//    for (size_t i = 0; i < points.size(); i++) {
//        if (points[i] < 5) {
//            pData->points.push_back(invalidValue);
//        } else {
//            double v = ((double)points[i] - mom.udt.offset) / mom.udt.scale;
//            pData->points.push_back(v);
//        }
//    }
//}
//
//void GRadarVolume::extractData(basedataImage& bdi, int dataType, double invalidValue)
//{
//    clear();
//
//    longitude = bdi.siteInfo.lon;
//    latitude = bdi.siteInfo.lat;
//    //latitude = 0;
//    elev = bdi.siteInfo.height;
//
//    // ������ǵ��������ɨ��ֱ�ӷ���
//    if (bdi.taskConf.scanType != SP_PPI && bdi.taskConf.scanType != SP_VOL) {
//        return;
//    }
//
//    GRadialSurf* pSurf = nullptr;
//    int icut = 0;
//    minEl = 360;
//    maxEl = -360;
//
//    for (cvRadial::iterator it = bdi.radials.begin(); it != bdi.radials.end(); ++it) {
//        double el = it->el;
//        if (el > 90) el -= 360;
//
//        if (it->state == CUT_START || it->state == VOL_START) { // ���ǿ�ʼ����ɨ��ʼ
//            pSurf = nullptr;
//            for (cvGeneMom::iterator jt = it->mom.begin(); jt != it->mom.end(); ++jt) {
//                if (jt->udt.type == dataType) {
//                    double cel = bdi.cuts[icut].el;
//                    if (cel > 90) cel -= 360;
//
//                    pSurf = new GRadialSurf(bdi.cuts[icut].dopReso, cel);
//
//                    GRadialLine* pData = new GRadialLine(it->az, el);
//                    fillRadialData(pData, *jt, invalidValue);
//
//                    pSurf->radials.push_back(pData);
//                    surfList.push_back(pSurf);
//
//                    if (minEl > el) minEl = el;
//                    if (maxEl < el) maxEl = el;
//                    break;
//                }
//            }
//        } else if (it->state == CUT_END || it->state == VOL_END) { // ���ǽ���������ɨ����
//            icut++;
//        } else if (it->state == CUT_MID) { // �����м�����
//            if (pSurf) {
//                for (cvGeneMom::iterator jt = it->mom.begin(); jt != it->mom.end(); ++jt) {
//                    if (jt->udt.type == dataType) {
//                        GRadialLine* pData = new GRadialLine(it->az, el);
//                        pSurf->radials.push_back(pData);
//                        fillRadialData(pData, *jt, invalidValue);
//                        break;
//                    }
//                }
//            }
//        }
//    }
//
//    for (size_t i = 0; i < surfList.size(); i++) {
//        surfList[i]->sort();
//    }
//}
//
//// �����״�������߽緶Χ
//void GRadarVolume::calcBoundBox(double& minX, double& maxX,
//                               double& minY, double& maxY,
//                               double& minZ, double& maxZ) const
//{
//    if (surfList.size() == 0) {
//        minX = maxX = minY = maxY = minZ = maxZ = 0;
//        return;
//    }
//
//    // �������
//    const GRadialSurf* pSurf = surfList[0];
//    double el = toRadian(pSurf->el);
//    // ����
//    const double arcLen = pSurf->radials[0]->points.size() * pSurf->interval;
//    // TODO δ���Ǵ�������
//    const double r = cos(el) * arcLen;
//    minX = -r; maxX = r;
//    minY = -r; maxY = r;
//
//    // el1Ϊ���ǵ��ϰ벿��, �°벿��Ϊel0, el = el0+el1
//    const double el1 = arcLen * 0.5 / RN;
//    // r_Ϊ�״ﴫ��Բ��·��r���ҳ�
//    const double r_ = 2 * RN * sin(el1);
//    const double el0 = el - el1;
//    minZ = r_ * sin(el0);
//
//    // �������
//    pSurf = surfList[surfList.size() - 1];
//    el = toRadian(pSurf->el);
//    maxZ = arcLen * sin(el);
//}
//
//// ���ݷ�λ��az�������޵���az��[-PI/2 - PI/2]��[0 - 2*PI]
//double adjustAZ(double az, double dlon, double dlat)
//{
//    if (dlon >= 0) {
//        if (dlat >= 0) return az;
//        else return PI - az;
//    } else {
//        if (dlat >= 0) return az + PI * 2;
//        else return PI - az;
//    }
//}
//
//// ���㲢�����������޵�����λ��[0, 2*PI]
//inline double calcAzimuth(double x, double y)
//{
//    // atan2 �����ΧΪ(-PI/2, PI/2]
//    double azimuth = atan2(x, y);
//    if (azimuth < 0) return azimuth + PI * 2;
//    return azimuth;
//}
//
//
//// ���Ǵ�������������
//void GRadarVolume::gridding(double x0, double x1,
//                           double y0, double y1,
//                           double z0, double z1,
//                           int nx, int ny, int nz,
//                           double invalidValue, vtkDoubleArray* array) const
//{
//    // ������������
//    array->SetNumberOfComponents(1);
//    array->SetNumberOfTuples(nx * ny * nz);
//    double* pointer = array->GetPointer(0);
//    // �״ﾭγ��
//    const double slon = toRadian(this->longitude);
//    const double slat = toRadian(this->latitude);
//    //double r, r_square, gz_square, gy_square;
//    //double h, H, azimuth, el, d, d_square;
//    const double dx = (x1 - x0) / nx;
//    const double dy = (y1 - y0) / ny;
//    const double dz = (z1 - z0) / nz;
//    // �������淽λ��, ��Ϊѭ���������z��, ��ͬһx y, ��λ�ǲ���, Ϊ�˲��ظ����㷽λ��, ���뱣��
//    double** azimuthArray = new double* [ny];
//    for (int iy = 0; iy < ny; iy++) {
//        const double gy = y0 + iy * dy;
//        double* ty = azimuthArray[iy] = new double[nx];
//        for (int ix = 0; ix < nx; ix++) {
//            const double gx = x0 + ix * dx;
//            ty[ix] = toAngle(calcAzimuth(gx, gy));
//        }
//    }
//    size_t index = 0;
//    //bool isPrint = false;
//    const int nxy = nx * ny;
//    #pragma omp parallel for
//    for (int iz = 0; iz < nz; iz++) {
//        const double gz = z0 + iz * dz;
//        //double gz_square = gz * gz;
//        //// �߶�Ϊz���������ƽ�浽���ľ���(���ǵ���������, ����뾶ȡ��Ч����뾶RE_��������ʵ����뾶RE)
//        //r = RM + this->elev + z;
//        //r_square = r * r;
//        //// �߶�Ϊz����������㵽���ƽ�����
//        //// centerToPlane = r * sin(clat);
//        //// �״�߶�
//        //h = this->elev;
//        for (int iy = 0; iy < ny; iy++) {
//            const double gy = y0 + iy * dy;
//            const double gy_square = gy * gy;
//            const double* azy = azimuthArray[iy];
//            for (int ix = 0; ix < nx; ix++) {
//                const double gx = x0 + ix * dx;
//
//                const double gl = sqrt(gx * gx + gy_square);
//                const double el0 = atan2(gz, gl);
//                const double r_ = gl / cos(el0);
//                const double el1 = asin(0.5 * r_ / RN);
//                // ����
//                const double el = toAngle(el0 + el1);
//                // ����·������
//                const double r = 2 * el1 * RN;
//                // ��λ��
//                const double azimuth = azy[ix];
//                const double v = interplateRadialData(el, azimuth, r, invalidValue);
//                pointer[iz * nxy + iy * ny + ix] = v;
//            }
//        }
//    }
//    for (int iy = 0; iy < ny; iy++) {
//        delete[] azimuthArray[iy];
//    }
//    delete[] azimuthArray;
//}
//
//// ���������϶�������, ���ֲ����ٶȿ��ܸ���
//int GRadarVolume::findElRange(double el) const
//{
//    int begin = 0, end = surfList.size() - 2;
//    int index;
//    while (begin <= end) {
//        index = (begin + end) / 2;
//        if (el >= surfList[index + 1]->el) {
//            begin = index + 1;
//        } else if (el < surfList[index]->el) {
//            end = index - 1;
//        } else {
//            return index;
//        }
//    }
//    return -1;
//}
//
//// �����������ٵ������, ˳������ٶȿ��ܸ���
//int GRadarVolume::findElRange1(double el) const
//{
//    const int n = surfList.size() - 1;
//    if (el < surfList[0]->el || el >= surfList[n]->el) {
//        return -1;
//    }
//    for (int i = 1; i <= n - 1; i++) {
//        if (el < surfList[i]->el) {
//            return i - 1;
//        }
//    }
//    return n - 1;
//}
//
//double GRadarVolume::interplateElRadialData(int isurf, double az, double r, double invalidValue) const
//{
//    GRadialSurf* pSurf = surfList[isurf];
//    double dopRes = pSurf->interval;
//    size_t nd = pSurf->radials[0]->points.size();
//    if (r < dopRes) return invalidValue;
//    if (nd * dopRes < r) return invalidValue;
//
//    vector<GRadialLine*>& radials = pSurf->radials;
//    int n = pSurf->radials.size();
//    int begin = 0;
//    int end = n - 2;
//    size_t index;
//
//    while (begin <= end) {
//        index = (begin + end) / 2;
//        //cout << index << "," << begin << "," << end << endl;
//        if (radials[index + 1]->az < az) {
//            begin = index + 1;
//        } else if (radials[index]->az > az) {
//            end = index - 1;
//        } else break;
//    }
//
//    // ����۰����ʧ��, ˵��azλ����ֹɨ���ߵ���ʼɨ����֮��
//    if (begin > end) index = pSurf->radials.size() - 1;
//
//    double daz0 = az - radials[index]->az;
//    if (daz0 < 0) daz0 += 360;
//    double daz1 = radials[(index + 1) % n]->az - az;
//    if (daz1 < 0) daz1 += 360;
//
//    /*
//    GRadialData *pData;
//    if (daz0 < daz1) pData = radials[index];
//    else pData = radials[(index + 1) % n];
//
//    int i0, i1;
//    double v0, v1, s;
//    i0 = (int)(d / dopRes);
//    i1 = i0 + 1;
//    if (i1 >= nd) i1 = i0;
//    v0 = pData->data[i0];
//    v1 = pData->data[i1];
//    if (v0 == invalidValue || v1 == invalidValue) return invalidValue;
//    s = (d - i0*dopRes) / dopRes;
//    return v0 + (v1 - v0)*s;
//    */
//
//    GRadialLine* pLine0, * pLine1;
//    pLine0 = radials[index];
//    pLine1 = radials[(index + 1) % n];
//
//    size_t i0 = (size_t)(r / dopRes);
//    size_t i1 = i0 + 1;
//    if (i1 >= nd) i1 = i0;
//
//    double v0, v1, vt, vb, s;
//
//    v0 = pLine0->points[i0];
//    v1 = pLine0->points[i1];
//    if (v0 == invalidValue || v1 == invalidValue) return invalidValue;
//    s = (r - i0 * dopRes) / dopRes;
//    vt = v0 + (v1 - v0) * s;
//
//    v0 = pLine1->points[i0];
//    v1 = pLine1->points[i1];
//    if (v0 == invalidValue || v1 == invalidValue) return invalidValue;
//    s = (r - i0 * dopRes) / dopRes;
//    vb = v0 + (v1 - v0) * s;
//
//    s = daz0 / (daz0 + daz1);
//    return vt + (vb - vt) * s;
//}
//
//double GRadarVolume::interplateRadialData(double el, double az, double r, double invalidValue) const
//{
//    int iel = findElRange1(el);
//    if (iel == -1) return invalidValue;
//    double v0 = interplateElRadialData(iel, az, r, invalidValue);
//    double v1 = interplateElRadialData(iel + 1, az, r, invalidValue);
//    if (v0 == invalidValue || v1 == invalidValue) return invalidValue;
//
//    double el0 = surfList[iel]->el;
//    double el1 = surfList[iel + 1]->el;
//    return v0 + (v1 - v0) * (el - el0) / (el1 - el0);
//}