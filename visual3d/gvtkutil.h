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

// VTK���ߺ�����
class VTKUtil
{
public:
    // ����vtkImageData��path
    static bool saveImageDataToMeta(vtkImageData* data, const char* path);
    
    // ����vtkImageDataΪjpg
    static bool saveImageDataToJPG(vtkImageData* data, const char* path);

    // ����vtkImageDataΪpng
    static bool saveImageDataToPNG(vtkImageData* data, const char* path);

    // ��path��ȡvtkImageData
    static vtkSmartPointer<vtkImageData> readImageData(const char* path);

    // QImageתVtkImageData
    static vtkSmartPointer<vtkImageData> toVtkImageData(const QImage* image);

    // ��ȡTIFF�ļ���vtkImageData
    static vtkSmartPointer<vtkImageData> readTIFF(const char* path);

    // ��ȡJPG�ļ���vtkImageData
    static vtkSmartPointer<vtkImageData> readJPG(const char* path);

    // ��ȡ��ɫ��vtkColorTransferFunction
	static bool readColorMap(const QString& path, const QString& tag, vtkColorTransferFunction* colorTF);

    // ΪcolorTF����ΪĬ�ϵ���ɫ��
	static void setDefaultColorMap(vtkColorTransferFunction* colorTF);
};