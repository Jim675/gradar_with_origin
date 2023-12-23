#include <vtkAutoInit.h>

//VTK_MODULE_INIT(vtkInteractionStyle);
//VTK_MODULE_INIT(vtkRenderingOpenGL2); // VTK was built with vtkRenderingOpenGL2
VTK_MODULE_INIT(vtkRenderingVolumeOpenGL2);
//VTK_MODULE_INIT(vtkRenderingFreeType);


#include "tradarvisualwnd.h"

#include <QApplication>
#include <QTextCodec>
#include <QVTKRenderWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkOutputWindow.h>
#include <qsurfaceformat.h>
#include <QSplashScreen>
#include <QPixmap>


// 优先使用NVIDIA独显运行
// extern "C" __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;

int main(int argc, char* argv[])
{
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());
    vtkOutputWindow::SetGlobalWarningDisplay(0);

    QApplication a(argc, argv);

	QPixmap pixmap(":/images/Resources/splash.png");
	QSplashScreen splash(pixmap);
	splash.show();

	QTime splashTime;
	splashTime.start();

    //QTextCodec *codec = QTextCodec::codecForName("System");
    //QTextCodec::setCodecForLocale(codec);
    //QTextCodec::setCodecForCStrings(codec);
    //QTextCodec::setCodecForTr(codec);

    TRadarVisualWnd w;
    w.show();

    QRectF rc;
    rc.setTopLeft(QPointF(73.55, 3.85));
    rc.setBottomRight(QPointF(135.08, 53.55));
    w.centerOn(rc);

	while (splashTime.elapsed() < 2000)
	{
		a.processEvents();
	}
	splash.finish(&w);

    return a.exec();
}
