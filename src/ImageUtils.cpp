#include "ImageUtils.h"

#if defined(__GNUC__)
#define omp_get_num_procs()     (1)
#else
#include <omp.h>
#endif

const uint qt_bayer_matrix[16][16] = {
    { 0x1, 0xc0, 0x30, 0xf0, 0xc, 0xcc, 0x3c, 0xfc,
      0x3, 0xc3, 0x33, 0xf3, 0xf, 0xcf, 0x3f, 0xff},
    { 0x80, 0x40, 0xb0, 0x70, 0x8c, 0x4c, 0xbc, 0x7c,
      0x83, 0x43, 0xb3, 0x73, 0x8f, 0x4f, 0xbf, 0x7f},
    { 0x20, 0xe0, 0x10, 0xd0, 0x2c, 0xec, 0x1c, 0xdc,
      0x23, 0xe3, 0x13, 0xd3, 0x2f, 0xef, 0x1f, 0xdf},
    { 0xa0, 0x60, 0x90, 0x50, 0xac, 0x6c, 0x9c, 0x5c,
      0xa3, 0x63, 0x93, 0x53, 0xaf, 0x6f, 0x9f, 0x5f},
    { 0x8, 0xc8, 0x38, 0xf8, 0x4, 0xc4, 0x34, 0xf4,
      0xb, 0xcb, 0x3b, 0xfb, 0x7, 0xc7, 0x37, 0xf7},
    { 0x88, 0x48, 0xb8, 0x78, 0x84, 0x44, 0xb4, 0x74,
      0x8b, 0x4b, 0xbb, 0x7b, 0x87, 0x47, 0xb7, 0x77},
    { 0x28, 0xe8, 0x18, 0xd8, 0x24, 0xe4, 0x14, 0xd4,
      0x2b, 0xeb, 0x1b, 0xdb, 0x27, 0xe7, 0x17, 0xd7},
    { 0xa8, 0x68, 0x98, 0x58, 0xa4, 0x64, 0x94, 0x54,
      0xab, 0x6b, 0x9b, 0x5b, 0xa7, 0x67, 0x97, 0x57},
    { 0x2, 0xc2, 0x32, 0xf2, 0xe, 0xce, 0x3e, 0xfe,
      0x1, 0xc1, 0x31, 0xf1, 0xd, 0xcd, 0x3d, 0xfd},
    { 0x82, 0x42, 0xb2, 0x72, 0x8e, 0x4e, 0xbe, 0x7e,
      0x81, 0x41, 0xb1, 0x71, 0x8d, 0x4d, 0xbd, 0x7d},
    { 0x22, 0xe2, 0x12, 0xd2, 0x2e, 0xee, 0x1e, 0xde,
      0x21, 0xe1, 0x11, 0xd1, 0x2d, 0xed, 0x1d, 0xdd},
    { 0xa2, 0x62, 0x92, 0x52, 0xae, 0x6e, 0x9e, 0x5e,
      0xa1, 0x61, 0x91, 0x51, 0xad, 0x6d, 0x9d, 0x5d},
    { 0xa, 0xca, 0x3a, 0xfa, 0x6, 0xc6, 0x36, 0xf6,
      0x9, 0xc9, 0x39, 0xf9, 0x5, 0xc5, 0x35, 0xf5},
    { 0x8a, 0x4a, 0xba, 0x7a, 0x86, 0x46, 0xb6, 0x76,
      0x89, 0x49, 0xb9, 0x79, 0x85, 0x45, 0xb5, 0x75},
    { 0x2a, 0xea, 0x1a, 0xda, 0x26, 0xe6, 0x16, 0xd6,
      0x29, 0xe9, 0x19, 0xd9, 0x25, 0xe5, 0x15, 0xd5},
    { 0xaa, 0x6a, 0x9a, 0x5a, 0xa6, 0x66, 0x96, 0x56,
      0xa9, 0x69, 0x99, 0x59, 0xa5, 0x65, 0x95, 0x55}
};

struct RGB Cube216ColorTable[256];
#define MAX_R 5
#define MAX_G 5
#define MAX_B 5
#define INDEXOF(__r,__g,__b) (((__r)*(MAX_G+1)+(__g))*(MAX_B+1)+(__b))

typedef void (*DitherFunc)(uint8_t* dest_data, uint8_t* src_data,
    uint8_t* transMask,
    int srcBytesPreLine, int width, int height,
    QRect bbox);

void cube216ColorTableInit()
{

    for (int rc = 0; rc <= MAX_R; rc++)                // build 6x6x6 color cube
        for (int gc = 0; gc <= MAX_G; gc++)
            for (int bc = 0; bc <= MAX_B; bc++)
            {
                struct RGB* color = &Cube216ColorTable[INDEXOF(rc, gc, bc)];
                color->r = rc * 255 / MAX_R;
                color->g = gc * 255 / MAX_G;
                color->b = bc * 255 / MAX_B;
            }
    Cube216ColorTable[0xff].b = 0;
    Cube216ColorTable[0xff].r = 255;
    Cube216ColorTable[0xff].g = 0;

    Cube216ColorTable[0xfe].b = 0;
    Cube216ColorTable[0xfe].r = 0;
    Cube216ColorTable[0xfe].g = 255;
}

const uint8_t* GetCube216ColorTable()
{
    bool init = false;
    if (!init) {
        cube216ColorTableInit();
    }
    init = true;
    return (const uint8_t*)Cube216ColorTable;
}
void ThresholdDither(uint8_t* dest_data, uint8_t* src_data,
    uint8_t* transMask,
    int srcBytesPreLine, int width, int height,
    QRect bbox)
{
    if (bbox.isEmpty()) {
        bbox.setRect(0, 0, width, height);
    }
    int y;
    for (y = bbox.y(); y < bbox.y() + bbox.height(); y++) {
        int x = bbox.x();
        const QRgb* p = (const QRgb*)(src_data + y * srcBytesPreLine + x * sizeof(QRgb));
        const QRgb* end = p + bbox.width();
        uchar* b = (dest_data + y * width + x);
        uchar* mask = (transMask + y * width + x);
#define DITHER(p,m) ((uchar) ((p * (m) + 127) / 255))
        while (p < end) {
            if (*mask == 0) {
                *b = TransIndex;
            }
            else {
                *b =
                    INDEXOF(
                        DITHER(qRed(*p), MAX_R),
                        DITHER(qGreen(*p), MAX_G),
                        DITHER(qBlue(*p), MAX_B)
                    );
            }
            b++;
            p++;
            x++;
            mask++;
        }
#undef DITHER
    }

}

void DiffuseDither(uint8_t* dest_data, uint8_t* src_data,
    uint8_t* transMask,
    int srcBytesPreLine, int width, int height,
    QRect bbox) 
{
    int* line1[3];
    int* line2[3];
    int* pv[3];
    src_data = src_data + bbox.y() * srcBytesPreLine + bbox.x() * sizeof(QRgb);
    int dstBytesPreLine = width;
    dest_data = dest_data + bbox.y() * dstBytesPreLine + bbox.x();
    width = bbox.width();
    height = bbox.height();
    QScopedArrayPointer<int> lineBuffer(new int[width * 9]);
    line1[0] = lineBuffer.data();
    line2[0] = lineBuffer.data() + width;
    line1[1] = lineBuffer.data() + width * 2;
    line2[1] = lineBuffer.data() + width * 3;
    line1[2] = lineBuffer.data() + width * 4;
    line2[2] = lineBuffer.data() + width * 5;
    pv[0] = lineBuffer.data() + width * 6;
    pv[1] = lineBuffer.data() + width * 7;
    pv[2] = lineBuffer.data() + width * 8;

    int endian = (QSysInfo::ByteOrder == QSysInfo::BigEndian);
    for (int y = 0; y < height; y++) {
        const uchar* q = src_data;
        const uchar* q2 = y < height - 1 ? q + srcBytesPreLine : src_data;
        uchar* b = dest_data;
        for (int chan = 0; chan < 3; chan++) {
            int* l1 = (y & 1) ? line2[chan] : line1[chan];
            int* l2 = (y & 1) ? line1[chan] : line2[chan];
            if (y == 0) {
                for (int i = 0; i < width; i++)
                    l1[i] = q[i * 4 + chan + endian];
            }
            if (y + 1 < height) {
                for (int i = 0; i < width; i++)
                    l2[i] = q2[i * 4 + chan + endian];
            }
            // Bi-directional error diffusion
            if (y & 1) {
                for (int x = 0; x < width; x++) {
                    int pix = qMax(qMin(5, (l1[x] * 5 + 128) / 255), 0);
                    int err = l1[x] - pix * 255 / 5;
                    pv[chan][x] = pix;

                    // Spread the error around...
                    if (x + 1 < width) {
                        l1[x + 1] += (err * 7) >> 4;
                        l2[x + 1] += err >> 4;
                    }
                    l2[x] += (err * 5) >> 4;
                    if (x > 1)
                        l2[x - 1] += (err * 3) >> 4;
                }
            }
            else {
                for (int x = width; x-- > 0;) {
                    int pix = qMax(qMin(5, (l1[x] * 5 + 128) / 255), 0);
                    int err = l1[x] - pix * 255 / 5;
                    pv[chan][x] = pix;

                    // Spread the error around...
                    if (x > 0) {
                        l1[x - 1] += (err * 7) >> 4;
                        l2[x - 1] += err >> 4;
                    }
                    l2[x] += (err * 5) >> 4;
                    if (x + 1 < width)
                        l2[x + 1] += (err * 3) >> 4;
                }
            }
        }
        if (endian) {
            for (int x = 0; x < width; x++) {
                *b++ = INDEXOF(pv[0][x], pv[1][x], pv[2][x]);
            }
        }
        else {
            for (int x = 0; x < width; x++) {
                *b++ = INDEXOF(pv[2][x], pv[1][x], pv[0][x]);
            }
        }
        src_data += srcBytesPreLine;
        dest_data += dstBytesPreLine;
    }
}

void OrderedDither(uint8_t* dest_data, uint8_t* src_data,
    uint8_t* transMask,
    int srcBytesPreLine, int width, int height,
    QRect bbox)
{
    if (bbox.isEmpty()) {
        bbox.setRect(0, 0, width, height);
    }
    int y;
    for (y = bbox.y(); y < bbox.y() + bbox.height(); y++) {
        int x = bbox.x();
        const QRgb* p = (const QRgb*)(src_data + y * srcBytesPreLine + x * sizeof(QRgb));
        const QRgb* end = p + bbox.width();
        uchar* b = (dest_data + y * width + x);
        uchar* mask = (transMask + y * width + x);
#define DITHER(__p, __d, __m) ((uchar) ((((256 * (__m) + (__m) + 1)) * (__p) + (__d)) >> 16))
        while (p < end) {
            if (*mask == 0) {
                *b++ = TransIndex;
            }
            else {
                uint d = qt_bayer_matrix[y & 15][x & 15] << 8;
                *b =
                    INDEXOF(
                        DITHER(qRed(*p), d, MAX_R),
                        DITHER(qGreen(*p), d, MAX_G),
                        DITHER(qBlue(*p), d, MAX_B)
                    );
                b++;
            }
            p++;
            x++;
            mask++;
        }
#undef DITHER
    }

}

ColorTableState GetImageColorTable(const QImage& image,
    uint8_t* indexData,
    struct ColorTableCache *cTable,
    QRect bbox)
{
    int bytesPrePixel = image.depth() / 8;
    int tblIndex = cTable->cacheMap.size();
    uint8_t* imgData = (uint8_t*)image.constBits();
    ColorTableState retVal = TABLENOTCHANGE;
    if (bbox.isEmpty()) {
        bbox.setRect(0, 0, image.width(), image.height());
    }
    for (size_t y = bbox.y(); y < bbox.y() + bbox.height(); y++)
    {
        QRgb* linePtr = (QRgb*)(imgData + y * image.bytesPerLine() + bbox.x() * bytesPrePixel);
        uint8_t* indexPtr = indexData + y * image.width() + bbox.x();
        for (size_t x = bbox.x(); x < bbox.x() + bbox.width(); x++)
        {
            QRgb color = (*linePtr) & 0xf0f0f0f0;
            auto colorIt = cTable->cacheMap.find(color);
            if (colorIt == cTable->cacheMap.end()) {
                if (tblIndex >= 255)
                    return TABLEOVERFLOW;
                cTable->table[tblIndex].r = qRed(color);
                cTable->table[tblIndex].g = qGreen(color);
                cTable->table[tblIndex].b = qBlue(color);
                cTable->cacheMap.insert(color, tblIndex);
                *indexPtr = tblIndex;
                tblIndex++;
                retVal = TABLECHANGED;
            }
            else {
                *indexPtr = colorIt.value();
            }

            linePtr = (QRgb*)((uint8_t *)linePtr + bytesPrePixel);
            indexPtr++;
        }
    }
    return retVal;
}

static DitherFunc DifferFuncList[] = {
    OrderedDither,
    ThresholdDither,
    DiffuseDither
};

void Dither(int type, uint8_t* dest_data, uint8_t* src_data, 
    uint8_t* transMask, int srcBytesPreLine, 
    int width, int height, QRect bbox)
{
    const int parallelCnt = omp_get_num_procs();
    if (parallelCnt == 1 || bbox.height() <= parallelCnt * 4) {
        DifferFuncList[type](dest_data, src_data, transMask, srcBytesPreLine, width, height, bbox);
        return;
    }
    int pi;
#pragma omp parallel for 
    for (pi = 0; pi < parallelCnt; pi++)
    {
        QRect pbox(bbox.x(), bbox.y() + bbox.height() / parallelCnt * pi,
            bbox.width(), bbox.height() / parallelCnt);
        if (pi == parallelCnt - 1 && bbox.height() % parallelCnt > 0) {
            pbox.setHeight(pbox.height() + bbox.height() % parallelCnt);
        }
        DifferFuncList[type](dest_data, src_data, transMask, srcBytesPreLine, width, height, pbox);
    }
}
