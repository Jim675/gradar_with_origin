#include "gpuinfo.h"
#include <stdexcept>
#include <QDebug>
#include <qmessagebox.h>

#ifdef _WIN32
#include <dxgi.h>
#pragma comment(lib, "dxgi.lib")

bool is_gpu_memory_suitable() {

    IDXGIFactory* pFactory;
    IDXGIAdapter* pAdapter;
    std::vector <IDXGIAdapter*> vAdapters;  // 显卡
    int iAdapterNum = 0;                    // 显卡的数量

    // 创建一个DXGI工厂
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&pFactory));

    if (FAILED(hr)) {
        throw std::exception();
        return {};
    }

    // 枚举适配器
    while (pFactory->EnumAdapters(iAdapterNum, &pAdapter) != DXGI_ERROR_NOT_FOUND) {
        vAdapters.push_back(pAdapter);
        ++iAdapterNum;
    }

    // 信息输出
    qDebug()<< "===============获取到" << iAdapterNum << "块显卡===============";
    for (size_t i = 0; i < vAdapters.size(); i++)
    {
        // 获取信息
        DXGI_ADAPTER_DESC adapterDesc;
        vAdapters[i]->GetDesc(&adapterDesc);

        // 输出显卡信息
        qDebug() << L"系统视频内存:" << adapterDesc.DedicatedSystemMemory / 1024 / 1024 << "M";
        qDebug() << L"专用视频内存:" << adapterDesc.DedicatedVideoMemory / 1024 / 1024 << "M";
        qDebug() << L"共享系统内存:" << adapterDesc.SharedSystemMemory / 1024 / 1024 << "M";

        if (adapterDesc.DedicatedSystemMemory / 1024 /1024 / 1024 >= 8 ||
            adapterDesc.DedicatedVideoMemory / 1024 / 1024 / 1024 >= 8||
            adapterDesc.SharedSystemMemory / 1024 / 1024 / 1024 >= 8) {
            return true;
        }
    }
    vAdapters.clear();

    return false;
}

#else

bool is_gpu_memory_suitable() {
    QMessageBox::info(nullptr, "--", "这个功能还没实现");
    return false;
}

#endif
