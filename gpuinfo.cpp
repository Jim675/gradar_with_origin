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
    std::vector <IDXGIAdapter*> vAdapters;  // �Կ�
    int iAdapterNum = 0;                    // �Կ�������

    // ����һ��DXGI����
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&pFactory));

    if (FAILED(hr)) {
        throw std::exception();
        return {};
    }

    // ö��������
    while (pFactory->EnumAdapters(iAdapterNum, &pAdapter) != DXGI_ERROR_NOT_FOUND) {
        vAdapters.push_back(pAdapter);
        ++iAdapterNum;
    }

    // ��Ϣ���
    qDebug()<< "===============��ȡ��" << iAdapterNum << "���Կ�===============";
    for (size_t i = 0; i < vAdapters.size(); i++)
    {
        // ��ȡ��Ϣ
        DXGI_ADAPTER_DESC adapterDesc;
        vAdapters[i]->GetDesc(&adapterDesc);

        // ����Կ���Ϣ
        qDebug() << L"ϵͳ��Ƶ�ڴ�:" << adapterDesc.DedicatedSystemMemory / 1024 / 1024 << "M";
        qDebug() << L"ר����Ƶ�ڴ�:" << adapterDesc.DedicatedVideoMemory / 1024 / 1024 << "M";
        qDebug() << L"����ϵͳ�ڴ�:" << adapterDesc.SharedSystemMemory / 1024 / 1024 << "M";

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
    QMessageBox::info(nullptr, "--", "������ܻ�ûʵ��");
    return false;
}

#endif
