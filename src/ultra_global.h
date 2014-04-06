#ifndef ULTRA_GLOBAL_H
#define ULTRA_GLOBAL_H

#include <QtCore/qglobal.h>

#ifdef ULTRA_SHARED
#  define ULTRA_EXPORT Q_DECL_EXPORT
#else
#  define ULTRA_EXPORT Q_DECL_IMPORT
#endif

#endif // ULTRA_GLOBAL_H
