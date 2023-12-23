#pragma once

#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>

// 用于切片显示的颜色表
class GSliceColorMap : public vtkColorTransferFunction
{
public:
    static GSliceColorMap* New();

    void SetColorTransferFunction(vtkColorTransferFunction* colorTF)
    {
        double data[6] = {};
        for (int i = 0; i < colorTF->GetSize(); i++) 
        {
            colorTF->GetNodeValue(i, data);
            AddRGBPoint(data[0], data[1], data[2], data[3], data[4], data[5]);
        }
        Build();
    }

    void SetOpacityTransferFunction(vtkPiecewiseFunction* opacityTF)
    {
        this->mOpacityTF = opacityTF;
    }

    /**
    * Map a set of scalars through the lookup table.
    */
    void MapScalarsThroughTable2(void* input, unsigned char* output, int inputDataType,
        int numberOfValues, int inputIncrement, int outputIncrement) override;

private:
    GSliceColorMap();

    ~GSliceColorMap() override = default;

    //double range[2] = {};

    vtkPiecewiseFunction* mOpacityTF = nullptr;

    template<class T>
    void MapDoubleScalars(T* input, unsigned char* output, int length, int inIncr, int outFormat);
};