#pragma once

namespace KoiGifOpt
{	
    /* 0:GloPalette, 1:OcTreePalette */
    static const char *_opt_PattleType = "OptGifPattleType";  

    /* 0:Ordered, 1:Threshold, 2:Diffuse*/
    static const char *_opt_DitherType = "OptGifDitherType";  

    enum PlayLoopEnum{
        Infinite,
        Once
    };
    static const char *_opt_PlayLoop = "OptGifPlayLoop";
} // namespace Koi
