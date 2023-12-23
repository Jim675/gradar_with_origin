#pragma once

#include <QString>
#include <vtkSystemIncludes.h>
#include "gopacitytable.h"

// 动画配置类
class GAnimationConfig
{
public:
    double  mInterval = 1000;           // 间隔时间
    bool    mIsSkipEmpty = true;        // 是否跳过空数据体
};

// 网格化配置
class GGriddingConfig
{
public:
    int mNX = 256;                      // X方向网格化数目
    int mNY = 256;                      // Y方向网格化数目
    int mNZ = 128;                      // Z方向网格化数目
};


// 渲染配置
class GRenderConfig
{
public:
    // 高程缩放系数
    double mZScale = 1.0;

    // 自动调整采样间距
    bool mAutoSampleDistance = false;

    // 采样间距
    double mSampleDistance = 50.0;

    // 使用模糊去除木纹
    bool mUseJittering = true;

    // 不透明度
    //double mOpacity = 0.6;

    // 不透明度表
    GOpacityTable mOpacityTable;

    // 数据体插值方式
    int mInterpolationMethod = VTK_LINEAR_INTERPOLATION;

    // 数据体光照效果
    bool mVolumeShade = true;
    double mVolumeAmbient = 0.6;
    double mVolumeDiffuse = 0.4;
    double mVolumeSpecular = 0.0;
    double mVolumeSpecularPower = 10.0;

    // 地形光照效果
    bool mTerrainShade = false;
    double mTerrainAmbient = 0.6;
    double mTerrainDiffuse = 0.4;
    double mTerrainSpecular = 0.0;
    double mTerrainSpecularPower = 10.0;

    // 颜色条
    double mScalarBarLabelsCount = 8;
    double mScalarBarWidthRate = 0.05;
    double mScalarBarHeightRate = 0.8;
    int mScalarBarMaxWidthPixels = 120;

    // 立方体坐标轴字号
    int mCubeAxesFontSize = 12;

    bool mCubeAxesVisibility = true;

	bool saveToIni(const QString& fileName) const;

	bool loadFromIni(const QString& fileName);
};

// 配置管理类
class GConfig
{
public:
    // 动画配置
    static GAnimationConfig mAnimationConfig;

    // 网格化配置
    static GGriddingConfig mGriddingConfig;

    // 渲染配置
    static GRenderConfig mRenderConfig;

    // 上次选择的路径
    static QString mLastSelectPath;

private:
    GConfig() {}
};
