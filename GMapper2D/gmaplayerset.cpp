#include "gmaplayerset.h"
#include "gutils.h"
#include "gmaplayer.h"
#include "gmapview.h"

GMapLayerSet::GMapLayerSet()
{
    mpView = nullptr;
}

GMapLayerSet::~GMapLayerSet()
{
    clear();
}

void GMapLayerSet::clear()
{
    qDeleteAll(mLayerList);
    mLayerList.clear();
}

GMapLayer* GMapLayerSet::get(int idx)
{
    if (idx < 0 || idx >= count()) return nullptr;

    return mLayerList[idx];
}

int GMapLayerSet::getLayerIdxByName(const QString& layerName)
{
    int i;
    for (i = 0; i < count(); i++) {
        GMapLayer* pLayer = mLayerList[i];
        if (pLayer->getLayerName() == layerName) {
            return i;
        }
    }

    return -1;
}

GMapLayer* GMapLayerSet::getLayerByName(const QString& layerName)
{
    return get(getLayerIdxByName(layerName));
}

void GMapLayerSet::addLayer(GMapLayer* pLayer)
{
    pLayer->attachView(mpView);
    mLayerList.prepend(pLayer);
}

void GMapLayerSet::removeLayer(int index)
{
    mLayerList.remove(index);
}

void GMapLayerSet::moveLayer(int ifrom, int ito)
{
    GMapLayer* pLayer;
    pLayer = mLayerList[ifrom];
    if (ifrom > ito) {
        mLayerList.insert(ito, pLayer);
        mLayerList.remove(ifrom + 1);
    } else if (ifrom < ito) {
        mLayerList.insert(ito + 1, pLayer);
        mLayerList.remove(ifrom);
    }
}

void GMapLayerSet::draw(QPainter* p, const QRect& cr)
{
    int i;
    for (i = mLayerList.count() - 1; i >= 0; i--) {
        GMapLayer* pLayer = mLayerList[i];
        if (pLayer->isVisible()) {
            pLayer->draw(p, cr);
        }
    }
}

void GMapLayerSet::attachView(GMapView* pView)
{
    int i;

    mpView = pView;
    for (i = 0; i < mLayerList.count(); i++) {
        mLayerList[i]->attachView(mpView);
    }
}

bool GMapLayerSet::save(QDataStream& stream)
{
    qint32 i;
    stream << (qint32)mLayerList.count();
    for (i = 0; i < count(); i++) {
        GMapLayer* pLayer = mLayerList[i];
        stream << pLayer->className();
        if (!pLayer->save(stream)) {
            return false;
        }
    }

    return true;
}

bool GMapLayerSet::load(QDataStream& stream, bool& isModify)
{
    bool isRep;
    qint32 i, n;
    QString className;

    clear();
    stream >> n;
    isModify = false;
    for (i = 0; i < n; i++) {
        stream >> className;
        GMapLayer* pLayer = GMapLayer::instance(className);
        if (pLayer == NULL || !pLayer->load(stream, isRep)) {
            if (!pLayer) delete pLayer;
            isModify = true;
            continue;
        }

        if (isRep) isModify = true;
        mLayerList.append(pLayer);
    }

    return true;
}