#include <math.h>
#include <QDir>
#include <QFileInfo>
#include "gutils.h"

const QString QStringNull = QString("");

void gSwapByte4(char *to, const char *from)
{
	unsigned char tmp;
	tmp = from[0];
	to[0] = from[3];
	to[3] = tmp;
	tmp = from[1];
	to[1] = from[2];
	to[2] = tmp;
}

void gSwapByte2(char *to, const char *from)
{
	unsigned char tmp;
	tmp = from[0];
	to[0] = from[1];
	to[1] = tmp;
}

void gSkipPadBytes(QDataStream &stream, int nbytes)
{
	stream.device()->seek(stream.device()->pos()+nbytes);
}

bool gFileCopy(const QString &dst, const QString &src)
{
	QFile srcFile(src);
	QFile dstFile(dst);

	if(!srcFile.open(QIODevice::ReadOnly)) return false;
	if(!dstFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) 
	{
		srcFile.close();
		return false;
	}

	uint len, bufLen, size;
	char buf[1024];
	bufLen = 1024;
	size = srcFile.size();
	for(len=bufLen; len<size; len+=bufLen)
	{
		srcFile.read(buf, bufLen);
		dstFile.write(buf, bufLen);
	}
	len = size-len+bufLen;
	srcFile.read( buf, len );
	dstFile.write( buf, len );
	dstFile.flush();

	srcFile.close();
	if(dstFile.error() != QFile::NoError) 
	{
		dstFile.close();
		return false;
	}
	dstFile.close();
	return true;
}

double gAdjustStep(double step0, int &exp)
{
	exp = qRound(log10(step0));
	double base = pow(10.0, exp-1);
	double adt = 1;
	if(exp < 0)
	{
		adt = pow(10.0, -exp);
	}
	step0 *= adt;
	base *= adt;
	step0 = (int)(step0 / base) * base;
	step0 /= adt;
	return step0;
}

double gAdaptStep(double step)//修正步长
{
	double exp;

	exp = 1.0;
	while(step < 10)
	{
		step *= 10;
		exp /= 10;
	}
	while(step > 100)
	{
		step /= 10;
		exp *= 10;
	}

	step = qRound(step/5)*5*exp;
	return step;
}

bool gDeleteDir(const QString &path )
{
	QDir dir(path);
	if(path.isEmpty()) return false;
	if(!dir.exists()) 
	{
		//看是否传入的路径为一个文件的路径,是的话删除文件
		QFileInfo fl(path);
		if(fl.isFile())
		{
			QFile::remove(path);
			return true;
		}	
	}
	//设置过滤 这里是所有的文件
	dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot); 
	// 获取所有的文件信息
	QFileInfoList fileList = dir.entryInfoList(); 
	//遍历文件信息，是文件则删除
	foreach (QFileInfo file, fileList)
	{ 
		if (file.isFile())
		{ 
			file.dir().remove(file.fileName());
		}else
		{
			gDeleteDir(file.absoluteFilePath());
		}
	}
	// 删除空文件夹
	return dir.rmdir(dir.absolutePath());

}

