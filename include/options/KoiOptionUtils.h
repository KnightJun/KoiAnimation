#pragma once
#include <QSettings>

# if defined(KOIANIMATION_LIB)
extern QSettings *KoiOptions;
#endif
#ifndef opt_bool
#define opt_bool(KEY)    (KoiOptions->value(_opt_##KEY).toBool())
#define opt_bool_def(KEY, DEF)    (KoiOptions->value(_opt_##KEY, DEF).toBool())
#define opt_int(KEY)    (KoiOptions->value(_opt_##KEY).toInt())
#define opt_int_def(KEY, DEF)    (KoiOptions->value(_opt_##KEY, DEF).toInt())
#define opt_float(KEY)    (KoiOptions->value(_opt_##KEY).toFloat())
#define opt_float_def(KEY, DEF)    (KoiOptions->value(_opt_##KEY, DEF).toFloat())
#define opt_SetVal(KEY, VAL) (KoiOptions->setValue(_opt_##KEY, VAL))
#define opt_contains(KEY)       (KoiOptions->contains(_opt_##KEY))
#define opt_SetDefaultVal(KEY, VAL) {if(!KoiOptions->contains(_opt_##KEY))KoiOptions->setValue(_opt_##KEY, VAL);}
#endif

enum EncodeSeting
{
	EncodeGloPalette,
	EncodeOcTreePalette,
};

enum DitherType
{
    Ordered = 0,
    Threshold,
    Diffuse
};

#include "GifEncoderOpt.h"
#include "APngEncoderOpt.h"
#include "WebpEncoderOpt.h"
