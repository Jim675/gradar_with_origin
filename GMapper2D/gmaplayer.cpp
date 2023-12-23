#include "gmaplayer.h"
#include "gmapview.h"

//----class GMapLayer--------------------------------------------------------------
IMPL_GREFLECTION_CLASS(GMapLayer, GMapLayer)

GMapLayer::GMapLayer()
{
    mLayerName = QString::null;
    mIsVisible = true;
    mpView = NULL;
}

GMapLayer::~GMapLayer()
{

}

void GMapLayer::draw(QPainter* p, const QRect& /*cr*/)
{

}

QString GMapLayer::maybeTip(double lx, double ly)
{
    return QString::null;
}

bool GMapLayer::save(QDataStream& stream)
{
    stream << mLayerName;
    stream << mBoundRect;
    stream << mIsVisible;

    return true;
}

bool GMapLayer::load(QDataStream& stream, bool& isReplace)
{
    isReplace = false;
    stream >> mLayerName;
    stream >> mBoundRect;
    stream >> mIsVisible;

    return true;
}