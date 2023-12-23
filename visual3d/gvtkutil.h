#pragma once

#include <QString>
#include <QImage>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkTIFFReader.h>
#include <vtkJPEGReader.h>
#include <vtkJPEGWriter.h>
#include <vtkPNGWriter.h>
#include <vtkUnsignedCharArray.h>
#include <vtkMetaImageWriter.h>
#include <vtkMetaImageReader.h>
#include <vtkErrorCode.h>
#include <vtkColorTransferFunction.h>

// VTK工具函数类
class VTKUtil
{
public:
    // 保存vtkImageData到path
    static bool saveImageDataToMeta(vtkImageData* data, const char* path);
    
    // 保存vtkImageData为jpg
    static bool saveImageDataToJPG(vtkImageData* data, const char* path);

    // 保存vtkImageData为png
    static bool saveImageDataToPNG(vtkImageData* data, const char* path);

    // 从path读取vtkImageData
    static vtkSmartPointer<vtkImageData> readImageData(const char* path);

    // QImage转VtkImageData
    static vtkSmartPointer<vtkImageData> toVtkImageData(const QImage* image);

    // 读取TIFF文件导vtkImageData
    static vtkSmartPointer<vtkImageData> readTIFF(const char* path);

    // 读取JPG文件导vtkImageData
    static vtkSmartPointer<vtkImageData> readJPG(const char* path);

    // 读取颜色表到vtkColorTransferFunction
	static bool readColorMap(const QString& path, const QString& tag, vtkColorTransferFunction* colorTF);

    // 为colorTF设置为默认的颜色表
	static void setDefaultColorMap(vtkColorTransferFunction* colorTF);
};