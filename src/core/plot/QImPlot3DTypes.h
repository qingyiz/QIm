#ifndef QIMPLOT3DTYPES_H
#define QIMPLOT3DTYPES_H

#include "QImAPI.h"

namespace QIM
{
using QImPlot3DMarker = int;

enum QImPlot3DMarker_
{
    QImPlot3DMarker_None = -1,
    QImPlot3DMarker_Circle = 0,
    QImPlot3DMarker_Square,
    QImPlot3DMarker_Diamond,
    QImPlot3DMarker_Up,
    QImPlot3DMarker_Down,
    QImPlot3DMarker_Left,
    QImPlot3DMarker_Right,
    QImPlot3DMarker_Cross,
    QImPlot3DMarker_Plus,
    QImPlot3DMarker_Asterisk,
    QImPlot3DMarker_COUNT
};

using QImPlot3DColormap = int;

enum QImPlot3DColormap_
{
    QImPlot3DColormap_Deep = 0,
    QImPlot3DColormap_Dark = 1,
    QImPlot3DColormap_Pastel = 2,
    QImPlot3DColormap_Paired = 3,
    QImPlot3DColormap_Viridis = 4,
    QImPlot3DColormap_Plasma = 5,
    QImPlot3DColormap_Hot = 6,
    QImPlot3DColormap_Cool = 7,
    QImPlot3DColormap_Pink = 8,
    QImPlot3DColormap_Jet = 9,
    QImPlot3DColormap_Twilight = 10,
    QImPlot3DColormap_RdBu = 11,
    QImPlot3DColormap_BrBG = 12,
    QImPlot3DColormap_PiYG = 13,
    QImPlot3DColormap_Spectral = 14,
    QImPlot3DColormap_Greys = 15
};

struct QIM_CORE_API QImPlot3DPoint
{
    double x { 0.0 };
    double y { 0.0 };
    double z { 0.0 };
};
}  // namespace QIM

#endif  // QIMPLOT3DTYPES_H
