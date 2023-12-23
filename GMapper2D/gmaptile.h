#ifndef GMAPTILE_H
#define GMAPTILE_H

#include <QImage>
#include <QVector>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "gmapper2d_global.h"

GMAPPER2D_EXPORT extern const char* GOOGLE_S_FMT;
GMAPPER2D_EXPORT extern const char* GOOGLE_SR_FMT;
GMAPPER2D_EXPORT extern const char* GOOGLE_M_FMT;
GMAPPER2D_EXPORT extern const char* TECENT_SR_FMT;

struct GMAPPER2D_EXPORT GMapTile
{
    // ��Ƭ״̬
    enum State
    {
        gUnLoad = 0x01, gLoading = 0x02, gLoaded = 0x04, gVisible = 0x08
    };

    int			mGrade;		// ��Ƭ�ȼ�
    int			mRow;		// ��Ƭ�к�
    int			mCol;		// ��Ƭ�к�
    quint32		mState;		// ��Ƭͼ�����״̬
    QImage		mImage;		// ��Ƭͼ��

    GMapTile(int grade = 1, int row = 0, int col = 0);
    ~GMapTile();

    bool operator == (const GMapTile& other);
    bool operator != (const GMapTile& other);
};

// ��ͼ���������Խṹ��
struct GMapServerTest;

// ��Ƭ������
class GMAPPER2D_EXPORT GMapTileLoader: public QObject
{
    Q_OBJECT

private:
    bool								mIsSaveToCache;		// �Ƿ����ص���ͼ�񻺴浽����
    QNetworkAccessManager				mNetManager;		// ������ʹ�����
    QMap<QNetworkReply*, GMapTile*>	mReplyMap;
    const char* mDownloadFormat;
    int									mMaxAccessCount;	// ���Http���Ӵ���
    QVector<GMapTile*>					mHttpTiles;			// ����ʹ��Http���ص�Tile�б�
    QVector<GMapTile*>					mWaitTiles;			// ���ڵȴ����ص�Tile�б�
    int									mHttpTimerId;		// ˢ��Timer ID
    QString								mMapTilesPath;		// ��ͼ��Ƭ����·��
    QString								mElevTilesPath;		// �߳���Ƭ����·��
    bool								mIsCreateSubDir;	// �Ƿ��Զ�������ƬĿ¼
    static int							mServerId;			// ����������
    static QMap<QNetworkReply*, GMapServerTest*>	mTestReplyMap;		// ����������ӳ���
    int									mMapType;			// ��ͼ����: {0: Google, 1: Tencent}

protected:
    // ���뱾����ӽ�����Ƭ
    bool loadLocalNearest(GMapTile* pTile);

    // ���뱾����Ƭ
    bool loadFromLocal(GMapTile* pTile);

    // ͨ��Http������Ƭ����
    void httpRequest(GMapTile* pTile, const char* fmt);

    // ����Ƭ�б��в���ָ����������Ƭ
    int findTile(const QVector<GMapTile*>& tiles, const GMapTile* pTile);

public:
    GMapTileLoader(QObject* parent = 0);
    ~GMapTileLoader();

    bool isSaveToCahe() const
    {
        return mIsSaveToCache;
    }

    void setSaveToCahe(bool isSave)
    {
        mIsSaveToCache = isSave;
    }

    // ���Բ�ѡ����Ӧ�ٶ����ķ�����
    static bool testServer(int maxTime);

    // �������ط��������
    static void setServerId(int id);

    // ��ȡ���ط��������
    static int getServerId();

    // ��ȡ��Ƭ�ļ��洢·��
    const QString& savePath() const;

    // ������Ƭ�ļ��洢·��
    void setMapTilesPath(const QString& mapTilesPath);

    // ���ñ��ظ߳���Ƭ·��
    void setElevTilesPath(const QString& elevTilesPath);

    // ��ȡ�Ƿ��Զ������ļ���Ŀ¼
    bool isCreateSubPath() const;

    // �����Ƿ��Զ������ļ���Ŀ¼
    void setCreateSubPath(bool isCreate);

    // ����������Ƭ����
    void load(const QVector<GMapTile*>& tiles, const char* fmt = GOOGLE_SR_FMT);

    // �����û�������ֱ�Ӵ�����������Ƭ����
    void loadNoCache(QVector<GMapTile*>& tiles, const char* fmt = GOOGLE_SR_FMT);

    // ��ֹ��Ƭ��������(�������http����)
    void cancelLoad();

    // ���ر��ظ߳���Ƭ
    bool loadElevTileFromLocal(GMapTile* pTile);

signals:
    // ����Ƭ������ɺ��ʹ��ź�
    void httpTileFinished(GMapTile* pTile);

    // ��������Ƭ������ɺ��ʹ��ź�
    void localTileFinished(GMapTile* pTile);

protected slots:

    // http�������
    void httpTestFinished();

    // http�������
    void httpFinished();

    void httpReadyRead();

    //void timerEvent(QTimerEvent *event);
};

#endif