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

    int mrecordnumber;//记录号
    int mcontentlength;//记录内容的长度
    int mShapeType;//几何类型
    double mbox[4];//几何边界
    int mPartsNum;//构成当前面状目标的子环的个数
    int mPointsNum;//构成当前面状目标所包含的坐标点个数

    QVector<int> mRecordParts;//存储子环的个数
    QVector<int> mRecordPoints;//存储点的个数
    QVector<int> mPartsIndexList;//每个子环在mPoints数组中的起始位置
    QVector<QPointF> mPoints;//记录了所有的坐标信息

    // 获取当前shape文件中的所有多边形列表(仅当mShapeType==TYPE_POLYGON)
    QVector<QPolygonF> getPolygonList() const;
};

//文件头
class ShapeFileHead
{
public:

public:
    int mfilecode;//文件编号
    int munused[5];//五个没有被使用
    int mfilelength;//文件长度
    int mversion;//版本号
    int mShapeType;//几何类型


    double mxmin;//X方向上的最小值	
    double mymin;//Y方向上的最小值	
    double mxmax;//X方向上的最大值	
    double mymax;//Y方向上的最大值	
    double mzmin;//Z方向上的最小值	
    double mzmax;//Z方向上的最大值	
    double mmmin;//M方向上的最小值	
    double mmmax;//M方向上的最大值	
};


class ShapeReader
{
public:
    //读取.shp文件
    //static Shape* read(const char* path);
    static Shape* read(const QString& path);

private:
    //读取.shp文件 文件头
    static int readFileHead(ifstream& ifs);
    //读取点状目标信息
    static Shape* readPoint(ifstream& ifs);
    //读取线状目标信息
    static Shape* readLine(ifstream& ifs);
    //读取面状目标信息
    static Shape* readPolygon(ifstream& ifs);
};
