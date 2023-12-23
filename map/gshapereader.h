#pragma once

#include <string>
#include <fstream>

#include <qpoint.h>
#include <qvector.h>
#include <qstring.h>
#include <qpolygon.h>

//using namespace std;
using std::string;
using std::ifstream;

class Shape
{
public:
    static constexpr int TYPE_POINT = 1;
    static constexpr int TYPE_LINE = 3;
    static constexpr int TYPE_POLYGON = 5;

    int mrecordnumber;//��¼��
    int mcontentlength;//��¼���ݵĳ���
    int mShapeType;//��������
    double mbox[4];//���α߽�
    int mPartsNum;//���ɵ�ǰ��״Ŀ����ӻ��ĸ���
    int mPointsNum;//���ɵ�ǰ��״Ŀ������������������

    QVector<int> mRecordParts;//�洢�ӻ��ĸ���
    QVector<int> mRecordPoints;//�洢��ĸ���
    QVector<int> mPartsIndexList;//ÿ���ӻ���mPoints�����е���ʼλ��
    QVector<QPointF> mPoints;//��¼�����е�������Ϣ

    // ��ȡ��ǰshape�ļ��е����ж�����б�(����mShapeType==TYPE_POLYGON)
    QVector<QPolygonF> getPolygonList() const;
};

//�ļ�ͷ
class ShapeFileHead
{
public:

public:
    int mfilecode;//�ļ����
    int munused[5];//���û�б�ʹ��
    int mfilelength;//�ļ�����
    int mversion;//�汾��
    int mShapeType;//��������


    double mxmin;//X�����ϵ���Сֵ	
    double mymin;//Y�����ϵ���Сֵ	
    double mxmax;//X�����ϵ����ֵ	
    double mymax;//Y�����ϵ����ֵ	
    double mzmin;//Z�����ϵ���Сֵ	
    double mzmax;//Z�����ϵ����ֵ	
    double mmmin;//M�����ϵ���Сֵ	
    double mmmax;//M�����ϵ����ֵ	
};


class ShapeReader
{
public:
    //��ȡ.shp�ļ�
    //static Shape* read(const char* path);
    static Shape* read(const QString& path);

private:
    //��ȡ.shp�ļ� �ļ�ͷ
    static int readFileHead(ifstream& ifs);
    //��ȡ��״Ŀ����Ϣ
    static Shape* readPoint(ifstream& ifs);
    //��ȡ��״Ŀ����Ϣ
    static Shape* readLine(ifstream& ifs);
    //��ȡ��״Ŀ����Ϣ
    static Shape* readPolygon(ifstream& ifs);
};
