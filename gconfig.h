#pragma once

#include <QString>
#include <vtkSystemIncludes.h>
#include "gopacitytable.h"

// ����������
class GAnimationConfig
{
public:
    double  mInterval = 1000;           // ���ʱ��
    bool    mIsSkipEmpty = true;        // �Ƿ�������������
};

// ��������
class GGriddingConfig
{
public:
    int mNX = 256;                      // X����������Ŀ
    int mNY = 256;                      // Y����������Ŀ
    int mNZ = 128;                      // Z����������Ŀ
};


// ��Ⱦ����
class GRenderConfig
{
public:
    // �߳�����ϵ��
    double mZScale = 1.0;

    // �Զ������������
    bool mAutoSampleDistance = false;

    // �������
    double mSampleDistance = 50.0;

    // ʹ��ģ��ȥ��ľ��
    bool mUseJittering = true;

    // ��͸����
    //double mOpacity = 0.6;

    // ��͸���ȱ�
    GOpacityTable mOpacityTable;

    // �������ֵ��ʽ
    int mInterpolationMethod = VTK_LINEAR_INTERPOLATION;

    // ���������Ч��
    bool mVolumeShade = true;
    double mVolumeAmbient = 0.6;
    double mVolumeDiffuse = 0.4;
    double mVolumeSpecular = 0.0;
    double mVolumeSpecularPower = 10.0;

    // ���ι���Ч��
    bool mTerrainShade = false;
    double mTerrainAmbient = 0.6;
    double mTerrainDiffuse = 0.4;
    double mTerrainSpecular = 0.0;
    double mTerrainSpecularPower = 10.0;

    // ��ɫ��
    double mScalarBarLabelsCount = 8;
    double mScalarBarWidthRate = 0.05;
    double mScalarBarHeightRate = 0.8;
    int mScalarBarMaxWidthPixels = 120;

    // �������������ֺ�
    int mCubeAxesFontSize = 12;

    bool mCubeAxesVisibility = true;

	bool saveToIni(const QString& fileName) const;

	bool loadFromIni(const QString& fileName);
};

// ���ù�����
class GConfig
{
public:
    // ��������
    static GAnimationConfig mAnimationConfig;

    // ��������
    static GGriddingConfig mGriddingConfig;

    // ��Ⱦ����
    static GRenderConfig mRenderConfig;

    // �ϴ�ѡ���·��
    static QString mLastSelectPath;

private:
    GConfig() {}
};
