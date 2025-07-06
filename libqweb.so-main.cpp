// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qpa/qplatformintegrationplugin.h>
#include "qvncintegration.h"
#include <QDebug>
#include <unistd.h>
#include <cstdlib>
#include <QImage>
#include <QtConcurrent/QtConcurrentMap>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QVncIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "vnc.json")
public:
    QPlatformIntegration *create(const QString&, const QStringList&) override;
};

template <typename T>
T* rgba64_pixel_data_ptr(QImage& img, int x, int y) {
    if(Q_UNLIKELY(!img.valid(x, y))) return nullptr;
    return reinterpret_cast<T*>(
        img.scanLine(y) + x * sizeof(quint64)
        );
}

QPlatformIntegration* QVncIntegrationPlugin::create(const QString& system, const QStringList& paramList)
{
    if (!system.compare("web"_L1, Qt::CaseInsensitive)) {
        qDebug() << "VNC" << 1000+system.compare("xnc"_L1, Qt::CaseInsensitive) << "END";
        qDebug() << "Size of pointer: " << sizeof(void *);
        qDebug() << "Page Size:" << sysconf(_SC_PAGESIZE);
        qDebug() << "CPU Cache Size:" << sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
        QImage mImage;
        if (void *data = nullptr; !posix_memalign(&data, sysconf(_SC_PAGESIZE), sysconf(_SC_PAGESIZE)))
            mImage = QImage(static_cast<uchar *>(data),
                                   sysconf(_SC_LEVEL1_DCACHE_LINESIZE)/sizeof(quint64),
                                   sysconf(_SC_PAGESIZE)/sysconf(_SC_LEVEL1_DCACHE_LINESIZE),
                                   sysconf(_SC_LEVEL1_DCACHE_LINESIZE),
                                   QImage::Format_RGBA64,
                                   std::free,
                                   data);
        qDebug() << "Format of mImage:" << mImage.format();
        qDebug() << "Width of mImage:" << mImage.width();
        qDebug() << "BytePerLine of mImage:" << mImage.bytesPerLine();
        qDebug() << "Height of mImage:" << mImage.height();
        qDebug() << "Size of mImageData:" << mImage.sizeInBytes();
        qDebug() << "Array mImage address:" << reinterpret_cast<uintptr_t>(mImage.constBits());
        qDebug() << "Page aligned:" << (( reinterpret_cast<uintptr_t>(mImage.constBits()) % sysconf(_SC_PAGESIZE) == 0) ? "Yes" : "No");
        //QtConcurrent::blockingMap(
        //    [&] {
        //        QList<int> list;
        //        for(int i = 0; i < mImage.height(); ++i)
        //            list << i;
        //        return list;
        //    }(),
        //    [&](int h) {
        //    void **line = reinterpret_cast<void **>(mImage.scanLine(h));
        //    std::memset(line, 0, mImage.bytesPerLine());
        //    });
        // 使用示例
        //if(auto* ptr = imagePixel<int*>(mImage, x, y)) {
        //    *ptr = &valid_object;  // 确保对象生命周期足够
        //}
        int i {0};
        for (int y = 0; y < 64; y++) {
            for(int x = 0; x < 8; x++) {
                //*(reinterpret_cast<int **>(mImage.scanLine(y)) + x) = &i;
                if(auto* ptr = rgba64_pixel_data_ptr<int *>(mImage, x, y)) {
                    *ptr = &i;  // 确保对象生命周期足够
                }
                *(reinterpret_cast<int **>(mImage.scanLine(0)) + 0) = &i;
                qDebug() << "Address(" << i++ << ")"
                         << reinterpret_cast<uintptr_t>(reinterpret_cast<void **>(mImage.scanLine(y)) + x)
                         << "is nullptr:"
                         << (*(reinterpret_cast<void **>(mImage.scanLine(y)) + x) == nullptr ? "Yes" : "No")
                         << "and content is"
                         << reinterpret_cast<uintptr_t>(*(reinterpret_cast<int **>(mImage.scanLine(y)) + x));
            }
        }
        return new QVncIntegration(paramList);
    }

    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"

