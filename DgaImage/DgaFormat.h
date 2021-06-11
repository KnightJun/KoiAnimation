#include <stdint.h>
struct DgaHeader
{
    uint16_t magicNum;
    uint8_t version;
    uint8_t padding8;

    uint32_t imgWidth;
    uint32_t imgHeight;
    int32_t frameCount;
};

enum DgaFrameOpt{
    OptCopy,
    OptXor
};

struct DgaFrame
{
    uint8_t  imgformat;
    uint8_t  opt; /* 0 src, 1 xor*/ 
    uint16_t padding16;
    
    uint32_t imgX;
    uint32_t imgY;
    uint32_t imgWidth;
    uint32_t imgHeight;
    uint32_t timeStamp;

    uint32_t dataLen;
};

