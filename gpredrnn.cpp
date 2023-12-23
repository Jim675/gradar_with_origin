#include "gpredrnn.h"
#undef slots
#include <torch/torch.h>
#include <torch/script.h>
#define slots Q_SLOTS
#include <vector>
#include <qdebug.h>
#include <QImage>
#include <cmath>

using namespace torch;

QImage TensorToQImage(const torch::Tensor& tensor)
{
    int dim = tensor.dim();
    if (dim != 4) {
        qFatal("dim must be 4.");
    }
 
    int channels = tensor.size(1);
    assert(channels == 1);

    int width = tensor.size(2);
    int height = tensor.size(3);
    
    QImage image(width, height, QImage::Format_Grayscale8);
    auto byte_tensor = tensor * 255;
    byte_tensor = byte_tensor.clamp(0, 255);
    byte_tensor = byte_tensor.toType(torch::kByte);
    memcpy(image.bits(), byte_tensor[0][0].data_ptr(), width * height);
    return image;
}

/*!
    QImage to torch::Tensor N(1) x C(1) x H x W
*/
torch::Tensor QImageToTensor(const QImage& image)
{
    int width    = image.width();
    int height   = image.height();
    int channels = 1;

    // create tensor
    torch::TensorOptions option(torch::kByte);
    torch::Tensor tensor = torch::zeros({ 1,channels,width, height}, option);//N C H W
    // fill tensor
    memcpy(tensor[0][0].data_ptr(), image.bits(), width * height);
    
    return tensor.toType(torch::kFloat32) / 255.0f;
}

class PredRNN : public IPredict {
public:
	virtual ~PredRNN() {
		delete device;
		device = nullptr;
        
	}

	void loadModel(std::string path) {
		device = new Device(kCUDA);
		model = jit::load(path);
		model.to(*device);
		model.eval();
	}
    

	virtual void setProperties(int thepredictNum, int imgwidth, int patchsize, float maxmin) override {
		this->thepredictNum = thepredictNum;
		this->imgwidth = imgwidth;
		this->patchsize = patchsize;
		this->maxmin = maxmin;
	}

	virtual QVector<QVector<QImage>> predict(const QVector<QVector<QImage>>& input, bool issimvp) override {

        QVector<QVector<QImage>> res;

        for (int j = 0; j < input[0].size(); ++j) {
            std::vector<torch::Tensor> input_mat;
            for (int i = 0; i < input.size(); ++i) {
                QImage qimg = input[i][j];
                //qimg.save(QString("D:\\output\\origin_%1_%2.png").arg(j).arg(i));
                auto tens = QImageToTensor(qimg);
                //QImage back = TensorToQImage(tens);
                //back.save(QString("D:\\output\\test_%1_%2.png").arg(j).arg(i));
                input_mat.push_back(tens);
            }

            QVector<QImage> predictonce = predictsingle(input_mat, issimvp);
            for (int i = 0; i < 5; i++)
            {
                QString filename = QString("D:\\output\\%1_%2.png").arg(j).arg(i);
                predictonce[i].save(filename);
            }
            res.push_back(predictonce);
        }
        QVector<QVector<QImage>> tres;
        for (int i = 0; i < 5; i++)
        {
            QVector<QImage> image;
            for (int j = 0; j < 9; j++)
            {
                image.push_back(res[j][i]);
            }
            tres.push_back(image);
        }
        return tres;
	}
private:
    QVector<QImage> predictsingle(std::vector<torch::Tensor>& imgs, bool simvp);
    torch::Tensor reshape_patch(torch::Tensor& img_tensor, int patch_size);
    torch::Tensor reshape_patch_back(torch::Tensor& patch_tensor, int patch_size);

private:
	jit::script::Module model;
	Device* device = nullptr;
	int thepredictNum = -1;
	int imgwidth = -1;
	int patchsize = -1;
	float maxmin = -1;
};

IPredict* IPredict::load(std::string path) {
	auto pred = new PredRNN();
	pred->loadModel(path);
	return pred;
}



QVector<QImage> PredRNN::predictsingle(std::vector<torch::Tensor>& imgs, bool simvp = false) {
    c10::IValue output;
    torch::Tensor input_tensor = torch::zeros({ 5, 1, imgwidth, imgwidth });
    qDebug("input %d %d %d %d", imgs[0].size(0), imgs[0].size(1), imgs[0].size(2), imgs[0].size(3));//b c 920 920
    for (int i = 0; i < 5; i++) {
        input_tensor[i] = imgs[i][0] ;//imgs:5 b c h w    input_tensor:5 c h w
    }
    
    if (simvp)
    {   
        input_tensor = input_tensor.unsqueeze(0);
        input_tensor = input_tensor.to(at::kCUDA);
        qDebug("input %d %d %d %d %d", input_tensor.size(0), input_tensor.size(1), input_tensor.size(2), input_tensor.size(3), input_tensor.size(4));
        output = model.forward({ input_tensor });
    }
    else
    {
        input_tensor = input_tensor.squeeze(1);//5 h w
        input_tensor = input_tensor.unsqueeze(3);//5 h w 1
        torch::Tensor zeros = torch::zeros({ 5, imgwidth, imgwidth, 1 });
        torch::Tensor input = torch::cat({ input_tensor, zeros }, 0);
        input = input.reshape({ 1, 10, imgwidth, imgwidth, 1 });
        torch::Tensor reshape_patchs = reshape_patch(input, patchsize);
        torch::Tensor maskflag = torch::zeros({ 1, 4, imgwidth / patchsize, imgwidth / patchsize, patchsize * patchsize });
        reshape_patchs = reshape_patchs.to(*device);
        maskflag = maskflag.to(*device);
        at::NoGradGuard nograd;
        output = model.forward({ reshape_patchs, maskflag });
    }



    torch::Tensor output_tensor;
    QVector<torch::Tensor> res;
    torch::Tensor patch_tensorxx;
    if (simvp)
    {
        output_tensor = output.toTensor();
        output_tensor = output_tensor.squeeze(2);//œ÷‘⁄ «B 5 H W
        qDebug("output_tensor %d %d %d %d", output_tensor.size(0), output_tensor.size(1), output_tensor.size(2), output_tensor.size(3));
        patch_tensorxx = output_tensor.cpu();
        auto sizes = patch_tensorxx.sizes();//bdhw
        qDebug() << "qwert" << sizes[0]<< " "<< sizes[1]<<" "<< sizes[2]<< " "<< sizes[3];
        
        for (int i = 0; i < 5; i++) {
            torch::Tensor frame = patch_tensorxx.select(0, 0).select(0, i);// 436 436
            res.push_back(frame.view({ 1, 1, imgwidth, imgwidth }));
        }
        qDebug() << "222";
    }
    else
    {
        output_tensor = output.toTuple()->elements()[0].toTensor();

        patch_tensorxx = reshape_patch_back(output_tensor, patchsize);//bdhwc
        patch_tensorxx = patch_tensorxx.squeeze(-1);//bdhw

        patch_tensorxx = patch_tensorxx.cpu();
        auto sizes = patch_tensorxx.sizes();
        for (int i = 0; i < 5; i++) {
            torch::Tensor frame = patch_tensorxx.select(0, 0).select(0, i);// 436 436
            res.push_back(frame.view({ 1, 1, imgwidth, imgwidth }));
        }
    }
    qDebug() << "333";

    
        


    QVector<QImage> resvector;
    for (int k = 0; k < 5; k++) {

        resvector.push_back(TensorToQImage(res[k]));
    }
    qDebug() << "444";
    return resvector;
}

torch::Tensor PredRNN::reshape_patch(torch::Tensor& img_tensor, int patch_size) {
    // assert tensor shape 
    AT_ASSERTM(img_tensor.ndimension() == 5, "Tensor must be 5D");

    int batch_size = img_tensor.size(0);
    int seq_length = img_tensor.size(1);
    int img_height = img_tensor.size(2);
    int img_width = img_tensor.size(3);
    int num_channels = img_tensor.size(4);

    torch::Tensor reshaped = img_tensor.reshape({
        batch_size, seq_length,
        img_height / patch_size, patch_size,
        img_width / patch_size, patch_size,
        num_channels
        });

    torch::Tensor transposed = reshaped.permute({ 0,1,2,4,3,5,6 });

    torch::Tensor patch_tensor = transposed.reshape({
        batch_size, seq_length,
        img_height / patch_size,
        img_width / patch_size,
        patch_size * patch_size * num_channels
        });

    return patch_tensor;
}

torch::Tensor PredRNN::reshape_patch_back(torch::Tensor& patch_tensor, int patch_size) {
    // Assert tensor shape
    AT_ASSERTM(patch_tensor.ndimension() == 5, "Tensor must be 5D");

    int batch_size = patch_tensor.size(0);
    int seq_length = patch_tensor.size(1);
    int patch_height = patch_tensor.size(2);
    int patch_width = patch_tensor.size(3);
    int channels = patch_tensor.size(4);
    int img_channels = channels / (patch_size * patch_size);
    torch::Tensor reshaped = patch_tensor.reshape({
        batch_size, seq_length,
        patch_height, patch_width,
        patch_size, patch_size,
        img_channels
        });

    torch::Tensor transposed = reshaped.permute({ 0,1,2,4,3,5,6 });

    torch::Tensor img_tensor = transposed.reshape({
        batch_size, seq_length,
        patch_height * patch_size,
        patch_width * patch_size,
        img_channels
        });

    return img_tensor;
}