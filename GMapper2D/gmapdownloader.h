#ifndef GMAP2D_MAKER_H
#define GMAP2D_MAKER_H

#include <QObject>
#include <QVector>
#include <QPointF>
#include <QProgressDialog>
#include "gmaptile.h"
#include "gmapper2d_global.h"

class GMAPPER2D_EXPORT GMapDownloader: public QObject
{
    Q_OBJECT

private:
    QString					mDirPath;			// Ŀ¼��
    QRectF					mWorldRect;			// ī�����������귶Χ
    QRectF					mLonLatRect;		// ���ؾ�γ�ȷ�Χ
    QRectF					mLonLatExtendRect;	// ��չ�ĵ�ͼ��γ�ȷ�Χ
    QRectF					mImageLonLatRect;	// ����ͼ��γ�ȷ�Χ
    GMapTileLoader* mpTileLoader;		// ��Ƭ������
    double					mTileScale;			// ��Ƭ���ű���
    QVector<GMapTile*>		mImageTiles;		// ��Ƭͼ���б�
    int						mDownloadCount;		// ��Ƭ������Ŀ
    QProgressDialog* mProgressDlg;		// ���ȶԻ���
    bool					mIsDownloadCancled;	// �Ƿ�ȡ������

public:
    GMapDownloader(QObject* parent = 0);
    ~GMapDownloader();

    // ������ͼ����
    bool download(const QRectF& lonLatRect, int fromGrade, int toGrade, const char* fmt = GOOGLE_SR_FMT);

    // ��ȡ������Ƭ����
    int tileCount() const;

private:

    // ���г�ʼ��׼��
    bool prepare(const QRectF& lonLatRect, int scaleGrade);

protected slots:
    // ��Ӧ��Ƭ��������¼�, ƴ�ӵ�ͼ
    void onTileFinished(GMapTile* pTile);
};


#endif
