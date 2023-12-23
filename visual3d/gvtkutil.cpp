#include <QFile>
#include <QXmlStreamReader>
#include <QColor>
#include "gvtkutil.h"

bool VTKUtil::saveImageDataToMeta(vtkImageData* data, const char* path)
{
	vtkSmartPointer<vtkMetaImageWriter> writer = vtkSmartPointer<vtkMetaImageWriter>::New();
	writer->SetInputData(data);
	writer->SetFileName(path);
	writer->Write();
	//writer->Update();

	int code = writer->GetErrorCode();
	return code == vtkErrorCode::NoError;
}

bool VTKUtil::saveImageDataToJPG(vtkImageData* data, const char* path)
{
	vtkNew<vtkJPEGWriter> writer;
	writer->SetInputData(data);
	writer->SetFileName(path);
	writer->SetQuality(100);
	writer->Write();
	//writer->Update();

	int code = writer->GetErrorCode();
	return code == vtkErrorCode::NoError;
}

bool VTKUtil::saveImageDataToPNG(vtkImageData* data, const char* path)
{
	vtkNew<vtkPNGWriter> writer;
	writer->SetInputData(data);
	writer->SetFileName(path);
	writer->SetCompressionLevel(0);
	writer->Write();
	//writer->Update();

	int code = writer->GetErrorCode();
	return code == vtkErrorCode::NoError;
}

vtkSmartPointer<vtkImageData> VTKUtil::readImageData(const char* path)
{
	vtkSmartPointer<vtkMetaImageReader> reader = vtkSmartPointer<vtkMetaImageReader>::New();
	reader->SetFileName(path);
	reader->Update();

	vtkSmartPointer<vtkImageData> data;
	int code = reader->GetErrorCode();
	if (code == vtkErrorCode::NoError) {
		data = reader->GetOutput();
	}
	return data;
}

vtkSmartPointer<vtkImageData> VTKUtil::toVtkImageData(const QImage* image)
{
	const int width = image->width();
	const int height = image->height();

	QImage::Format format = image->format();
	if (format == QImage::Format::Format_RGB888) {
		// 内存排布 R G B R G B 
	}
	else {
		//throw new std::runtime_error("不支持该图像格式");
		return vtkSmartPointer<vtkImageData>(nullptr);
	}

	vtkSmartPointer<vtkImageData> pointer = vtkSmartPointer<vtkImageData>::New();
	pointer->SetDimensions(width, height, 1);
	pointer->SetOrigin(-0.5 * width, -0.5 * height, 1);

	vtkNew<vtkUnsignedCharArray> colorArray;
	colorArray->SetNumberOfComponents(3);
	colorArray->SetNumberOfTuples(static_cast<vtkIdType>(width) * height);
	int i = 0;
	// vtkImageData行顺序相反
	for (int row = height - 1; row >= 0; row--) {
		const unsigned char* line = image->scanLine(row);
		for (int col = 0; col < width; col++) {
			// 内存排布 R G B R G B 
			const unsigned char* rgb = &line[col * 3];
			colorArray->SetValue(i++, rgb[0]); // red
			colorArray->SetValue(i++, rgb[1]); // green
			colorArray->SetValue(i++, rgb[2]); // blue
			//cout << "rgb:" << (int)rgb[0] << ',' << (int)rgb[1] << ',' << (int)rgb[2] << endl;
		}
	}
	pointer->GetPointData()->SetScalars(colorArray);
	return pointer;
}

vtkSmartPointer<vtkImageData> VTKUtil::readTIFF(const char* path)
{
	vtkNew<vtkTIFFReader> reader;
	reader->SetFileName(path);
	reader->Update();
	vtkImageData* imageData = reader->GetOutput();
	vtkSmartPointer<vtkImageData> pointer(imageData);
	return pointer;
}

vtkSmartPointer<vtkImageData> VTKUtil::readJPG(const char* path)
{
	vtkNew<vtkJPEGReader> reader;
	reader->SetFileName(path);
	reader->Update();
	vtkImageData* imageData = reader->GetOutput();
	vtkSmartPointer<vtkImageData> pointer(imageData);
	return pointer;
}

// 从文件中读取色标
bool VTKUtil::readColorMap(const QString& path, const QString& tag, vtkColorTransferFunction* colorTF)
{
	QFile file(path);
	if (!file.open(QIODevice::OpenModeFlag::ReadOnly | QIODevice::OpenModeFlag::Text))
	{
		return false;
	}
	QXmlStreamReader reader(&file);

	colorTF->RemoveAllPoints();

	bool into = false;
	bool hasError = false;

	while (!reader.atEnd())
	{
		if (reader.hasError())
		{
			hasError = true;
			break;
		}
		QXmlStreamReader::TokenType token = reader.readNext();
		// 当前节点标签名
		QStringRef curTag = reader.name();
		//qDebug() << "curTag:" << curTag;
		if (curTag == tag)
		{
			if (token == QXmlStreamReader::TokenType::StartElement)
			{
				into = true;
				colorTF->SetClamping(false);
			}
			else if (token == QXmlStreamReader::TokenType::EndElement) {
				colorTF->Build();
				break;
			}
		}
		else if (into && curTag == "color")
		{
			const QStringRef level = reader.attributes().value("level");
			const QStringRef desp = reader.attributes().value("desp");
			token = reader.readNext();
			if (token == QXmlStreamReader::TokenType::Characters)
			{
				const QStringRef text = reader.text();
				//qDebug() << "level:" << level
				//    << ", desp:" << desp
				//    << ", text:" << text;
				bool ok = false;
				double x = level.toDouble(&ok);
				QColor color(text);
				if (!(ok && color.isValid())) {
					// 如果数值或者颜色解析失败
					hasError = true;
					break;
				}
				//qDebug() << "redF:" << color.redF()
				//    << ", greenF:" << color.greenF()
				//    << ", blueF:" << color.blueF();
				colorTF->AddRGBPoint(x, color.redF(), color.greenF(), color.blueF(), 1.0, 1.0);
			}
			else
			{
				hasError = true;
				break;
			}
			token = reader.readNext();
			//curTag = reader.name();
			if (token != QXmlStreamReader::TokenType::EndElement)
			{
				hasError = true;
				break;
			}
		}
	}

	file.close();
	if (hasError)
	{
		colorTF->RemoveAllPoints();
		return false;
	}
	else
	{
		return true;
	}
}

inline constexpr double div255(double color)
{
	return color / 255.0;
}
void VTKUtil::setDefaultColorMap(vtkColorTransferFunction* pColorTF)
{
	double keys[] = { -10, 10, 10.4, 20.0, 20.4, 30.0, 30.4, 40.0, 40.4, 50.0, 50.4, 60.0, 60.4, 70.0, 70.4, 80.0 };
	//double keys[] = { -5, 0, 1, 2.5, 5, 7.5, 10, 15, 20, 30, 40, 50, 70, 100, 150, 200, 250, 300, 400, 500, 600, 750 };
	// 构造默认颜色转换函数
	//colorTF = vtkDiscretizableColorTransferFunction::New();
	// midpoint=0.5, sharpness=0.0, 在 -10.0 到下个数据点之间颜色会均匀渐变
	pColorTF->AddRGBPoint(keys[0], div255(64), div255(64), div255(64), 0.5, 0.0);
	pColorTF->AddRGBPoint(keys[1], div255(164), div255(164), div255(255), 0.5, 0.0);
	pColorTF->AddRGBPoint(keys[2], div255(164), div255(164), div255(255), 0.5, 0.0);
	//midpoint = 0.0, sharpness = 1.0, 在 20.0 颜色会突变
	pColorTF->AddRGBPoint(keys[3], div255(100), div255(100), div255(192), 0.0, 1.0);
	pColorTF->AddRGBPoint(keys[4], div255(64), div255(128), div255(255), 0.5, 0.0);
	pColorTF->AddRGBPoint(keys[5], div255(32), div255(64), div255(128), 0.0, 1.0);
	pColorTF->AddRGBPoint(keys[6], div255(0), div255(255), div255(0), 0.5, 0.0);
	pColorTF->AddRGBPoint(keys[7], div255(0), div255(128), div255(0), 0.0, 1.0);
	pColorTF->AddRGBPoint(keys[8], div255(255), div255(255), div255(0), 0.5, 0.0);
	pColorTF->AddRGBPoint(keys[9], div255(255), div255(128), div255(0), 0.0, 1.0);
	pColorTF->AddRGBPoint(keys[10], div255(255), div255(0), div255(0), 0.5, 0.0);
	pColorTF->AddRGBPoint(keys[11], div255(160), div255(0), div255(0), 0.0, 1.0);
	pColorTF->AddRGBPoint(keys[12], div255(255), div255(0), div255(255), 0.5, 0.0);
	pColorTF->AddRGBPoint(keys[13], div255(128), div255(0), div255(128), 0.0, 1.0);
	pColorTF->AddRGBPoint(keys[14], div255(255), div255(255), div255(255), 0.5, 0.0);
	pColorTF->AddRGBPoint(keys[15], div255(128), div255(128), div255(128), 0.0, 1.0);
	//colorTF->SetDiscretize(true);
	pColorTF->Build();
}
