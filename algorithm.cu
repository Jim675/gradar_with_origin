#include <iostream>
#include <chrono>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

#include "constant.h"

using std::cout;
using std::endl;

//constexpr double PI = 3.1415926535897932384626433832795;
//
//// 地球半径
//constexpr double RE = 6371393.0;
//
//// 等效地球半径
//constexpr double RM = RE * 4.0 / 3.0;
//
//// 雷达传播圆弧路径的半径为地球半径4倍
//constexpr double RN = RE * 4.0;
//
//// 2倍等效地球半径
////constexpr double RM_2 = RM * 2;
//
//// 赤道(equator)周长(circumference)  Equatorial Circumference
//constexpr double EC = 20037508.3427892430765884088807;
//
//

// CUDA kernel函数, 运行于GPU之上
__global__ void kernel_calc(double* d_x, double* d_y, double* d_az, double* d_r,
                            const double slon, const double slat, const double el, const double cv,
                            const unsigned int height, const unsigned int width)
{
    const unsigned int index = blockDim.x * blockIdx.x + threadIdx.x;
    if (index >= height * width) {
        return;
    }
    const unsigned int row = index / width;
    const unsigned int col = index % width;
    // 墨卡托直接转经纬度弧度
    const double dlon = d_x[col] / EC * PI - slon;
    const double lat = atan(exp(d_y[row] * PI / EC)) * 2.0 - PI / 2.0;
    /*printf("cuda: lon: %lf, lat: %lf\n", lon, lat);*/
    double sin_slat;
    double cos_slat;
    sincos(slat, &sin_slat, &cos_slat);// cuda函数，报错不用管
    double sin_lat;
    double cos_lat;
    sincos(lat, &sin_lat, &cos_lat);
    // 两经纬度之间的圆心角
    const double a = acos(sin_slat * sin_lat +
                          cos_slat * cos_lat * cos(dlon));
    const double sin_a = sin(a);
    // 传播路径弧长
    const double r = fabs(RN * (a + el + asin(cv * sin_a - sin(a + el))));
    d_r[index] = r;
    if (r >= 250.0) {
        // 已知经纬度求方位角, 这里az实际上是sin(az)，为了省寄存器
        double az = cos_lat * sin(dlon) / sin_a;
        if (az >= -1.0) {
            if (az <= 1.0) {
                az = asin(az);
            } else {
                az = PI / 2;
            }
        } else {
            az = -PI / 2;
        }
        const double dlat = lat - slat;
        // 修正方位角
        if (dlon >= 0) {
            if (dlat < 0) az = PI - az;
        } else {
            if (dlat >= 0) az += (2 * PI);
            else az = PI - az;
        }
        d_az[index] = az * 180.0 / PI;
    } else {
        d_az[index] = -1.0;
    }
    //printf("cuda: az: %lf, r: %lf\n", az, r);
}

// CUDA kernel函数, 运行于GPU之上
__global__ void kernel_calc2(double* d_x, double* d_y, double* d_az, double* d_r,
                             const double slon, const double slat, const double el, const double cv,
                             const unsigned int height, const unsigned int width)
{
    unsigned int i = blockDim.x * blockIdx.x + threadIdx.x;
    if (i >= height) {
        return;
    }
    // 墨卡托直接转经纬度弧度
    const double lat = atan(exp(d_y[i] * PI / EC)) * 2.0 - PI / 2.0;
    double sin_slat;
    double cos_slat;
    sincos(slat, &sin_slat, &cos_slat);// cuda函数，报错不用管
    double sin_lat;
    double cos_lat;
    sincos(lat, &sin_lat, &cos_lat);
    const double dlat = lat - slat;
    for (unsigned int j = 0; j < width; j++) {
        const double dlon = d_x[j] / EC * PI - slon;
        /*printf("cuda: lon: %lf, lat: %lf\n", lon, lat);*/
        // 两经纬度之间的圆心角
        const double a = acos(sin_slat * sin_lat +
                              cos_slat * cos_lat * cos(dlon));
        const double sin_a = sin(a);
        // 传播路径弧长
        const double r = fabs(RN * (a + el + asin(cv * sin_a - sin(a + el))));
        d_r[i * width + j] = r;

        if (r >= 250.0) {
            // 已知经纬度求方位角, 这里az实际上是sin(az)，为了省寄存器
            double az = cos_lat * sin(dlon) / sin_a;
            if (az >= -1.0) {
                if (az <= 1.0) {
                    az = asin(az);
                } else {
                    az = PI / 2;
                }
            } else {
                az = -PI / 2;
            }
            // 修正方位角
            if (dlon >= 0) {
                if (dlat < 0) az = PI - az;
            } else {
                if (dlat >= 0) az += (2 * PI);
                else az = PI - az;
            }
            d_az[i * width + j] = az * 180.0 / PI;
        } else {
            d_az[i * width + j] = -1.0;
        }
        //printf("cuda2: az: %lf, r: %lf\n", az, r);
    }
}

static void printDTime(std::chrono::steady_clock::time_point start, std::chrono::steady_clock::time_point end, const char* msg)
{
    auto dtime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    cout << msg << ": " << dtime.count() << "ms" << endl;
}

// 使用CUDA计算方位角和距离
extern "C" int calc_az_r(double* x, double* y,
                         double* az, double* r,
                         const double slon, const double slat, const double el, const double elev_add_RE_div_RN,
                         const size_t width, const size_t height)
{

    auto t0 = std::chrono::steady_clock::now();
    // 创建设备(GPU)上的数组
    double* d_x = nullptr;
    double* d_y = nullptr;
    double* d_az = nullptr;
    double* d_r = nullptr;

    const size_t n = width * height;
    const size_t wsize = width * sizeof(double);
    const size_t hsize = height * sizeof(double);
    const size_t tsize = n * sizeof(double);

    // 分配显存
    cudaError_t error = cudaMalloc(&d_x, wsize);
    if (error != cudaSuccess) {
        return error;
    }
    error = cudaMalloc(&d_y, hsize);
    if (error != cudaSuccess) {
        return error;
    }
    error = cudaMalloc(&d_az, tsize);
    if (error != cudaSuccess) {
        return error;
    }
    error = cudaMalloc(&d_r, tsize);
    if (error != cudaSuccess) {
        return error;
    }
    // 把数据从内存拷贝到显存
    error = cudaMemcpy(d_x, x, wsize, cudaMemcpyHostToDevice);
    if (error != cudaSuccess) {
        return error;
    }
    error = cudaMemcpy(d_y, y, hsize, cudaMemcpyHostToDevice);
    if (error != cudaSuccess) {
        return error;
    }
    // Thread -> Block -> Grid
    //dim3 blocksPerGrid(1);
    //unsigned N = 1024;
    //dim3  threadsPerBlock(N);
    unsigned int threadsPerBlock = 512;
    unsigned int blocksPerGrid = (n + threadsPerBlock - 1) / threadsPerBlock;
    auto t1 = std::chrono::steady_clock::now();
    printDTime(t0, t1, "CUDA t0 -> t1 spend time");
    kernel_calc << < blocksPerGrid, threadsPerBlock >> > (d_x, d_y, d_az, d_r,
                                                          slon, slat, el, elev_add_RE_div_RN,
                                                          (unsigned int)width, (unsigned int)height);
    cudaDeviceSynchronize();
    auto t2 = std::chrono::steady_clock::now();
    printDTime(t1, t2, "CUDA t1 -> t2 spend time");
    error = cudaGetLastError();
    if (error != cudaSuccess) {
        return error;
    }
    // Copy result from device memory to host memory
    // h_C contains the result in host memory
    error = cudaMemcpy(az, d_az, tsize, cudaMemcpyDeviceToHost);
    if (error != cudaSuccess) {
        cout << "数据拷贝回内存失败" << endl;
        return error;
    }
    error = cudaMemcpy(r, d_r, tsize, cudaMemcpyDeviceToHost);
    if (error != cudaSuccess) {
        cout << "数据拷贝回内存失败" << endl;
        return error;
    }
    // 释放显存
    cudaFree(d_x);
    cudaFree(d_y);
    cudaFree(d_az);
    cudaFree(d_r);
    // 释放内存
    auto t3 = std::chrono::steady_clock::now();
    printDTime(t2, t3, "CUDA t2 -> t3 spend time");
    printDTime(t0, t3, "CUDA spend time");
    return 0;
}

// 使用CUDA计算方位角和距离
extern "C" int calc_az_r2(double* x, double* y,
                          double* az, double* r,
                          const double slon, const double slat, const double el, const double elev_add_RE_div_RN,
                          const size_t width, const size_t height)
{

    auto t0 = std::chrono::steady_clock::now();
    // 创建设备(GPU)上的数组
    double* d_x = nullptr;
    double* d_y = nullptr;
    double* d_az = nullptr;
    double* d_r = nullptr;

    const size_t n = width * height;
    const size_t wsize = width * sizeof(double);
    const size_t hsize = height * sizeof(double);
    const size_t tsize = n * sizeof(double);

    // 分配显存
    cudaError_t error = cudaMalloc(&d_x, wsize);
    if (error != cudaSuccess) {
        return error;
    }
    error = cudaMalloc(&d_y, hsize);
    if (error != cudaSuccess) {
        return error;
    }
    error = cudaMalloc(&d_az, tsize);
    if (error != cudaSuccess) {
        return error;
    }
    error = cudaMalloc(&d_r, tsize);
    if (error != cudaSuccess) {
        return error;
    }
    // 把数据从内存拷贝到显存
    error = cudaMemcpy(d_x, x, wsize, cudaMemcpyHostToDevice);
    if (error != cudaSuccess) {
        return error;
    }
    error = cudaMemcpy(d_y, y, hsize, cudaMemcpyHostToDevice);
    if (error != cudaSuccess) {
        return error;
    }

    unsigned int threadsPerBlock = 512;
    unsigned int blocksPerGrid = (height + threadsPerBlock - 1) / threadsPerBlock;
    auto t1 = std::chrono::steady_clock::now();
    printDTime(t0, t1, "CUDA2 t0 -> t1 spend time");
    kernel_calc2 << < blocksPerGrid, threadsPerBlock >> > (d_x, d_y, d_az, d_r,
                                                           slon, slat, el, elev_add_RE_div_RN,
                                                           (unsigned int)width, (unsigned int)height);
    cudaDeviceSynchronize();
    auto t2 = std::chrono::steady_clock::now();
    printDTime(t1, t2, "CUDA2 t1 -> t2 spend time");
    error = cudaGetLastError();
    if (error != cudaSuccess) {
        return error;
    }
    error = cudaMemcpy(az, d_az, tsize, cudaMemcpyDeviceToHost);
    if (error != cudaSuccess) {
        cout << "数据拷贝回内存失败" << endl;
        return error;
    }
    error = cudaMemcpy(r, d_r, tsize, cudaMemcpyDeviceToHost);
    if (error != cudaSuccess) {
        cout << "数据拷贝回内存失败" << endl;
        return error;
    }
    // 释放显存
    cudaFree(d_x);
    cudaFree(d_y);
    cudaFree(d_az);
    cudaFree(d_r);
    auto t3 = std::chrono::steady_clock::now();
    printDTime(t2, t3, "CUDA2 t2 -> t3 spend time");
    printDTime(t0, t3, "CUDA2 spend time");
    return 0;
}

// 使用CUDA计算方位角和距离
extern "C" int calc_az_r3(double* x, double* y,
                          double** az, double** r,
                          const double slon, const double slat, const double el, const double elev_add_RE_div_RN,
                          const size_t width, const size_t height)
{

    auto t0 = std::chrono::steady_clock::now();
    // 创建设备(GPU)上的数组
    double* d_x = nullptr;
    double* d_y = nullptr;
    //double* d_az = nullptr;
    //double* d_r = nullptr;

    const size_t n = width * height;
    const size_t wsize = width * sizeof(double);
    const size_t hsize = height * sizeof(double);
    const size_t tsize = n * sizeof(double);

    // 分配显存
    cudaError_t error = cudaMalloc(&d_x, wsize);
    if (error != cudaSuccess) {
        return error;
    }
    error = cudaMalloc(&d_y, hsize);
    if (error != cudaSuccess) {
        return error;
    }
    error = cudaMallocManaged(az, tsize);
    if (error != cudaSuccess) {
        return error;
    }
    error = cudaMallocManaged(r, tsize);
    if (error != cudaSuccess) {
        return error;
    }
    // 把数据从内存拷贝到显存
    error = cudaMemcpy(d_x, x, wsize, cudaMemcpyHostToDevice);
    if (error != cudaSuccess) {
        return error;
    }
    error = cudaMemcpy(d_y, y, hsize, cudaMemcpyHostToDevice);
    if (error != cudaSuccess) {
        return error;
    }
    auto t1 = std::chrono::steady_clock::now();
    // Invoke kernel
    unsigned int threadsPerBlock = 512;
    unsigned int blocksPerGrid = (height + threadsPerBlock - 1) / threadsPerBlock;
    kernel_calc2 << < blocksPerGrid, threadsPerBlock >> > (d_x, d_y, *az, *r,
                                                           slon, slat, el, elev_add_RE_div_RN,
                                                           (unsigned int)width, (unsigned int)height);
    cudaDeviceSynchronize();
    auto t2 = std::chrono::steady_clock::now();
    printDTime(t1, t2, "CUDA3 calc kernel spend time");
    error = cudaGetLastError();
    if (error != cudaSuccess) {
        return error;
    }
    // 释放显存
    cudaFree(d_x);
    cudaFree(d_y);
    auto t3 = std::chrono::steady_clock::now();
    printDTime(t0, t3, "CUDA3 spend time");
    return 0;
}