#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(KOIANIMATION_LIB)
#  define KOIANIMATION_EXPORT Q_DECL_EXPORT
# else
#  define KOIANIMATION_EXPORT Q_DECL_IMPORT
# endif
#else
# define KOIANIMATION_EXPORT
#endif
