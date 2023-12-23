#include "gdbfreader.h"

#include <string>
#include <fstream>

#include <QDebug>
#include <qstring.h>
#include <QTextCodec>

using std::ifstream;
using std::ios;

//读取dbf文件
Dbf* DbfReader::read(const char* path)
{
    ifstream ifs;
    ifs.open(path, ios::binary | ios::out);
    if (ifs.fail()) {
        ifs.close();
        return nullptr;
    }
    Dbf* dbfHead = DbfReader::readDbfHead(ifs);//读文件头
    Dbf* dbf = readDbfRecord(ifs, dbfHead);//文件记录内容
    ifs.close();
    return dbf;
}

//读取dbf文件头
Dbf* DbfReader::readDbfHead(ifstream& ifs)
{
    Dbf* dbfHead = new Dbf();
    ifs.seekg(4, ios::beg);
    ifs.read((char*)&dbfHead->mRecordNum, sizeof(int));//读取文件头中的记录条数
    ifs.read((char*)&dbfHead->mBytesOfFileHead, sizeof(short));//读取文件头的字节数
    ifs.read((char*)&dbfHead->mBytesOfEachRecord, sizeof(short));//读取一条记录的文件长度
    dbfHead->mFieldNum = (dbfHead->mBytesOfFileHead - 33) / 32;//计算字段数
    for (int i = 0; i < dbfHead->mFieldNum; i++)//读取文件头中关于字段的描述
    {
        ifs.seekg(32 + 32 * i, ios::beg);

        dbfHead->dbfField = new DbfField();

        for (int j = 0; j < 11; j++) {//读取字段名
            ifs.read(dbfHead->dbfField->mName + j, sizeof(char));
        }

        ifs.read(&dbfHead->dbfField->mDataType, sizeof(char));//读取字段的类型
        ifs.seekg(4, ios::cur);
        ifs.read((char*)&dbfHead->dbfField->mFieldLength, sizeof(char));//读取字段的长度
        dbfHead->mTable.push_back(dbfHead->dbfField);//将读取的字段内容存入		
    }
    char terminator;
    return dbfHead;
}

//读取dbf文件记录内容
Dbf* DbfReader::readDbfRecord(ifstream& ifs, Dbf* dbfHead)
{
    ifs.seekg(dbfHead->mBytesOfFileHead, ios::beg);
    char deleteTag;
    for (int i = 0; i < dbfHead->mRecordNum; i++)//读取每一个属性信息
    {
        ifs.read(&deleteTag, sizeof(char));
        for (unsigned int j = 0; j < dbfHead->mTable.size(); j++)//读取每一个字段的信息
        {
            const size_t len = dbfHead->mTable[j]->mFieldLength;
            //char* record = new char[len] {0};
            string record(len, 0);
            for (int k = 0; k < len; k++) {
                ifs.read(&record[k], sizeof(char));
            }
            dbfHead->mTable[j]->mFieldList.push_back(std::move(record));
        }
    }
    return dbfHead;
}