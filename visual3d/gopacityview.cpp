#include <QPainter>
#include <QPaintEvent>
#include <QInputDialog>
#include "gopacityview.h"

GOpacityView::GOpacityView(QWidget* parent, Qt::WindowFlags f) : GGraphView(parent, f)
{
    mIsLButtonDown = false;
    mSelIndex = -1;
    mGainMinValue = mGainMaxValue = 0;
}

GOpacityView::~GOpacityView()
{

}

void GOpacityView::setOpacityTable(const GOpacityTable& table)
{
    mOpacityTable = table;

    setDataRange(mOpacityTable.getValueMin(), 0, mOpacityTable.getValueMax(), 1);
    mGainMinValue = mOpacityTable.getValueMin();
    mGainMaxValue = mOpacityTable.getValueMax();
}

const GOpacityTable& GOpacityView::getOpacityTable()
{
    return mOpacityTable;
}

void GOpacityView::setFuncType(int funcType)
{
    mOpacityTable.setFuncType(funcType);
    mOpacityTable.generate();
    update();
}

void GOpacityView::setClamping(bool clamping)
{
    mOpacityTable.setClamping(clamping);
}

double GOpacityView::getGainMinValue()
{
    return mGainMinValue;
}

double GOpacityView::getGainMaxValue()
{
    return mGainMaxValue;
}

void GOpacityView::setGainMinValue(double v)
{
    mGainMinValue = v;
}

void GOpacityView::setGainMaxValue(double v)
{
    mGainMaxValue = v;
}

void GOpacityView::drawKeyPoints(QPainter* p)
{
    p->setPen(Qt::black);
    p->setBrush(Qt::blue);

    double x0 = mOpacityTable.getValueMin();
    double w = mOpacityTable.getValueMax() - mOpacityTable.getValueMin();
    for (int i = 0; i < mOpacityTable.keyPointCount(); i++)
    {
        if (mSelIndex == i) p->setBrush(Qt::red);
        else p->setBrush(Qt::blue);

        QPointF pt = mOpacityTable.getKeyPoint(i);
        int x = lpXTodpX(x0 + pt.x() * w);
        int y = lpYTodpY(pt.y());
        p->drawEllipse(x - 4, y - 4, 10, 10);
    }
}

void GOpacityView::drawOpacity(QPainter* p)
{
    int n = mOpacityTable.opacityCount();
    if (n < 2) return;

    p->setPen(QPen(Qt::darkGreen, 2));

    double lx0 = mOpacityTable.getValueMin();
    double ldx = mOpacityTable.getValueInv();

    int x0, y0, x1, y1;

    x0 = lpXTodpX(lx0);
    y0 = lpYTodpY(mOpacityTable.getOpacity(0));
    for (int i = 1; i < n; i++)
    {
        x1 = lpXTodpX(lx0 + ldx * i);
        y1 = lpYTodpY(mOpacityTable.getOpacity(i));

        int dx = x1 - x0;
        int dy = y1 - y0;
        if (dx * dx + dy * dy >= 9)
        {
            p->drawLine(x0, y0, x1, y1);
            x0 = x1;	y0 = y1;
        }
    }
}

void GOpacityView::drawGainValue(QPainter* p)
{
    int x0 = lpXTodpX(mGainMinValue);
    int x1 = lpXTodpX(mGainMaxValue);

    p->setClipRect(mViewRect.left(), mViewRect.top(), mViewRect.width(), mViewRect.height());

    p->fillRect(mViewRect.left(), mViewRect.top(), x0 - mViewRect.left(), mViewRect.height(), Qt::lightGray);
    p->fillRect(x1, mViewRect.top(), mViewRect.right() - x1, mViewRect.height(), Qt::lightGray);

    p->setClipping(false);

    QPolygon points;
    int y0 = mViewRect.top();

    if (x0 >= mViewRect.left())
    {
        p->setPen(QPen(Qt::blue, 2));
        p->drawLine(x0, mViewRect.top(), x0, mViewRect.bottom());

		points.setPoints(5, x0, y0 - 2, x0 - 5, y0 - 7, x0 - 5, y0 - 9, x0 + 5, y0 - 9, x0 + 5, y0 - 7);
		p->setPen(Qt::black);
		p->setBrush(Qt::blue);
		p->drawPolygon(points);
    }

    if (x0 <= mViewRect.right())
    {
        p->setPen(QPen(Qt::red, 2));
        p->drawLine(x1, mViewRect.top(), x1, mViewRect.bottom());

        points.setPoints(5, x1, y0 - 2, x1 - 5, y0 - 7, x1 - 5, y0 - 9, x1 + 5, y0 - 9, x1 + 5, y0 - 7);
        p->setPen(Qt::black);
        p->setBrush(Qt::red);
        p->drawPolygon(points);
    }
}

void GOpacityView::paintEvent(QPaintEvent* event)
{
    if (mViewRect.isEmpty()) return;

    QPainter  p(this);

	drawBackground(&p, event->rect());

    //drawGainValue(&p);

	drawHRuler(&p);
	drawVRuler(&p);
	p.setPen(Qt::black);
    p.setBrush(Qt::NoBrush);
	p.drawRect(mViewRect.left(), mViewRect.top(), mViewRect.width() - 1, mViewRect.height() - 1);

    p.setClipRect(mViewRect.left() - 6, mViewRect.top() - 6, mViewRect.width() + 12, mViewRect.height() + 12);

    p.setRenderHint(QPainter::Antialiasing);

    drawOpacity(&p);

    drawKeyPoints(&p);

    p.setRenderHint(QPainter::Antialiasing, false);
}

int GOpacityView::ptInPoint(int x, int y, int diff)
{
    int diff2 = diff * diff;
    double lx0 = mOpacityTable.getValueMin();
    double lw = mOpacityTable.getValueMax() - lx0;
    for (int i = 0; i < mOpacityTable.keyPointCount(); i++)
    {
        QPointF pt = mOpacityTable.getKeyPoint(i);
        int kx = lpXTodpX(lx0 + pt.x() * lw);
        int ky = lpYTodpY(pt.y());

        int dx = kx - x;
        int dy = ky - y;
        if (dx * dx + dy * dy <= diff2) return i;
    }

    return -1;
}

int GOpacityView::ptInRange(int x, int diff)
{
    double lx0 = mOpacityTable.getValueMin();
    double lw = mOpacityTable.getValueMax() - lx0;
    for (int i = 1; i < mOpacityTable.keyPointCount(); i++)
    {
        int x0 = lpXTodpX(lx0 + mOpacityTable.getKeyPoint(i - 1).x() * lw);
        int x1 = lpXTodpX(lx0 + mOpacityTable.getKeyPoint(i).x() * lw);
        if (x0 + diff <= x && x <= x1 - diff)
        {
            return i - 1;
        }
    }

    return -1;
}

void GOpacityView::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::RightButton)
    {
        GGraphView::mousePressEvent(e);
    } 
    else if (e->button() == Qt::LeftButton)
    {
        int idx = ptInPoint(e->x(), e->y());
        if (e->modifiers() & Qt::ShiftModifier)
        {
            if (idx > 0 && idx < mOpacityTable.keyPointCount() - 1)
            {
                mOpacityTable.removeKeyOpacity(idx);
                mOpacityTable.generate();
                update();
                emit valueChanged(0);
            }
        } 
        else
        {
            if (idx >= 0)
            {
                mSelIndex = idx;
                update();
            } 
            else
            {
                /*if (!mViewRect.contains(e->pos()))
                {
                    int y0 = mViewRect.top() - 6;
                    int dx = lpXTodpX(mGainMinValue) - e->x();
                    int dy = e->y() - y0;
                    // 选中左边值线
                    if (dx * dx + dy * dy <= 25)
                    {
                        mSelIndex = -100;
                        return;
                    }
                    
                    // 选中右边值线
                    dx = lpXTodpX(mGainMaxValue) - e->x();
                    if (dx * dx + dy * dy <= 25)
                    {
                        mSelIndex = -200;
                    }

                    return;
                }*/

                int ir = ptInRange(e->x());
                if (ir >= 0)
                {
                    double lx0 = mOpacityTable.getValueMin();
                    double lw = mOpacityTable.getValueMax() - lx0;
                    double lx = (dpXTolpX(e->x()) - lx0) / lw;
                    double ly = dpYTolpY(e->y());
                    if (ly < 0) ly = 0;
                    if (ly > 1) ly = 1;
                    mSelIndex = mOpacityTable.insertKeyOpacity(lx, ly);
                    mOpacityTable.generate();
                    update();
                    emit valueChanged(0);
                }
            }
        }
    }
}

void GOpacityView::mouseMoveEvent(QMouseEvent* e)
{
    GGraphView::mouseMoveEvent(e);

    if (mSelIndex >= 0)
    {
        double ly = dpYTolpY(e->y());
        if (ly < 0) ly = 0;
        if (ly > 1) ly = 1;

		double lx0 = mOpacityTable.getValueMin();
		double lw = mOpacityTable.getValueMax() - lx0;
        double lx = (dpXTolpX(e->x()) - lx0) / lw;
        if (lx < 0) lx = 0;
        if (lx > 1) lx = 1;
        double eps = 0.005;
        if (mSelIndex > 0 && mSelIndex < mOpacityTable.keyPointCount() - 1)
        {
            QPointF key = mOpacityTable.getKeyPoint(mSelIndex - 1);
            if (lx < key.x()+eps) lx = key.x() + eps;
            else
            {
                key = mOpacityTable.getKeyPoint(mSelIndex + 1);
                if (lx > key.x()-eps) lx = key.x() - eps;
            }

            mOpacityTable.setKeyOpacity(mSelIndex, lx, ly);
        }
        else
        {
            mOpacityTable.setKeyOpacity(mSelIndex, ly);
        }
       
        mOpacityTable.generate();
        update();
    }
    /*else
    {
        if (mSelIndex == -100)
        {
            double eps = mOpacityTable.getValueInv();
            mGainMinValue = dpXTolpX(e->x());
            if (mGainMinValue < mOpacityTable.getValueMin()) mGainMinValue = mOpacityTable.getValueMin();
            if (mGainMinValue > mGainMaxValue-eps) mGainMinValue = mGainMaxValue-eps;
            update();
        }
        else if (mSelIndex == -200)
        {
            double eps = mOpacityTable.getValueInv();
			mGainMaxValue = dpXTolpX(e->x());
			if (mGainMaxValue > mOpacityTable.getValueMax()) mGainMaxValue = mOpacityTable.getValueMax();
			if (mGainMaxValue < mGainMinValue+eps) mGainMaxValue = mGainMinValue + eps;
            update();
        }
    }*/
}

void GOpacityView::mouseReleaseEvent(QMouseEvent* e)
{
    GGraphView::mouseReleaseEvent(e);

    if (e->button() == Qt::LeftButton)
    {
        if (mSelIndex >= 0) emit valueChanged(0);
        //else if (mSelIndex == -100 || mSelIndex == -200) emit valueChanged(1);
        mSelIndex = -1;
        update();
    }
}

void GOpacityView::mouseDoubleClickEvent(QMouseEvent* e)
{
    /*if (e->button() == Qt::LeftButton)
    {
        int idx = ptInPoint(e->x(), e->y());
        if (idx >= 0)
        {
            double sigma = mOpacityTable.getKeySigma(idx);
            bool ok;
            sigma = QInputDialog::getDouble(this, u8"Gauss函数参数设置", u8"Sigma参数", sigma, 1e-4, 0.5, 4, &ok);
            if (ok)
            {
                mOpacityTable.setKeySigma(idx, sigma);
                mOpacityTable.generate();
                update();
                emit valueChanged(0);
            }
        }
    }*/
}