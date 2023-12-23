#include <QSettings>
#include "gopacitytable.h"
#include "gmath.h"

#define OPACITY_SIGMA	0.05

struct SplineParam	// 一段三次曲线系数
{
	// a*t^3 + b*t^2 + c*t + d
	double a, b, c, d;

	SplineParam()
	{
		a = b = c = d = 0;
	}
};

class GSpline1d
{
private:
	QVector<QPointF>		mPoints;
	QVector<SplineParam>	mParams;

public:
	GSpline1d() {}
	~GSpline1d() {}

    void clear()
    {
		mPoints.clear();
		mParams.clear();
    }

	// 插值计算
    void interpolate(const QPointF* points, int count)
    {
		clear();

		for (int i = 0; i < count; i++)
		{
			mPoints << points[i];
		}

		double* a = new double[count - 1];
		double* b = new double[count];
		double* c = new double[count - 1];
		double* d = new double[count];
		double* m = new double[count];

		a[count - 2] = 0;
		b[0] = b[count - 1] = 1;
		c[0] = 0;
		d[0] = d[count - 1] = 0;
		for (int i = 1; i < count - 1; i++)
		{
			double h0 = points[i].x() - points[i - 1].x();
			double h1 = points[i + 1].x() - points[i].x();
			a[i - 1] = h0;
			c[i] = h1;
			b[i] = 2 * (h0 + h1);
			d[i] = (points[i + 1].y() - points[i].y()) / h1 - (points[i].y() - points[i - 1].y()) / h0;
			d[i] *= 6.0;
		}

		gTridagSlove(a, b, c, d, count, m);

		for (int i = 0; i < count - 1; i++)
		{
			SplineParam param;
			param.a = points[i].y();
			double h = points[i + 1].x() - points[i].x();
			param.b = (points[i + 1].y() - points[i].y()) / h - 0.5 * h * m[i] - h * (m[i + 1] - m[i]) / 6.0;
			param.c = 0.5 * m[i];
			param.d = (m[i + 1] - m[i]) / (6 * h);
			mParams.append(param);
		}

		delete[]a;
		delete[]b;
		delete[]c;
		delete[]d;
		delete[]m;
    }

	// 计算第index段曲线, 弦长参数等于t时的样条坐标
    double get(double x)
    {
		if (x < mPoints.first().x()) return mPoints.first().y();
		if (x > mPoints.last().x()) return mPoints.last().y();

		double y = 0;
		for (int i = 0; i < mPoints.count() - 1; i++)
		{
			if (x >= mPoints[i].x() && x <= mPoints[i + 1].x())
			{
				double t = x - mPoints[i].x();
				y = mParams[i].a + mParams[i].b * t + mParams[i].c * t * t + mParams[i].d * t * t * t;
				break;
			}
		}
		return y;
    }
};

GOpacityTable::GOpacityTable()
{
    mValueMin = 0;
    mValueMax = 1;
    mClamping = false;

    mKeyPoints.append(QVector3D(0.0, 0.6, OPACITY_SIGMA));
    mKeyPoints.append(QVector3D(1.0, 0.6, OPACITY_SIGMA));
    mFuncType = Linear;
    generate(OPACITY_TABLE_DEPTH);
}

GOpacityTable::GOpacityTable(const GOpacityTable& other)
{
    *this = other;
}

GOpacityTable::~GOpacityTable()
{

}

GOpacityTable& GOpacityTable::operator= (const GOpacityTable& other)
{
    mFuncType = other.mFuncType;
    mClamping = other.mClamping;
    mKeyPoints = other.mKeyPoints;
    mOpacityTable = other.mOpacityTable;
    mValueMin = other.mValueMin;
    mValueMax = other.mValueMax;
    mValueInv = other.mValueInv;

    return *this;
}

void GOpacityTable::setFuncType(int funcType)
{
    mFuncType = funcType;
}

int GOpacityTable::getFuncType() const
{
    return mFuncType;
}

void GOpacityTable::setClamping(bool clamping)
{
    mClamping = clamping;
}

bool GOpacityTable::getClamping() const
{
    return mClamping;
}

void GOpacityTable::setValueRange(double min, double max)
{
    mValueMin = min;
    mValueMax = max;
}

void GOpacityTable::getValueRange(double* min, double* max) const
{
    *min = mValueMin;
    *max = mValueMax;
}

double GOpacityTable::getValueMin() const
{
    return mValueMin;
}

double GOpacityTable::getValueMax() const
{
    return mValueMax;
}

double GOpacityTable::getValueInv() const
{
    return mValueInv * (mValueMax - mValueMin);
}

QPointF GOpacityTable::getKeyPoint(int idx) const
{
    if (idx < 0 || idx >= keyPointCount()) return QPointF();

    return QPointF(mKeyPoints.at(idx).x(), mKeyPoints.at(idx).y());
}

void GOpacityTable::setKeyOpacity(int idx, double opacity)
{
    if (idx >= 0 && idx < keyPointCount())
    {
        mKeyPoints[idx].setY(opacity);
    }
}

void GOpacityTable::setKeyOpacity(int idx, double pos, double opacity)
{
	if (idx >= 0 && idx < keyPointCount())
	{
        mKeyPoints[idx].setX(pos);
		mKeyPoints[idx].setY(opacity);
	}
}

int GOpacityTable::insertKeyOpacity(double pos, double opacity)
{
    if (pos < 0 || pos > 1) return -1;

    int count = mKeyPoints.count();

    double p = 0.0;
    for (int i = 0; i < count; i++)
    {
        p = mKeyPoints[i].x();
        if (pos > p)
        {
            continue;
        } else if (pos == p)
        {
            mKeyPoints[i].setY(opacity);  //修改
            return i;
        } else if (pos < p)
        {
            mKeyPoints.insert(i, QVector3D(pos, opacity, OPACITY_SIGMA));
            return i;
        }
    }

    mKeyPoints.append(QVector3D(pos, opacity, OPACITY_SIGMA));
    return mKeyPoints.count() - 1;
}

void GOpacityTable::removeKeyOpacity(int idx)
{
    mKeyPoints.remove(idx);
}

double GOpacityTable::getOpacity(int idx) const
{
    return mOpacityTable[idx];
}

double GOpacityTable::getKeySigma(int idx) const
{
    return mKeyPoints.at(idx).z();
}

void GOpacityTable::setKeySigma(int idx, double sigma)
{
    mKeyPoints[idx].setZ(sigma);
}

void GOpacityTable::generate(int n)
{
    if (mKeyPoints.count() < 2) return;

    mOpacityTable.resize(n);

    int i;
    double x, dx;
    dx = 1.0 / (n - 1.0);
    mValueInv = dx;

    if (mFuncType == Linear)
    {
        int ipos = 0;
        for (int i = 0; i < n; i++)
        {
            x = i * dx;
            while (ipos <= keyPointCount() - 1 && x > mKeyPoints[ipos].x()) ipos++;
            if (ipos == 0) mOpacityTable[i] = mKeyPoints[ipos].y();
            else if (ipos == keyPointCount()) mOpacityTable[i] = mKeyPoints[ipos - 1].y();
            else
            {
                QPointF pt0 = mKeyPoints[ipos - 1].toPointF();
                QPointF pt1 = mKeyPoints[ipos].toPointF();
                double ly = pt0.y() + (pt1.y() - pt0.y()) * (x - pt0.x()) / (pt1.x() - pt0.x());
                if (ly < 0) ly = 0;
                if (ly > 1) ly = 1;
                mOpacityTable[i] = ly;
            }
        }
    } else if (mFuncType == Spline)
    {
        GSpline1d spline;
        QVector<QPointF> kpts;
        for (int i = 0; i < mKeyPoints.count(); i++)
        {
            kpts << QPointF(mKeyPoints[i].x(), mKeyPoints[i].y());
        }

        spline.interpolate(kpts.data(), mKeyPoints.count());
        for (int i = 0; i < n; i++)
        {
            x = i * dx;
            double ly = spline.get(x);
            if (ly < 0) ly = 0;
            if (ly > 1) ly = 1;
            mOpacityTable[i] = ly;
        }
    } else if (mFuncType == Gauss)
    {
        for (int i = 0; i < n; i++)
        {
            x = i * dx;
            double ly = 0;

            for (int j = 0; j < keyPointCount(); j++)
            {
                QVector3D kpt = mKeyPoints[j];
                double dx = kpt.x() - x;
                double sigma2 = 2 * kpt.z() * kpt.z();
                ly += kpt.y() * exp(-dx * dx / sigma2);
            }
            if (ly < 0) ly = 0;
            if (ly > 1) ly = 1;
            mOpacityTable[i] = ly;
        }
    }
}

void GOpacityTable::fillMemUseOpacityTalbe(float* mem) const
{
    for (int i = 0; i < mOpacityTable.count(); i++)
    {
        mem[i] = (float)mOpacityTable[i];
    }
}

void GOpacityTable::saveToIni(const QString& filename)
{
	QSettings settings(filename, QSettings::IniFormat);
	settings.beginGroup("OPACITY_TABLE");
	settings.setValue("FUNC_TYPE", mFuncType);
	settings.setValue("CLAMPING", mClamping);
	settings.setValue("MIN_VALUE", mValueMin);
	settings.setValue("MAX_VALUE", mValueMax);

    settings.beginWriteArray("KEY_POINTS");
    for (int i = 0; i < mKeyPoints.count(); i++)
    {
        settings.setArrayIndex(i);
        QVector3D pt3 = mKeyPoints.at(i);
        settings.setValue("X", pt3.x());
        settings.setValue("Y", pt3.y());
        settings.setValue("SIGMA", pt3.z());
    }
    settings.endArray();

	settings.endGroup();
}

void GOpacityTable::loadFromIni(const QString& filename)
{
	QSettings settings(filename, QSettings::IniFormat);
	settings.beginGroup("OPACITY_TABLE");

    mFuncType = settings.value("FUNC_TYPE", "0").toInt();
    mClamping = settings.value("CLAMPING", "false").toBool();
    mValueMin = settings.value("MIN_VALUE", "0").toDouble();
    mValueMax = settings.value("MAX_VALUE", "1").toDouble();

    mKeyPoints.clear();
    int n = settings.beginReadArray("KEY_POINTS");
    for (int i = 0; i < n; i++)
    {
        settings.setArrayIndex(i);
        double x, y, sigma;
        x = settings.value("X").toDouble();
        y = settings.value("Y").toDouble();
        sigma = settings.value("SIGMA").toDouble();
        mKeyPoints.append(QVector3D(x, y, sigma));
    }
    settings.endArray();

    settings.endGroup();

    if (n <= 0)
    {
		mKeyPoints.append(QVector3D(0.0, 0.6, OPACITY_SIGMA));
		mKeyPoints.append(QVector3D(1.0, 0.6, OPACITY_SIGMA));
    }

    generate();
}
