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
    // 瓦片状态
    enum State
    {
        gUnLoad = 0x01, gLoading = 0x02, gLoaded = 0x04, gVisible = 0x08
    };

    int			mGrade;		// 瓦片等级
    int			mRow;		// 瓦片行号
    int			mCol;		// 瓦片列号
    quint32		mState;		// 瓦片图像加载状态
    QImage		mImage;		// 瓦片图像

    GMapTile(int grade = 1, int row = 0, int col = 0);
    ~GMapTile();

    bool operator == (const GMapTile& other);
    bool operator != (const GMapTile& other);
};

// 地图服务器测试结构体
struct GMapServerTest;

// 瓦片下载器
class GMAPPER2D_EXPORT GMapTileLoader: public QObject
{
    Q_OBJECT

private:
    bool								mIsSaveToCache;		// 是否将下载到的图像缓存到本地
    QNetworkAccessManager				mNetManager;		// 网络访问管理器
    QMap<QNetworkReply*, GMapTile*>	mReplyMap;
    const char* mDownloadFormat;
    int									mMaxAccessCount;	// 最大Http连接次数
    QVector<GMapTile*>					mHttpTiles;			// 正在使用Http下载的Tile列表
    QVector<GMapTile*>					mWaitTiles;			// 正在等待下载的Tile列表
    int									mHttpTimerId;		// 刷新Timer ID
    QString								mMapTilesPath;		// 地图瓦片存盘路径
    QString								mElevTilesPath;		// 高程瓦片存盘路径
    bool								mIsCreateSubDir;	// 是否自动创建瓦片目录
    static int							mServerId;			// 服务器索引
    static QMap<QNetworkReply*, GMapServerTest*>	mTestReplyMap;		// 服务器测试映射表
    int									mMapType;			// 地图类型: {0: Google, 1: Tencent}

protected:
    // 载入本地最接近的瓦片
    bool loadLocalNearest(GMapTile* pTile);

    // 载入本地瓦片
    bool loadFromLocal(GMapTile* pTile);

    // 通过Http请求瓦片数据
    void httpRequest(GMapTile* pTile, const char* fmt);

    // 在瓦片列表中查找指定参数的瓦片
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

    // 测试并选择响应速度最快的服务器
    static bool testServer(int maxTime);

    // 设置下载服务器编号
    static void setServerId(int id);

    // 获取下载服务器编号
    static int getServerId();

    // 获取瓦片文件存储路径
    const QString& savePath() const;

    // 设置瓦片文件存储路径
    void setMapTilesPath(const QString& mapTilesPath);

    // 设置本地高程瓦片路径
    void setElevTilesPath(const QString& elevTilesPath);

    // 获取是否自动创建文件子目录
    bool isCreateSubPath() const;

    // 设置是否自动创建文件子目录
    void setCreateSubPath(bool isCreate);

    // 批量下载瓦片数据
    void load(const QVector<GMapTile*>& tiles, const char* fmt = GOOGLE_SR_FMT);

    // 不利用缓存数据直接从网络下载瓦片数据
    void loadNoCache(QVector<GMapTile*>& tiles, const char* fmt = GOOGLE_SR_FMT);

    // 终止瓦片数据下载(针对网络http下载)
    void cancelLoad();

    // 加载本地高程瓦片
    bool loadElevTileFromLocal(GMapTile* pTile);

signals:
    // 当瓦片下载完成后发送此信号
    void httpTileFinished(GMapTile* pTile);

    // 当本地瓦片下载完成后发送此信号
    void localTileFinished(GMapTile* pTile);

protected slots:

    // http测试完成
    void httpTestFinished();

    // http请求完成
    void httpFinished();

    void httpReadyRead();

    //void timerEvent(QTimerEvent *event);
};

#endif