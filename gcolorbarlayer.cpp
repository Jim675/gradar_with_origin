#include <QFontMetrics>
#include "gcolorbarlayer.h"

GColorBarLayer::GColorBarLayer() : mFont("Arial", 14, QFont::Weight::ExtraLight)
{
    QFontMetrics metrics(mFont);
    mFontWidth = metrics.width("80.0") + 8;
    mFontHeight = metrics.boundingRect('0').height();
}

GColorBarLayer::~GColorBarLayer()
{

}

// 设置标签数量
void GColorBarLayer::setTagNum(int mTagNum)
{
    this->mTagNum = mTagNum;
}

// 设置标签间隔
void GColorBarLayer::setNumberSpace(double numberSpace)
{
    this->mNumberSpace = numberSpace;
}

void GColorBarLayer::setColorTransferFunction(vtkColorTransferFunction* mColorTF)
{
    if (this->mColorTF != mColorTF) {
        this->mColorTF = mColorTF;
        // 更新颜色传输函数后需要把之前的缓存清空
        if (!mPixmap.isNull()) {
            mPixmap = QPixmap();
        }
    }
}

// 绘制色标
void GColorBarLayer::draw(QPainter* painter, const QRect& rect)
{
    if (mColorTF == nullptr) return;
    // 色标高度
    const int barHeight = rect.height() * mHeightFactor;
    const int startX = mLeftMargin;
    const int startY = (rect.height() - barHeight) / 2;

    // 优先使用缓存
    if (mPixmap.height() != (mFontHeight + barHeight)) {
        painter->setFont(mFont);
        mPixmap = QPixmap(mFontWidth + mBarWidth, mFontHeight + barHeight);
        mPixmap.fill(Qt::transparent);
        QPainter pixmapPainter(&mPixmap);

        // 字体高度一半
        const int half = mFontHeight / 2;
        double rgb[3] = {};
        const double* range = mColorTF->GetRange();
        const double length = range[1] - range[0];
        for (int i = 0; i < barHeight; i++) {
            double value = (1 - ((double)i / (barHeight - 1))) * length + range[0];
            // cout << value << endl;
            mColorTF->GetColor(value, rgb);
            pixmapPainter.setPen(QColor(rgb[0] * 255, rgb[1] * 255, rgb[2] * 255));
            pixmapPainter.drawLine(mFontWidth, i + half, mFontWidth + mBarWidth, i + half);
        }
        // QFont font("Times New Roman", 16, QFont::Weight::Normal);
        pixmapPainter.setFont(mFont);
        pixmapPainter.setPen(QColor(0, 0, 0));
        double space = mNumberSpace;
        int tagNum = mTagNum;
        if (space == 0) {
            if (tagNum > 0) {
                space = length / (tagNum - 1);
            } else {
                space = 10.0;
                tagNum = (int)(length / space) + 1;
            }
        } else {
            tagNum = (int)(length / space) + 1;
        }
        double pxSpace = barHeight / (tagNum - 1);
        for (int i = 0; i < tagNum; i++) {
            pixmapPainter.drawText(0, pxSpace * i + mFontHeight, QString::number(range[1] - space * i, 'f', 1));
        }
    }
    painter->drawPixmap(startX, startY, mPixmap);
}