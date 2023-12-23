#include <QSettings>
#include "gconfig.h"

// 初始化全局变量
GAnimationConfig GConfig::mAnimationConfig;

GGriddingConfig GConfig::mGriddingConfig;

GRenderConfig GConfig::mRenderConfig;

QString GConfig::mLastSelectPath;

bool GRenderConfig::saveToIni(const QString& fileName) const
{
	QSettings settings(fileName, QSettings::IniFormat);
	settings.beginGroup("RENDER_CONFIG");

	if (settings.status() != QSettings::NoError) return false;

	settings.setValue("Z_SCALE", mZScale);
	settings.setValue("AUTO_SAMPLE_DISTANCE", mAutoSampleDistance);
	settings.setValue("SAMPLE_DISTANCE", mSampleDistance);
	settings.setValue("USE_JITTERING", mUseJittering);
	settings.setValue("INTERPOLATION_METHOD", mInterpolationMethod);
	settings.setValue("VOLUME_SHADE", mVolumeShade);
	settings.setValue("VOLUME_AMBIENT", mVolumeAmbient);
	settings.setValue("VOLUME_DIFFUSE", mVolumeDiffuse);
	settings.setValue("VOLUME_SPECULAR", mVolumeSpecular);
	settings.setValue("VOLUME_SPECULAR_POWER", mVolumeSpecularPower);

	settings.setValue("TERRAIN_SHADE", mTerrainShade);
	settings.setValue("TERRAIN_AMBIENT", mTerrainAmbient);
	settings.setValue("TERRAIN_DIFFUSE", mTerrainDiffuse);
	settings.setValue("TERRAIN_SPECULAR", mTerrainSpecular);
	settings.setValue("TERRAIN_SPECULAR_POWER", mTerrainSpecularPower);

	settings.setValue("SCALAR_BAR_LABELS_COUNT", mScalarBarLabelsCount);
	settings.setValue("SCALAR_BAR_WIDTH_RATE", mScalarBarWidthRate);
	settings.setValue("SCALAR_BAR_HEIGHT_RATE", mScalarBarHeightRate);
	settings.setValue("SCALAR_BAR_MAX_WIDTH_PIXELS", mScalarBarMaxWidthPixels);
	settings.setValue("CUBE_AXES_FONT_SIZE", mCubeAxesFontSize);
	settings.setValue("CUBE_AXES_VISIBILITY", mCubeAxesVisibility);

	settings.endGroup();
	return true;
}

bool GRenderConfig::loadFromIni(const QString& fileName)
{
	QSettings settings(fileName, QSettings::IniFormat);
	settings.beginGroup("RENDER_CONFIG");

	if (settings.status() != QSettings::NoError) return false;

	mZScale = settings.value("DEEP_SCALE", "1.0").toDouble();
	mAutoSampleDistance = settings.value("AUTO_SAMPLE_DISTANCE", "false").toBool();
	mSampleDistance = settings.value("SAMPLE_DISTANCE", "50").toDouble();
	mAutoSampleDistance = settings.value("USE_JITTERING", "false").toBool();
	mInterpolationMethod = settings.value("INTERPOLATION_METHOD", "1").toInt();

	mVolumeShade = settings.value("VOLUME_SHADE", "true").toBool();
	mVolumeAmbient = settings.value("VOLUME_AMBIENT", "0.6").toDouble();
	mVolumeDiffuse = settings.value("VOLUME_DIFFUSE", "0.4").toDouble();
	mVolumeSpecular = settings.value("VOLUME_SPECULAR", "0.0").toDouble();
	mVolumeSpecularPower = settings.value("VOLUME_SPECULAR_POWER", "10.0").toDouble();

	mVolumeShade = settings.value("TERRAIN_SHADE", "true").toBool();
	mVolumeAmbient = settings.value("TERRAIN_AMBIENT", "0.6").toDouble();
	mVolumeDiffuse = settings.value("TERRAIN_DIFFUSE", "0.4").toDouble();
	mVolumeSpecular = settings.value("TERRAIN_SPECULAR", "0.0").toDouble();
	mVolumeSpecularPower = settings.value("TERRAIN_SPECULAR_POWER", "10.0").toDouble();

	mScalarBarLabelsCount = settings.value("SCALAR_BAR_LABELS_COUNT", "8").toInt();
	mScalarBarWidthRate = settings.value("SCALAR_BAR_WIDTH_RATE", "0.08").toDouble();
	mScalarBarHeightRate = settings.value("SCALAR_BAR_HEIGHT_RATE", "0.8").toDouble();
	mScalarBarMaxWidthPixels = settings.value("SCALAR_BAR_MAX_WIDTH_PIXELS", "100").toInt();
	mCubeAxesFontSize = settings.value("CUBE_AXES_FONT_SIZE", "12").toInt();
	mCubeAxesVisibility = settings.value("CUBE_AXES_VISIBILITY", true).toBool();

	settings.endGroup();

	return true;
}
