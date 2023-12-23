#pragma once
#include <QImage>

class IPredict {
public:

	static IPredict* load(std::string path);

	virtual ~IPredict() {}
	
	virtual void setProperties(int thepredictNum, int imgwidth, int patchsize, float maxmin) = 0;

	virtual QVector<QVector<QImage>> predict(const QVector<QVector<QImage>>& input, bool issimvp) = 0;
};



