#pragma once

#include <string>
#include <fstream>

#include <qpoint.h>
#include <qvector.h>
#include <qstringlist.h>
#include <qstring.h>

using std::string;
using std::vector;
using std::ifstream;

class DbfField
{
public:
    char mName[11] = {0};//字段的名称
    char mDataType = 0;//字段的类型
    unsigned char mFieldLength = 0;//字段的长度
    char mFieldContent[100];//字段的内容
    vector<string> mFieldList;//存储字段的内容
};

class Dbf
{
public:
    int mRecordNum = 0; //dbf文件中的记录条数，即，行数
    int mFieldNum = 0; //dbf文件中的字段数，即，列数
    short mBytesOfFileHead = 0; //dbf文件头中的字节数
    short mBytesOfEachRecord = 0; //每一条记录的字节数
    DbfField* dbfField = nullptr; //创建某个字段
    vector<DbfField*> mTable;//存储所有字段的内容

    ~Dbf()
    {
        for (auto item : mTable) {
            delete item;
        }
    }
};

class DbfReader
{
public:
    //读取dbf文件
    static Dbf* read(const char* path);
private:
    //读取dbf文件头
    static Dbf* readDbfHead(ifstream& ifs);
    //读取dbf记录内容
    static Dbf* readDbfRecord(ifstream& ifs, Dbf* dbfHead);
};


