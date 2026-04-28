#ifndef QIMPLOTHIGHLIGHTRULE_H
#define QIMPLOTHIGHLIGHTRULE_H

#include "QImAPI.h"
#include <algorithm>
#include <QColor>

namespace QIM
{
class QIM_CORE_API QImPlotHighlightRule
{
public:
    enum class Axis
    {
        X,
        Y
    };

    enum class Mode
    {
        Between,
        LessThan,
        GreaterThan
    };

    bool enabled { false };
    Axis axis { Axis::X };
    Mode mode { Mode::Between };
    double minValue { 0.0 };
    double maxValue { 0.0 };
    QColor color { Qt::red };

    bool operator==(const QImPlotHighlightRule& other) const
    {
        return enabled == other.enabled &&
               axis == other.axis &&
               mode == other.mode &&
               minValue == other.minValue &&
               maxValue == other.maxValue &&
               color == other.color;
    }

    bool operator!=(const QImPlotHighlightRule& other) const
    {
        return !(*this == other);
    }

    QImPlotHighlightRule normalized() const
    {
        QImPlotHighlightRule result = *this;
        if (result.mode == Mode::Between && result.minValue > result.maxValue) {
            std::swap(result.minValue, result.maxValue);
        }
        return result;
    }

    bool matches(double x, double y) const
    {
        if (!enabled) {
            return false;
        }
        const QImPlotHighlightRule rule = normalized();
        const double value = rule.axis == Axis::X ? x : y;
        switch (rule.mode) {
        case Mode::Between:
            return value >= rule.minValue && value <= rule.maxValue;
        case Mode::LessThan:
            return value < rule.maxValue;
        case Mode::GreaterThan:
            return value > rule.minValue;
        }
        return false;
    }
};
}  // namespace QIM

#endif  // QIMPLOTHIGHLIGHTRULE_H
