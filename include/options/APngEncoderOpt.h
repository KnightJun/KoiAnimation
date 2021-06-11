#pragma once

namespace KoiAPngOpt
{
    enum ColorTypeEnum{
        TrueColor,
        Pattle
    };
    static const char *_opt_ColorType = "OptAPngColorType";  
    
    /*(0 - no compression, 9 - "maximal" compression)*/
    static const char *_opt_CompressLevel = "OptAPngCompressLevel";  

} // namespace Koi
