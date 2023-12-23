#include "ggraphview.h"
#include "gutils.h"
#include <QString>

GGraphView::GGraphView(QWidget * parent /*= 0*/, Qt::WindowFlags f /*= 0*/)
	:QWidget(parent)
{
	mIsDrawGrid = true;
	mZoomScale = 1;
	mOffsetX = 0;
	mMinOffsetX = 0;
	mOffsetY = 0;
	mMinOffsetY = 0;
	mIsDrag = false;
	mMaxLpX = 0;
	mMinLpX = 0;
	mMaxLpY = 0;
	mMinLpY = 0;
	mExtendRange = 0.0;
	mRulerDeltaX = 0;
	mRulerDeltaY = 0;
	mRulerXCount = 5;
	mRulerYCount = 5;
	mTitleMargin = 0;
	mIsFlipX = false;
	mIsFlipY = false;
	mMargins.setTopLeft(QPoint(40, 40));
	mMargins.setBottomRight(QPoint(40, 40));
	mBackgroundColor = Qt::white;
	mHRulerPos = TOP;
	mVRulerPos = LEFT;

	mMainTitleFont.setFamily("Arial");
	mMainTitleFont.setPointSize(10);

	mXYTitleFont.setFamily("Arial");
	mXYTitleFont.setPointSize(10);

	this->setMinimumSize(100, 100);
	mInfoLabel = NULL;
	setMouseTracking(true);
}

GGraphView::~GGraphView()
{

}

void GGraphView::setMargins(int marginLeft, int marginRight, int marginTop, int marginBottom)
{
	mMargins.setTop(marginTop);
	mMargins.setBottom(marginBottom);
	mMargins.setLeft(marginLeft);
	mMargins.setRight(marginRight);

	int marginRectWidth = width() - marginLeft - marginRight;
	int marginRectHight = height() - marginTop - marginBottom;
	mViewRect.setRect(marginLeft, marginTop, marginRectWidth, marginRectHight);
}

void GGraphView::resizeEvent(QResizeEvent* event)
{
	int marginRectWidth = width() - mMargins.left() - mMargins.right();
	int marginRectHeight = height() - mMargins.top() - mMargins.bottom();
	mViewRect.setRect(mMargins.left(), mMargins.top(), marginRectWidth, marginRectHeight);
}

void GGraphView::setTitle(const QString & title)
{
	this->mTitle = title;
}


void GGraphView::setTitleMargin(int margin)
{
	mTitleMargin = margin;
}

void GGraphView::setXTitle(const QString & xTitle)
{
	this->mXTitle = xTitle;
}

void GGraphView::setYTitle(const QString & yTitle)
{
	this->mYTitle = yTitle;
}

void GGraphView::setTitleFont(QFont font)
{
	this->mMainTitleFont = font;
}

void GGraphView::setXYTitleFont(QFont font)
{
	this->mXYTitleFont = font;
}

void GGraphView::clear()
{

}

void GGraphView::setInfoLabel(QLabel *label)
{
	mInfoLabel = label;
}

void GGraphView::setHRulerPos(GGraphView::HRulerPos pos)
{
	mHRulerPos = pos;
}

void GGraphView::setVRulerPos(GGraphView::VRulerPos pos)
{
	mVRulerPos = pos;
}

void GGraphView::setDrawGrid(bool isDrawGrid)
{
	this->mIsDrawGrid = isDrawGrid;
}

void GGraphView::setFlipX(bool isFlipX)
{
	mIsFlipX = isFlipX;

}

void GGraphView::setFlipY(bool isFlipY)
{
	mIsFlipY = isFlipY;

}

void GGraphView::setHRulerLdx(double ldx)
{
	mRulerDeltaX = ldx;
}

void GGraphView::setVRulerLdy(double ldy)
{
	mRulerDeltaY = ldy;
}

double GGraphView::getHRulerLdx()
{
	return mRulerDeltaX;
}

double GGraphView::getVRulerLdy()
{
	return mRulerDeltaY;
}

void GGraphView::setXRulerCount(int count)
{
	mRulerXCount = count;
}

void GGraphView::setYRulerCount(int count)
{
	mRulerYCount = count;
}

void GGraphView::setBackground(QColor cr)
{
	mBackgroundColor = cr;
}

void GGraphView::setDataRange(double minX, double minY, double maxX, double maxY)
{
	mMinLpX = minX;
	mMaxLpX = maxX;
	mMinLpY = minY;
	mMaxLpY = maxY;
}

void GGraphView::setXDataRange(double minX, double maxX)
{
	mMinLpX = minX;
	mMaxLpX = maxX;
}

void GGraphView::setYDataRange(double minY, double maxY)
{
	mMinLpY = minY;
	mMaxLpY = maxY;
}

double GGraphView::getRangeMinX()
{
	return mMinLpX;

}

double GGraphView::getRangeMaxX()
{
	return mMaxLpX;
}

double GGraphView::getRangeMinY()
{
	return mMinLpY;
}

double GGraphView::getRangeMaxY()
{
	return mMaxLpY;
}

void GGraphView::paintEvent(QPaintEvent * event)
{
	if(mViewRect.isEmpty()) return;

	QWidget::paintEvent(event);
	QPainter  p(this);
	drawBackground(&p, event->rect());
	drawHRuler(&p);
	drawVRuler(&p);
	p.setPen(Qt::black);
	p.drawRect(mViewRect.left(), mViewRect.top(), mViewRect.width()-1, mViewRect.height()-1);

	drawTitle(&p);
}

void GGraphView::wheelEvent(QWheelEvent * event)
{
	// 获取到用户缩放时鼠标的位置对应的逻辑值
	int mouseX = event->pos().x();
	int mouseY = event->pos().y();

	// 当滚动区不在绘图区不进行缩放
	if (mouseX < mViewRect.left()||mouseX > mViewRect.right() ||
		mouseY < mViewRect.top()||mouseY > mViewRect.bottom())
	{
		return;
	}


	double lxm = dpXTolpX(mouseX);
	double lym = dpYTolpY(mouseY);

	if (event->delta() > 0)
	{
		mZoomScale += 0.1;
	}
	else
	{
		mZoomScale -= 0.1;
		if (mZoomScale < 1)
		{
			 mZoomScale = 1;
		}
	}

	int w = qRound(mZoomScale * mViewRect.width());
	int h = qRound(mZoomScale * mViewRect.height());
	int dxc = lpXTodpX(lxm);
	int dyc = lpYTodpY(lym);

	mOffsetX += mouseX - dxc;
	mOffsetY += mouseY - dyc;

	if (mIsFlipX) 
	{
		mMinOffsetX = w - mViewRect.width();
		if(mOffsetX > mMinOffsetX) mOffsetX = mMinOffsetX;
		if(mOffsetX < 0) mOffsetX = 0;
	}
	else 
	{
		mMinOffsetX = mViewRect.width() - w;
		if (mOffsetX < mMinOffsetX) mOffsetX = mMinOffsetX;
		if (mOffsetX > 0) mOffsetX = 0;
	}
	
	if (mIsFlipY)
	{
		mMinOffsetY = mViewRect.height() - h;
		if (mOffsetY < mMinOffsetY) mOffsetY = mMinOffsetY;
		if (mOffsetY > 0) mOffsetY = 0;
	}
	else
	{
		mMinOffsetY = h - mViewRect.height();
		if(mOffsetY > mMinOffsetY) mOffsetY = mMinOffsetY;
		if(mOffsetY < 0) mOffsetY = 0;
	}
	
	// 重新绘制
	update();

}

void GGraphView::mouseMoveEvent(QMouseEvent * event)
{
	if (mIsDrag)
	{
		int dx = event->pos().x() - mMousePos.x();
		int dy = event->pos().y() - mMousePos.y();
		mMousePos = event->pos();
		mOffsetX += dx;
		mOffsetY += dy;

		if (mIsFlipX) 
		{
			if(mOffsetX > mMinOffsetX) mOffsetX = mMinOffsetX;
			if(mOffsetX < 0) mOffsetX = 0;
		}
		else 
		{
			if (mOffsetX < mMinOffsetX) mOffsetX = mMinOffsetX;
			if (mOffsetX > 0) mOffsetX = 0;
		}

		if (mIsFlipY)
		{
			if (mOffsetY < mMinOffsetY) mOffsetY = mMinOffsetY;
			if (mOffsetY > 0) mOffsetY = 0;
		}
		else
		{
			if(mOffsetY > mMinOffsetY) mOffsetY = mMinOffsetY;
			if(mOffsetY < 0) mOffsetY = 0;
		}

		update();
	}

	if (mInfoLabel)
	{
		double lx = dpXTolpX(event->x());
		double ly = dpYTolpY(event->y());
		QString str = QString("%1: %2, %3: %4").arg(mXTitle).arg(lx).arg(mYTitle).arg(ly);
		mInfoLabel->setText(str);
	}
}

void GGraphView::mousePressEvent(QMouseEvent * event)
{
	if (event->button() == Qt::RightButton)
	{
		mIsDrag = true;
		mMousePos = event->pos();
	}
}

void GGraphView::mouseReleaseEvent(QMouseEvent * event)
{
	if (event->button() == Qt::RightButton)
	{
		mIsDrag = false;
	}
}

void GGraphView::drawBackground(QPainter *p, const QRect &r)
{
	p->fillRect(r, mBackgroundColor);
}

void GGraphView::drawTitle(QPainter *p)
{
	QFontMetrics fm = p->fontMetrics();

	// 绘制主标题
	p->setPen(Qt::black);
	p->setFont(mMainTitleFont);
	p->drawText(mViewRect.left() + mViewRect.width()/2 - fm.width(mTitle)/2,
		mViewRect.top() - fm.height() - mTitleMargin, mTitle);

	// 绘制XY轴标题
	p->setFont(mXYTitleFont);
	if (mHRulerPos == BOTTOM)
	{
		if (mIsFlipX) p->drawText(mViewRect.left()+8, mViewRect.bottom() + fm.height(), mXTitle);
		else p->drawText(mViewRect.right()-8-fm.width(mXTitle), mViewRect.bottom() + fm.height(), mXTitle);
		//p->drawText(mViewRect.left() - fm.width(mYTitle), mViewRect.top() - fm.height()/4, mYTitle);
	}
	else
	{
		if (mIsFlipX) p->drawText(mViewRect.left()+8, mViewRect.top()-fm.height()-5, mXTitle);
		else p->drawText(mViewRect.right()-8-fm.width(mXTitle), mViewRect.top()-fm.height()-5, mXTitle);
		//p->drawText(mViewRect.right()+4, mViewRect.top()+5, mXTitle);
		//p->drawText(mViewRect.left() - fm.width(mYTitle), mViewRect.bottom() + fm.height()/2, mYTitle);
	}
	if (mVRulerPos == LEFT)
	{
		if (mIsFlipY)
		{
			p->save();
			p->translate(mViewRect.left(), mViewRect.bottom());
			p->rotate(90);
			p->drawText(-8-fm.width(mYTitle), mViewRect.left()-8, mYTitle);
			p->restore();
		}
		else
		{
			p->save();
			p->translate(mViewRect.left(), mViewRect.top());
			p->rotate(90);
			p->drawText(8, mViewRect.left()-8, mYTitle);
			p->restore();
		}
	}
	else
	{
		if (mIsFlipY)
		{
			p->save();
			p->translate(mViewRect.right(), mViewRect.bottom());
			p->rotate(90);
			p->drawText(-8-fm.width(mYTitle), -mMargins.right()+fm.height()+5, mYTitle);
			p->restore();
		}
		else
		{
			p->save();
			p->translate(mViewRect.right(), mViewRect.top());
			p->rotate(90);
			p->drawText(8, -mMargins.right()+fm.height()+5, mYTitle);
			p->restore();
		}
	}
}

void GGraphView::drawHRuler(QPainter *p)
{
	int x, left, right, dx, y;
	double lx, ldx;

	if (mMaxLpX <= mMinLpX) return;

	p->setFont(mXYTitleFont);
	QFontMetrics fm = p->fontMetrics();
	p->setPen(Qt::black);

	if (mHRulerPos == BOTTOM) y = mViewRect.bottom();
	else y = mViewRect.top();
	//p->drawLine(mViewRect.left(), y, mViewRect.right(), y);

	left = mViewRect.left();
	right = mViewRect.right();

	// 只显示固定间隔的坐标
	if (mRulerDeltaX != 0)
	{
		ldx = mRulerDeltaX;
	}
	else
	{
		ldx = (dpXTolpX(right) - dpXTolpX(left)) / mRulerXCount;
		int f = ldx/ldx;
		ldx = f*gAdaptStep(abs(ldx));
	}

	lx = dpXTolpX(left);
	if (mIsFlipX)
	{
		lx = floor(lx / ldx) * ldx;
		ldx = -ldx;
	}
	else
	{
		lx = ceil(lx / ldx) * ldx;
	}
	x = lpXTodpX(lx);
	while (x < left)
	{
		lx += ldx;
		x = lpXTodpX(lx);
	}
	double subLdx = ldx * 0.2;

	while (x <= right+1)
	{
		if(mIsDrawGrid && x <= right)
		{
			p->setPen(Qt::gray);
			p->drawLine(x, mViewRect.top(), x, mViewRect.bottom());
		}

		p->setPen(Qt::black);
		if (fabs(lx) < fabs(ldx)*0.1) lx = 0;
		QString str = QString::number(lx);
		int tw = fm.width(str);

		if (mHRulerPos == BOTTOM)
		{
			p->drawText(x-tw/2, y+17, str);
			p->drawLine(x, y+5, x, y);
		}
		else
		{
			p->drawText(x-tw/2, y-fm.height()/2, str);
			p->drawLine(x, y-5, x, y);
		}
			

		lx += ldx;
		x = lpXTodpX(lx);
	}

	// 绘制小刻度
	lx = dpXTolpX(left);
	if (mIsFlipX)
	{
		lx = floor(lx / subLdx) * subLdx;
	}
	else
	{
		lx = ceil(lx / subLdx) * subLdx;
	}
	x = lpXTodpX(lx);
	while (x < left)
	{
		lx += subLdx;
		x = lpXTodpX(lx);
	}

	while (x <= right+1)
	{
		p->setPen(Qt::black);
		if (mHRulerPos == BOTTOM)
		{
			p->drawLine(x, y+2, x, y);
		}
		else
		{
			p->drawLine(x, y-2, x, y);
		}


		lx += subLdx;
		x = lpXTodpX(lx);
	}
}

void GGraphView::drawVRuler(QPainter *p)
{
	int x, top, bottom, y, dy;
	double ly, ldy;

	if (mMaxLpY <= mMinLpY) return;

	p->setFont(mXYTitleFont);

	QFontMetrics fm = p->fontMetrics();
	p->setPen(Qt::black);

	if (mVRulerPos == LEFT) x = mViewRect.left();
	else x = mViewRect.right();
	//p->drawLine(x, mViewRect.top(), x, mViewRect.bottom());

	top = mViewRect.top();
	bottom = mViewRect.bottom();

	// 计算逻辑间隔
	if (mRulerDeltaY != 0)
	{
		ldy = mRulerDeltaY;
	}
	else
	{
		ldy = (dpYTolpY(top) - dpYTolpY(bottom)) / mRulerYCount;
		int f = ldy/ldy;
		ldy = f*gAdaptStep(abs(ldy));
	}
	// 绘制主刻度, 并标注
	ly = dpYTolpY(bottom);

	if (mIsFlipY)
	{
		ly = ceil(ly / ldy) * ldy;
		ldy = -ldy;
	}
	else
	{
		ly = floor(ly / ldy) * ldy;
	}
	y = lpYTodpY(ly);

	while (y > bottom+1)
	{
		ly += ldy;
		y = lpYTodpY(ly);
	}
	double subLdy = ldy * 0.2;

	int th = fm.height();
	while (y >= top)
	{
		if (mIsDrawGrid)
		{
			p->setPen(Qt::gray);
			p->drawLine(mViewRect.left(), y, mViewRect.right(), y);
		}

		p->setPen(Qt::black);
		if (fabs(ly) < fabs(ldy)*0.1) ly = 0;
		QString str = QString::number(ly);
		int tw = fm.width(str);
		if (mVRulerPos == LEFT)
		{
			p->drawText(x-tw-10, y-th/2, tw, th, Qt::AlignCenter, str);
			p->drawLine(x, y, x-5, y);
		}
		else
		{
			p->drawText(x+10, y-th/2, tw, th, Qt::AlignCenter, str);
			p->drawLine(x, y, x+5, y);
		}

		ly += ldy;
		y = lpYTodpY(ly);
	}

	// 绘制小刻度
	ly = dpYTolpY(bottom);

	if (mIsFlipY)
	{
		ly = ceil(ly / subLdy) * subLdy;
	}
	else
	{
		ly = floor(ly / subLdy) * subLdy;
	}
	y = lpYTodpY(ly);

	while (y > bottom+1)
	{
		ly += subLdy;
		y = lpYTodpY(ly);
	}
	while (y >= top)
	{
		p->setPen(Qt::black);
		if (mVRulerPos == LEFT)
		{
			p->drawLine(x, y, x-2, y);
		}
		else
		{
			p->drawLine(x, y, x+2, y);
		}

		ly += subLdy;
		y = lpYTodpY(ly);
	}
}

void GGraphView::statDataRange()
{
	/*mMinLpX = mMinLpY = HUGE_VAL;
	mMaxLpX = mMaxLpY = -HUGE_VAL;
	
	int n = 0;
	for (int i=0; i<mLines.count(); i++)
	{
		QPolygonF line = mLines.at(i);

		for(int j=0; j<line.count(); j++)
		{
			double x = line.at(j).x();
			double y = line.at(j).y();
			if (mMinLpX > x) mMinLpX = x;
			if (mMaxLpX < x) mMaxLpX = x;
			if (mMinLpY > y) mMinLpY = y;
			if (mMaxLpY < y) mMaxLpY = y;
			n++;
		}
	}

	if (n <= 0)
	{
		mMinLpX = mMinLpY = 0;
		mMaxLpX = mMaxLpY = 1;
	}

	if(mMaxLpX == mMinLpX) 
	{
		mMaxLpX = mMinLpX + 1;
	}
	if (mMaxLpY == mMinLpY)
	{
		mMaxLpY = mMinLpY + 1;
	}


	// 计算偏移后的最值
	mMaxLpY = mMaxLpY + mExtendRange * (mMaxLpY - mMinLpY);
	mMinLpY = mMinLpY - mExtendRange * (mMaxLpY - mMinLpY);*/
}

int GGraphView::lpXTodpX(double lx)
{
	if (mIsFlipX)
	{
		return mViewRect.right() - qRound((lx - mMinLpX) * mViewRect.width() * mZoomScale /
										(mMaxLpX - mMinLpX)) + mOffsetX;
	}
	else
	{
		return mViewRect.left() + qRound((lx - mMinLpX) * mViewRect.width() * mZoomScale /
										(mMaxLpX - mMinLpX)) + mOffsetX;
	}
	
}

double GGraphView::dpXTolpX(int dpX)
{
	if (mIsFlipX)
	{
		return mMinLpX - (dpX - mViewRect.right() - mOffsetX) * (mMaxLpX - mMinLpX) /
			   (mViewRect.width()*mZoomScale);
	}
	else
	{
		return (dpX - mViewRect.left() - mOffsetX) * (mMaxLpX - mMinLpX) /
			   (mViewRect.width()*mZoomScale) + mMinLpX;
	}
	
}

int GGraphView::lpYTodpY(double ly)
{
	if (mIsFlipY)
	{
		return mViewRect.top() + qRound((ly - mMinLpY) * mViewRect.height() * mZoomScale /
									   (mMaxLpY - mMinLpY)) + mOffsetY;
	}
	else
	{
		return mViewRect.bottom() - qRound((ly - mMinLpY)*mViewRect.height() * mZoomScale /
										  (mMaxLpY - mMinLpY)) + mOffsetY;
	}
	
}

double GGraphView::dpYTolpY(int dpY)
{
	if(mIsFlipY)
	{
		return mMinLpY + (dpY - mViewRect.top() - mOffsetY) * (mMaxLpY - mMinLpY) /
						 (mViewRect.height()*mZoomScale);
	}
	else
	{
		return mMinLpY - (dpY - mViewRect.bottom() - mOffsetY)*(mMaxLpY - mMinLpY) /
						 (mViewRect.height()*mZoomScale);
	}
}
