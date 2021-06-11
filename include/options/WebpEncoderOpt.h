#pragma once

namespace KoiWebpOpt
{
    /* 0:lossy 1:lossless*/
    static const char *_opt_Lossless = "OptWebpLossless";  
    // between 0 and 100. For lossy, 0 gives the smallest
    // size and 100 the largest. For lossless, this
    // parameter is the amount of effort put into the
    // compression: 0 is the fastest but gives larger
    // files compared to the slowest, but best, 100.
    // quality/speed trade-off (0=fast, 6=slower-better)
    static const char *_opt_Quality = "OptWebpQuality";

    /* 0:single thread   1:multi thread*/
    static const char *_opt_MultiThread = "OptWebpMultiThread";  

    /* 0:none   1:minimize the output size (slow). Implicitly*/
    static const char *_opt_MinimizeSize = "OptWebpMinimizeSize";  
    
    // If true, use mixed compression mode; may choose
    // either lossy and lossless for each frame.
    static const char *_opt_AllowMixed = "OptWebpAllowMixed";  
    
} // namespace Koi
