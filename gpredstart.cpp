//#include "gpredstart.h"
//
//
////�õ�����
//void GPredStart::setImages(QVector<QVector<QImage>>& Images)
//{
//    this->Images = Images;
//}
//
////�õ�Ԥ��������
//QVector<QImage> GPredStart::getImages()
//{
//    return res;
//}
//void GPredStart::startPre(QVector<QVector<QImage>> images)
//{
//    PredRNN pred("D:/Data/predrnn_cuda.pt");
//
//   
//    QVector<QImage> res = pred.predict(images);
//}
///*
//void GPredStart::startPre(QVector<QVector<QImage>> images, QVector<QImage>& res)
//{
//    PredRNN pred("D:/Data/predrnn_cuda.pt");
//    pred.predict(images, res);
//}*/
///*
//void GPredStart::start(string path, string svaepath)
//{
//	qDebug() << "1" << endl;
//	PredRNN pred("D:/Data/predrnn_cuda.pt");
//	qDebug() << "2" << endl;
//
//	
//	//����ʹ��QImage��ȡ���ļ��ܲ���Ԥ��
//    QVector<QVector<QImage>> images;
//    QDir dir("D:\\Data\\NEW");
//    QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
//    for (int i = 0; i < subDirs.size(); i++) {
//        QString subDirPath = "D:/Data/NEW/"  + subDirs[i];
//        QDir subDir(subDirPath);
//        QStringList imageFiles = subDir.entryList(QDir::Files);
//        QVector<QImage> subImages;
//        for (int j = 0; j < imageFiles.size(); j++) {
//            QString imagePath = subDirPath + "/" + imageFiles[j];
//            QImage image(imagePath);
//            subImages.push_back(image);
//        }
//        images.push_back(subImages);
//    }
//
//    // ��ӡ��״
//    qDebug() << "���ļ�������:" << images.size();
//    for (int i = 0; i < images.size(); i++) {
//        qDebug() << "�ļ���" << i << "��ͼƬ����:" << images[i].size();
//    }
//    QVector<QImage> res;
//	//QVector<QImage> res = pred.predict(images, res);
//    pred.predict(images, res);
//	//vector<vector<cv::Mat>> res = pred.predict(path, svaepath);//res[i][j]�ǵ�i�����ǵĵ�j��ʱ�̵�Ԥ��ͼ
//	//vector<vector<cv::Mat>> res = pred.predict("F:/predModel/Desktop/", "F:/predModel/predrnn_output_test/");//res[i][j]�ǵ�i�����ǵĵ�j��ʱ�̵�Ԥ��ͼ
//	qDebug() << "3" << endl;
//
//}*/
