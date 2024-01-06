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

// ������������һ���������
class GRadialPoint
{
public:
    double x = 0.0;         // �洢��ά��ͼʱ��Webī����X����
    double y = 0.0;         // �洢��ά��ͼʱ��Webī����Y����
    double value = 0.0;     // �״�ɨ��ֵ

    GRadialPoint(double value) :x(0.0), y(0.0), value(value)
    {

    }
};

// һ�������������ϵ�����
class GRadialLine
{
public:
    double el = 0.0;                    // ���ǽǶ�
    double elRadian = 0.0;              // ���ǻ��� ��=180�� ��/2=90��

    double az = 0.0;                    // ��λ�ǽǶ�
    double azRadian = 0.0;              // ��λ�ǻ���

    vector<GRadialPoint> points;        // �״�ɨ�赽�����ݵ�

    GRadialLine() = default;

    // �ƶ����캯��
    GRadialLine(GRadialLine&& line) noexcept;

    // �ƶ���ֵ����
    GRadialLine& operator= (GRadialLine&& surf) noexcept;
};

// ׶������
class GRadialSurf
{
public:
    double interval = 0.0;              // �ֱ���,ÿ�����ݵ�ļ��
    double el = 0.0;                    // ƽ�����ǽǶ�
    double elRadian = 0.0;              // ƽ�����ǻ���
    vector<GRadialLine*> radials;       // ��������

    bool isConvert = false; // ����ϵ�Ƿ���Webī��������
    QRectF bound; // ׶��Webī���з�Χ�߽�

    GRadialSurf() = default;

    // �ƶ����캯��
    GRadialSurf(GRadialSurf&& surf) noexcept;

    // �ƶ���ֵ����
    GRadialSurf& operator= (GRadialSurf&& surf) noexcept;

    ~GRadialSurf();

    void sort();

    void clear();
};


// �״�������
class GRadarVolume
{
public:
    QString siteCode; // վ����
    QString siteName; // վ������
    QString startTime; // ɨ�迪ʼʱ��

    int surfNum = 0; // ɨ��׶������

    double longitude = 0; // վ�㾭��
    double latitude = 0; // վ��γ��
    double elev = 0; // վ�㺣�θ߶�

    double minEl = 0; // ��С����
    double maxEl = 0; // �������
    int	maxPointCount = 0; // ����������
    QVector<GRadialSurf*> surfs; // ׶���б�

    QRect bound; // Webī���з�Χ�߽�
    QString path; // �ļ�·��


    GRadarVolume() = default;

    ~GRadarVolume();

    //// ��ȡ��Ϣ
    //void extractInfo(const basedataImage& bdi);

    // ��ȡָ��������
    void extractData(const basedataImage& bdi, int dataType, double invalidValue);

    //����Ԥ��volume�޸����һ֡�״�����
    void modifyData(basedataImage& bdi, int dataType);

    // ͳ�������巶Χ
    void calcBoundBox(double& minX, double& minY, double& minZ, double& maxX, double& maxY, double& maxZ) const;

    // ��Ч����뾶������
    //void gridding1(double x0, double y0, double z0,
    //               int nx, int ny, int nz,
    //               double dx, double dy, double dz,
    //               double invalidValue, double* output) const;

    // ���Ǵ�������������
    void gridding(double x0, double x1,
        double y0, double y1,
        double z0, double z1,
        int nx, int ny, int nz,
        double invalidValue, float* output) const;

    // ̽�⿼�Ǵ��������������ЧZ����
    double detectMaxValidZ(double x0, double x1,
        double y0, double y1,
        double z0, double z1,
        int nx, int ny, int nz,
        double min, double max) const;

    void clear();
    // �����µ�׶��
    GRadialSurf* generateNewSurf(vtkImageData* imageData);
private:

    //// ��侶������
    //void fillRadialData(GRadialLine* pData, cv_geneMom& mom, double invalidValue);

    // ���ֲ�����������
    int findElRange(double el) const;

    // ˳�������������
    int findElRange1(double el) const;

    // �������ݲ�ֵ
    double interplateRadialData(double el, double az, double d, double invalidValue) const;

    // ��ֵͬһ��������ָ����λ�Ǻ;������ֵ
    double interplateElRadialData(int isurf, double az, double d, double invalidValue) const;
};

