#include "gslicecolormap.h"

GSliceColorMap* GSliceColorMap::New()
{
    return new GSliceColorMap();
}

GSliceColorMap::GSliceColorMap()
{
    SetClamping(false);
    SetAllowDuplicateScalars(false);
}


void GSliceColorMap::MapScalarsThroughTable2(void* input, unsigned char* output,
    int inputDataType, int numberOfValues, int inputIncrement, int outputFormat)
{
    if (this->GetSize() == 0) {
        vtkDebugMacro("Transfer Function Has No Points!");
        return;
    }
    if (this->IndexedLookup) {
        vtkErrorMacro(<< "MapImageThroughTable: Unknown input ScalarType");
    } else {
        if (inputDataType == VTK_DOUBLE) {
            MapDoubleScalars(reinterpret_cast<double*>(input), output, numberOfValues, inputIncrement, outputFormat);
        } else if (inputDataType == VTK_FLOAT) {
            MapDoubleScalars(reinterpret_cast<float*>(input), output, numberOfValues, inputIncrement, outputFormat);
        } else {
            vtkErrorMacro(<< "MapImageThroughTable: Unsupported input ScalarType" << inputDataType);
        }
    }
}

//------------------------------------------------------------------------------
// Accelerate the mapping by copying the data in 32-bit chunks instead
// of 8-bit chunks.  The extra "long" argument is to help broken
// compilers select the non-templates below for unsigned char
// and unsigned short.
template<class T>
void GSliceColorMap::MapDoubleScalars(T* input, unsigned char* output, int length, int inIncr, int outFormat)
{
    int i = length;
    double rgb[3];
    unsigned char* optr = output;
    T* iptr = input;

    while (--i >= 0) 
    {
        T x = *iptr;
        GetColor(x, rgb);
        unsigned char alpha = 0;

        //if (mOpacityTF) {
        //    alpha = static_cast<unsigned char>(mOpacityTF->GetValue(x) * 255 + 0.5);
        //} else {
        //    if (Range[0] <= x && Range[1] >= x) {
        //        alpha = 255;
        //    }
        //}
        if (Range[0] <= x && Range[1] >= x) 
        {
            alpha = 255;
        }

        if (outFormat == VTK_RGB || outFormat == VTK_RGBA) 
        {
            *(optr++) = static_cast<unsigned char>(rgb[0] * 255.0 + 0.5);
            *(optr++) = static_cast<unsigned char>(rgb[1] * 255.0 + 0.5);
            *(optr++) = static_cast<unsigned char>(rgb[2] * 255.0 + 0.5);
        } 
        else // LUMINANCE  use coeffs of (0.30  0.59  0.11)*255.0
        {
            *(optr++) =
                static_cast<unsigned char>(rgb[0] * 76.5 + rgb[1] * 150.45 + rgb[2] * 28.05 + 0.5);
        }

        if (outFormat == VTK_RGBA || outFormat == VTK_LUMINANCE_ALPHA) 
        {
            *(optr++) = alpha;
        }
        iptr += inIncr;
    }
}
