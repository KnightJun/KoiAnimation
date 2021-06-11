#pragma once

#include <QImage>
#include <QMap>
#define TransIndex 0xff

struct RGB
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};
struct TableStat
{
    uint64_t cnt;
    uint64_t R;
    uint64_t G;
    uint64_t B;
};
struct ColorTableCache
{
    struct RGB table[256];
    QMap<uint32_t, int> cacheMap;
};
enum ColorTableState
{
    TABLEOVERFLOW,
    TABLENOTCHANGE,
    TABLECHANGED
};

const uint8_t* GetCube216ColorTable();

void Dither(int type, uint8_t* dest_data, uint8_t* src_data,
    uint8_t* transMask,
    int srcBytesPreLine, int width, int height,
    QRect bbox = QRect());

ColorTableState GetImageColorTable(const QImage& image,
    uint8_t* indexData,
    struct ColorTableCache* cTable,
    QRect bbox = QRect());

bool remap_to_palette_floyd(const QImage& input_image,
    uint8_t* const output_index, const float max_dither_error);