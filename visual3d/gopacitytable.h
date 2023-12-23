#pragma once

#include <QVector>
#include <QPointF>
#include <QVector3D>

#define OPACITY_TABLE_DEPTH 256

class GOpacityTable
{
public:
	enum FuncType { Linear, Spline, Gauss };

protected:
	int						mFuncType;					// 函数类型
	bool					mClamping;					// 是否将范围外的透明度赋值为最近有效值的透明度
	QVector< QVector3D >	mKeyPoints;					// 关键点
	QVector<double>			mOpacityTable;				// 不透明度表
	double					mValueMin, mValueMax;		// 数值的范围
	double					mValueInv;					// 数值间隔

public:
	GOpacityTable();

	GOpacityTable(const GOpacityTable& other);

	~GOpacityTable();

	GOpacityTable& operator = (const GOpacityTable& other);

	void setFuncType(int funcType);

	int getFuncType() const;

	void setClamping(bool clamping);

	bool getClamping() const;

	void setValueRange(double min, double max);

	void getValueRange(double* min, double* max) const;

	double getValueMin() const;

	double getValueMax() const;

	double getValueInv() const;

	int keyPointCount() const
	{
		return mKeyPoints.count();
	}

	QPointF getKeyPoint(int idx) const;

	void setKeyOpacity(int idx, double opacity);

	void setKeyOpacity(int idx, double pos, double opacity);

	int insertKeyOpacity(double pos, double opacity);

	void removeKeyOpacity(int idx);

	double getKeySigma(int idx) const;

	void setKeySigma(int idx, double sigma);

	int opacityCount() const
	{
		return mOpacityTable.count();
	}

	double getOpacity(int idx) const;

	void generate(int n = OPACITY_TABLE_DEPTH);

	// 提取不透明表数组
	void fillMemUseOpacityTalbe(float* mem) const;

	void saveToIni(const QString& filename);

	void loadFromIni(const QString& filename);
};
