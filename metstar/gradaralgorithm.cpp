#include "gradaralgorithm.h"
#include "gradarvolume.h"
#include "gradialbody.h"

#include <gmapconstant.h>
#include <gmapcoordconvert.h>

#include <cmath>
#include <vtkColorTransferFunction.h>


/*
* ��������������״������(gx, gy, gz)תΪ��γ��(lon, lat)
*
* gx, gy, gx: ������������״������
* radarLon, radarLat, radarElev: ������������״ﾭγ��(�Ƕ�)�ͺ���
* lon, lat: ����õ��ľ�γ��
*/
void GRadarAlgorithm::gridToLonLat(
    const double gx, const double gy, const double gz,
    const double radarLon, const double radarLat, const double radarElev,
    double* lon, double* lat)
{
    const double sLon = toRadian(radarLon);
    const double sLat = toRadian(radarLat);
    const double sin_slat = sin(sLat);
    const double cos_slat = cos(sLat);

    // gz + ���ĵ��״����
    const double l = gz + radarElev + RE;
    // ���ĵ㵽���ƽ��߶�
    const double ch = l * sin_slat;
    // ���ĵ����ĵ��ڳ��ƽ����ͶӰ�����
    const double cl = l * cos_slat;

    const double dcl = gy * sin_slat;
    const double dch = gy * cos_slat;
    const double cl0 = cl - dcl;

    double tlon = atan2(gx, cl0) + sLon;
    double tlat = atan2(ch + dch, sqrt(gx * gx + cl0 * cl0));
    tlon = toAngle(tlon);
    tlat = toAngle(tlat);
    // TODO ����������ľ�γ��
    if (tlon > 180.0) {
        tlon -= 360.0;
    } else if (tlon <= -180.0) {
        tlon += 360.0;
    }
    *lon = tlon;
    *lat = tlat;
}

/*
* �Ѿ�γ��(lon, lat), gz תΪ������������״������(gx, gy)
*
* lon, lat: ��γ��
* gz: ������������״��z����
* radarLon, radarLat, radarElev: ������������״ﾭγ��(�Ƕ�)�ͺ���
* gx, gy: ������������״������
*/
void GRadarAlgorithm::lonLatToGrid(
    const double lon, const double lat, const double gz,
    const double radarLon, const double radarLat, const double radarElev,
    double* gx, double* gy)
{
    const double sLon = toRadian(radarLon);
    const double sLat = toRadian(radarLat);
    const double sin_slat = sin(sLat);
    const double cos_slat = cos(sLat);

    // gz + ���ĵ��״����
    const double l = gz + radarElev + RE;
    // ���ĵ㵽���ƽ��߶�
    const double ch = l * sin_slat;
    // ���ĵ����ĵ��ڳ��ƽ����ͶӰ�����
    const double cl = l * cos_slat;

    const double dLon = toRadian(lon) - sLon;
    double a = tan(dLon); // = gx / cl0 = gx / (cl-gy * sin_slat)
    // gx = a * cl0
    double b = tan(toRadian(lat)); // =(ch + dch) / sqrt(gx * gx + cl0 * cl0)
    // =(ch + dch) / sqrt((a^2+1) * cl0^2)
    // =(ch + dch) / (sqrt(a^2+1) * |cl0|)
    // ���� cl0>=0
    // =(ch + gy * cos_slat) / ((sqrt(a^2+1) * (cl - gy * sin_slat))
    // b * (sqrt(a^2+1) * (cl - gy * sin_slat) = ch + gy * cos_slat
    // b * (sqrt(a^2+1) * cl - gy * sin_slat * b * (sqrt(a^2+1) = ch + gy * cos_slat
    // gy * sin_slat * b * (sqrt(a^2+1) + gy * cos_slat = b * (sqrt(a^2+1) * cl - ch
    // gy * (sin_slat * b * (sqrt(a^2+1) + cos_slat) = b * (sqrt(a^2+1) * cl - ch
    const double t0 = b * sqrt(a * a + 1);
    // gy * (sin_slat * t0 + cos_slat) = t0 * cl - ch
    double tgy = (t0 * cl - ch) / (sin_slat * t0 + cos_slat);
    // ��� cl0<0
    // double tgy = (t0 * cl + ch) / (sin_slat * t0 - cos_slat);
    const double dcl = tgy * sin_slat;
    const double dch = tgy * cos_slat;
    const double cl0 = cl - dcl;
    double tgx = a * cl0;

    // TODO ����������ľ�γ��
    //if (tlon > 180.0) {
    //    tlon -= 360.0;
    //} else if (tlon <= -180.0) {
    //    tlon += 360.0;
    //}
    *gx = tgx;
    *gy = tgy;
}


/*
* ��׼����������, ���״��Ų�����·������תΪ�ҳ�
* ��׼����������, �ɽ����״��Ų�����·������һ�ΰ뾶ΪRNԲ��, RN = 4 * ����뾶
*
* arcLen: �״��Ų�����·������
* el: ��Ų���������, ��λ�ǻ���
* return: �ҳ�
*/
double GRadarAlgorithm::arcToChord(const double arcLen, const double el)
{
    // el1Ϊ���ǵ��ϰ벿��, �°벿��Ϊel0, el = el0+el1
    const double el1 = arcLen * 0.5 / RN;
    // r_Ϊ�״ﴫ��Բ��·��r���ҳ�
    const double chord = 2 * RN * sin(el1);
    // el1Ϊ���ǵ��ϰ벿��, �°벿��Ϊel0, el0 = el-��������Zƽ��н�, el = el0 + el1
    //const double el1 = arcLen * 0.5 / RN;
    //const double el0 = el - el0;
    return chord;
}


/*
* ��׶������ת��Ϊī����ͶӰ�������
*
* surface: Ҫת����׶��
* longitude: �״ﾭ�� �Ƕ�
* latitude: �״�ά�� �Ƕ�
* elev: �״ﺣ���߶�
*
* return ת���������
*/
void GRadarAlgorithm::surfToMercator(GRadialSurf* surface,
    const double longitude, const double latitude,
    const double elev)
{
    // ׶������
    const double elRadian = surface->elRadian;

    // �״�վ������
    const double sLon = toRadian(longitude);
    const double sLat = toRadian(latitude);
    const double sin_slat = sin(sLat);
    const double cos_slat = cos(sLat);

    // ���㾭γ�Ȳ�תī����ͶӰ
    surface->radials[0]->az = 0;
    for (GRadialLine* line : surface->radials) {
        //const double azRadian = toRadian(line->az);
        const double azRadian = line->azRadian;
        const size_t size = line->points.size();
        for (size_t i = 0; i < size; i++) {
            auto& item = line->points[i];
            //const double value = item->points[i];
            // rΪ�״ﴫ��·������
            const double r = (i + 1) * surface->interval;
            // el1Ϊ���ǵ��ϰ벿��, �°벿��Ϊel0, elRadian = el0 + el1
            const double el1 = r * 0.5 / RN;
            // el0Ϊ��������Zƽ��н�
            const double el0 = elRadian - el1;
            // r_Ϊ�״ﴫ��Բ��·��r���ҳ�
            const double r_ = 2 * RN * sin(el1);
            // �״ﴫ��Բ��·��r���ҳ�ͶӰ������XYƽ�泤��
            const double gl = r_ * cos(el0);
            // ������������״������ X Y Z
            const double gx = gl * sin(azRadian);
            const double gy = gl * cos(azRadian);
            const double gz = r_ * sin(el0);

            // gz + ���ĵ��״����
            const double l = gz + elev + RE;
            // ���ĵ㵽���ƽ��߶�
            const double ch = l * sin_slat;
            // ���ĵ����ĵ��ڳ��ƽ����ͶӰ�����
            const double cl = l * cos_slat;

            const double dcl = gy * sin_slat;
            const double dch = gy * cos_slat;
            const double cl0 = cl - dcl;

            double lon = atan2(gx, cl0) + sLon;
            double lat = atan2(ch + dch, sqrt(gx * gx + cl0 * cl0));
            lon = toAngle(lon);
            lat = toAngle(lat);

            // TODO ����������ľ�γ��
            if (lon > 180.0) {
                lon -= 360.0;
            } else if (lon <= -180.0) {
                lon += 360.0;
            }
            // ��γ��תī����ͶӰ
            GMapCoordConvert::lonLatToMercator(lon, lat, &item.x, &item.y);
        }
    }
    surface->bound = GRadarAlgorithm::calcMercatorBound(surface, longitude, latitude, elev);
    surface->isConvert = true;
}


/*
* ����׶������ת��ī����ͶӰ������귶Χ
*
* surface: Ҫ�����׶��
* longitude: �״ﾭ�� �Ƕ�
* latitude: �״�ά�� �Ƕ�
* elev: �״ﺣ�θ߶�

* return ת��������귶Χ
*/
QRectF GRadarAlgorithm::calcMercatorBound(const GRadialSurf* surface,
    const double longitude, const double latitude, const double elev)
{
    QRectF bound;
    if (surface->radials.size() == 0) {
        return bound;
    }

    // ׶������
    // �״�վ������
    const double sLon = toRadian(longitude);
    const double sLat = toRadian(latitude);
    //const double sin_slat = sin(sLat);
    //const double cos_slat = cos(sLat);

    double mx0 = 0, my0 = 0;
    GMapCoordConvert::lonLatToMercator(longitude, latitude, &mx0, &my0);

    // rΪ�״ﴫ��·������
    const double r = surface->radials[0]->points.size() * surface->interval;
    // el1Ϊ���ǵ��ϰ벿��, �°벿��Ϊel0, elRadian = el0 + el1
    const double el1 = r * 0.5 / RN;
    // el0Ϊ��������Zƽ��н�
    const double el0 = surface->elRadian - el1;
    // r_Ϊ�״ﴫ��Բ��·��r���ҳ�
    const double r_ = 2 * RN * sin(el1);
    // �״ﴫ��Բ��·��r���ҳ�ͶӰ������XYƽ�泤��
    const double gl = r_ * cos(el0);
    // ���ĵ��״��������ľ���
    const double z = RE + elev;

    // ��X�᷽�򳤶� ��λ��Ϊ90�� (PI/4)����

    // ������������״������ X Y Z
    const double gx = gl;
    const double gy = gl;
    const double gz = r_ * sin(el0);

    // ���㷽λ��Ϊ0�Ⱥ�90��ʱ ���Ǵ�������ľ�γ��
    double lon = sLon + atan2(gx, (z + gz) * cos(sLat));
    double lat = sLat + atan2(gy, z + gz);

    double mx1 = 0, my1 = 0;
    GMapCoordConvert::lonLatToMercator(toAngle(lon), toAngle(lat), &mx1, &my1);
    // half of w/h
    double hw = mx1 - mx0;
    double hh = my1 - my0;
    bound.setRect(mx0 - hw, my0 - hh, 2 * hw, 2 * hh);
    return bound;
}


// �ھ����Ͻ���ƽ�� ƽ���˴�СΪ5
void GRadarAlgorithm::smoothRadial(GRadarVolume* body)
{
    vector<double> tempPoints;
    for (auto surf : body->surfs) {
        for (auto line : surf->radials) {
            const size_t size = line->points.size();
            tempPoints.reserve(size);

            // ƽ��
            tempPoints.push_back(line->points[0].value * 0.7 + line->points[1].value * 0.3);
            tempPoints.push_back(line->points[0].value * 0.2 + line->points[1].value * 0.6 + line->points[2].value * 0.2);

            for (int i = 2; i < size - 2; i++) {
                //double mean = (line->points[i - 2] + line->points[i - 1] + line->points[i] + line->points[i + 1] + line->points[i + 2]) / 5;
                double mean = line->points[i - 2].value * 0.2 + line->points[i - 1].value * 0.2 + line->points[i].value * 0.2 + line->points[i + 1].value * 0.2 + line->points[i + 2].value * 0.2;
                tempPoints.push_back(mean);
            }

            //tempPoints.push_back((line->points[size - 3] + line->points[size - 2] + line->points[size - 1]) / 3);
            tempPoints.push_back(line->points[size - 3].value * 0.2 + line->points[size - 2].value * 0.6 + line->points[size - 1].value * 0.2);
            //tempPoints.push_back((line->points[size - 2] + line->points[size - 1]) / 2);
            tempPoints.push_back(line->points[size - 2].value * 0.3 + line->points[size - 1].value * 0.7);


            //line->points.swap(tempPoints);
            for (size_t i = 0; i < line->points.size(); ++i) {
                line->points[i].value = tempPoints[i];
            }
            tempPoints.clear();
        }
    }
}


double GRadarAlgorithm::interplateElRadialData(const GRadialSurf* pSurf, const double az, const double r, const double invalid)
{
    const double interval = pSurf->interval;
    const vector<GRadialLine*>& radials = pSurf->radials;
    // �������ݸ���
    const size_t n = radials.size();

    // ÿ���������ݲ�������
    const size_t nd = radials[0]->points.size();
    // ���������ж�, ����Ͳ����ж���
    //if (r < dopRes) {
    //    return invalid;
    //}
    //if (nd * dopRes < r) {
    //    return invalid;
    //}
    size_t index0 = 0;
    size_t index1 = 0;
    if (az < radials[0]->az || az >= radials[n - 1]->az) {// �ų����ּ������
        index0 = n - 1;
        index1 = 0;
    } else {
        // begin end ����Ϊ������
        int begin = 0;
        int end = n - 2;
        while (begin <= end) {
            index0 = (begin + end) / 2;
            if (az >= radials[index0 + 1]->az) {
                begin = index0 + 1;
            } else if (az < radials[index0]->az) {
                end = index0 - 1;
            } else break;
        }
        index1 = index0 + 1;
    }
    // ����������˳ʱ������ index, nextIndex
    const GRadialLine* pData0 = radials[index0];
    const GRadialLine* pData1 = radials[index1];

    // ��n�������Ϊ n * dopRes, 
    size_t i0 = static_cast<size_t>(r / interval) - 1;
    size_t i1 = i0 + 1;
    if (i1 >= nd) i1 = i0;

    // �������������������ھ����ϲ�ֵ
    double vNear = pData0->points[i0].value;
    double vFar = pData0->points[i1].value;

    if (vNear == invalid || vFar == invalid) return invalid;
    double rate = (r - (i0 + 1) * interval) / interval;
    double v0 = vNear + (vFar - vNear) * rate;

    vNear = pData1->points[i0].value;
    vFar = pData1->points[i1].value;
    if (vNear == invalid || vFar == invalid) return invalid;
    double v1 = vNear + (vFar - vNear) * rate;

    // �ڽǶ��ϲ�ֵ
    double dLeft = az - pData0->az;
    if (dLeft < 0.0) dLeft += 360.0;
    double dRight = pData1->az - az;
    if (dRight < 0.0) dRight += 360.0;
    rate = dLeft / (dLeft + dRight);

    return v0 + (v1 - v0) * rate;
}


// ʹ��OMP������CPU���߳�����
void GRadarAlgorithm::interpolateImageOMP(const GRadialSurf* surface, const size_t width, const size_t height,
    const double longitude, const double latitude, const double elev,
    const QRectF& bound, vtkColorTransferFunction* mColorTF,
    double* points, uchar* imageData)
{
    const double dx = bound.width() / (width - 1);
    const double dy = bound.height() / (height - 1);

    // �״�վ������
    const double slon = toRadian(longitude);
    const double slat = toRadian(latitude);
    const double elev_add_RE_div_RN = (elev + RE) / RN;

    // ׶������
    const double el = toRadian(surface->el);
    const double minR = surface->interval;
    const double maxR = minR * surface->radials[0]->points.size();

    // ���ԭʼ���������ͼ��ֵ, ��û����ƽ��
    //double* no_smooth = new double[width * height];

    // omp_set_num_threads(24);
    // #pragma omp parallel for num_threads(8)
    // omp���̼߳���, ����Ĭ���߳���Ϊ12
#pragma omp parallel
    {
#pragma omp for
        for (int row = 0; row < height; row++) {
            //if (row == 0) cout << "omp_get_num_threads:" << omp_get_num_threads() << endl;
            //const double dpY = bound.top() + (row + 0.5) * dy;
            const double dpY = bound.top() + row * dy;
            double rgb[3] = {};
            double dpX = bound.left() - dx;
            for (int col = 0; col < width; col++) {
                //const double dpX = bound.left() + (col + 0.5) * dx;
                //const double dpX = bound.left() + col * dx;
                dpX += dx;
                //const size_t index = row * width + col;
                // ī����ֱ��ת��γ�Ȼ���
                const double dlon = dpX / EHC * PI - slon;
                const double lat = atan(exp(dpY * PI / EHC)) * 2.0 - PI / 2.0;

                const double cos_lat = cos(lat);
                const double a = acos(sin(slat) * sin(lat) +
                    cos(slat) * cos_lat * cos(dlon));
                const double sin_a = sin(a);
                // ����·������
                const double r = fabs(RN * (a + el + asin(fma(elev_add_RE_div_RN, sin_a, -sin(a + el)))));
                if (r >= minR && r <= maxR) {
                    // ��֪��γ����λ�ǣ�azʵ����Ϊsin(az)
                    double az = cos_lat * sin(dlon) / sin_a;
                    if (az >= -1.0) {
                        if (az <= 1.0) {
                            az = asin(az);
                        } else {
                            az = PI / 2;
                        }
                    } else {
                        az = -PI / 2;
                    }
                    // ������λ��
                    const double dlat = lat - slat;
                    if (dlon >= 0) {
                        if (dlat < 0) az = PI - az;
                    } else {
                        if (dlat >= 0) az += (2 * PI);
                        else az = PI - az;
                    }
                    az = toAngle(az);
                    double value = interplateElRadialData(surface, az, r, INVALID_VALUE);
                    //no_smooth[index] = interplateElRadialData(surface, az, r, INVALID_VALUE);

                    //const double value = no_smooth[row * width + col];
                    if (value < -5.0 || value > 70.0) continue;
                    mColorTF->GetColor(value, rgb);
                    uchar* current = &imageData[((height - row - 1) * width + col) * 4];
                    current[0] = 255 * rgb[0];
                    current[1] = 255 * rgb[1];
                    current[2] = 255 * rgb[2];
                    current[3] = 255;
                } else {
                    //no_smooth[index] = INVALID_VALUE;
                }
            }
        }
        // ͬ������
        //#pragma omp barrier
        //// ��ͼ�����ƽ��
        //#pragma omp for
        //for (int row = 0; row < height; row++) {
        //    for (size_t col = 0; col < width; col++) {
        //        double rgb[3] = {};
        //        if (row == 0 || row == height - 1 || col == 0 || col == width - 1) {
        //            const double value = no_smooth[row * width + col];
        //            if (value < 10.0 || value > 80.0) continue;
        //            colorTF->GetColor(value, rgb);
        //            uchar* current = &imageData[((height - row - 1) * width + col) * 4];
        //            current[0] = 255 * rgb[0];
        //            current[1] = 255 * rgb[1];
        //            current[2] = 255 * rgb[2];
        //            current[3] = 255;
        //            continue;
        //        }
        //        size_t n = 0;
        //        double total = 0.0;
        //        // ---------------------------------
        //        double* p = &no_smooth[(row - 1) * width];
        //        if (p[col - 1] != INVALID_VALUE) {
        //            n++;
        //            total += p[col - 1];
        //        }
        //        if (p[col] != INVALID_VALUE) {
        //            n++;
        //            total += p[col];
        //        }
        //        if (p[col + 1] != INVALID_VALUE) {
        //            n++;
        //            total += p[col + 1];
        //        }
        //        // ---------------------------------
        //        p = &no_smooth[row * width];
        //        if (p[col - 1] != INVALID_VALUE) {
        //            n++;
        //            total += p[col - 1];
        //        }
        //        if (p[col] != INVALID_VALUE) {
        //            n++;
        //            total += p[col];
        //        }
        //        if (p[col + 1] != INVALID_VALUE) {
        //            n++;
        //            total += p[col + 1];
        //        }
        //        // ---------------------------------
        //        p = &no_smooth[(row + 1) * width];
        //        if (p[col - 1] != INVALID_VALUE) {
        //            n++;
        //            total += p[col - 1];
        //        }
        //        if (p[col] != INVALID_VALUE) {
        //            n++;
        //            total += p[col];
        //        }
        //        if (p[col + 1] != INVALID_VALUE) {
        //            n++;
        //            total += p[col + 1];
        //        }
        //        if (n > 0) {
        //            double value = total / n;
        //            if (value < 10.0 || value > 80.0) continue;
        //            colorTF->GetColor(value, rgb);
        //            uchar* current = &imageData[((height - row - 1) * width + col) * 4];
        //            //
        //            //*p = value;
        //            //
        //            current[0] = 255 * rgb[0];
        //            current[1] = 255 * rgb[1];
        //            current[2] = 255 * rgb[2];
        //            current[3] = 255;
        //        }
        //        //else {
        //        //    points[row * width + col] = INVALID_VALUE;
        //        //}
        //    }
        //}
    }
    //delete[] no_smooth;
}
// ʹ��OMP������CPU���߳�����õ��״�2Dͼ���imageData
void GRadarAlgorithm::interpolateImageGrayOMP(const GRadialSurf* surface, const size_t width, const size_t height,
    const double longitude, const double latitude, const double elev,
    const QRectF& bound, vtkColorTransferFunction* mColorTF,
    double* points, uchar* imageData)
{
    const double dx = bound.width() / (width - 1);
    const double dy = bound.height() / (height - 1);

    // �״�վ������
    const double slon = toRadian(longitude);
    const double slat = toRadian(latitude);
    const double elev_add_RE_div_RN = (elev + RE) / RN;

    // ׶������
    const double el = toRadian(surface->el);
    const double minR = surface->interval;
    const double maxR = minR * surface->radials[0]->points.size();

    // omp_set_num_threads(24);
    // #pragma omp parallel for num_threads(8)
    // omp���̼߳���, ����Ĭ���߳���Ϊ12
#pragma omp parallel
    {
#pragma omp for
        for (int row = 0; row < height; row++) {
           
            const double dpY = bound.top() + row * dy;
            double rgb[3] = {};
            double dpX = bound.left() - dx;
            for (int col = 0; col < width; col++) {
                
                dpX += dx;
                //const size_t index = row * width + col;
                // ī����ֱ��ת��γ�Ȼ���
                const double dlon = dpX / EHC * PI - slon;
                const double lat = atan(exp(dpY * PI / EHC)) * 2.0 - PI / 2.0;

                const double cos_lat = cos(lat);
                const double a = acos(sin(slat) * sin(lat) +
                    cos(slat) * cos_lat * cos(dlon));
                const double sin_a = sin(a);
                // ����·������
                const double r = fabs(RN * (a + el + asin(fma(elev_add_RE_div_RN, sin_a, -sin(a + el)))));
                if (r >= minR && r <= maxR) {
                    // ��֪��γ����λ�ǣ�azʵ����Ϊsin(az)
                    double az = cos_lat * sin(dlon) / sin_a;
                    if (az >= -1.0) {
                        if (az <= 1.0) {
                            az = asin(az);
                        }
                        else {
                            az = PI / 2;
                        }
                    }
                    else {
                        az = -PI / 2;
                    }
                    // ������λ��
                    const double dlat = lat - slat;
                    if (dlon >= 0) {
                        if (dlat < 0) az = PI - az;
                    }
                    else {
                        if (dlat >= 0) az += (2 * PI);
                        else az = PI - az;
                    }
                    az = toAngle(az);
                    double value = interplateElRadialData(surface, az, r, INVALID_VALUE);
                    
                    if (value < -5.0 || value > 70.0) continue;
                    imageData[row * height + col] = (int)value+120.0;
                  
                }
                else {
                   
                }
            }
        }
     
    }
    
}
