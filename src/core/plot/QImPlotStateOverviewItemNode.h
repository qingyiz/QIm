#ifndef QIMPLOTSTATEOVERVIEWITEMNODE_H
#define QIMPLOTSTATEOVERVIEWITEMNODE_H

#include <QColor>
#include <QString>
#include <Qt>
#include <QVector>
#include "QImPlotItemNode.h"

namespace QIM
{

class QIM_CORE_API QImPlotStateOverviewItemNode : public QImPlotItemNode
{
    Q_OBJECT
    QIM_DECLARE_PRIVATE(QImPlotStateOverviewItemNode)

    Q_PROPERTY(double stableValue READ stableValue WRITE setStableValue NOTIFY geometryChanged)
    Q_PROPERTY(double unstableValue READ unstableValue WRITE setUnstableValue NOTIFY geometryChanged)
    Q_PROPERTY(QColor stableColor READ stableColor WRITE setStableColor NOTIFY stableColorChanged)
    Q_PROPERTY(QColor unstableColor READ unstableColor WRITE setUnstableColor NOTIFY unstableColorChanged)
    Q_PROPERTY(QColor boundaryColor READ boundaryColor WRITE setBoundaryColor NOTIFY boundaryColorChanged)
    Q_PROPERTY(QColor labelColor READ labelColor WRITE setLabelColor NOTIFY labelColorChanged)
    Q_PROPERTY(QColor caseLabelFillColor READ caseLabelFillColor WRITE setCaseLabelFillColor NOTIFY caseLabelFillColorChanged)
    Q_PROPERTY(QColor caseLabelBorderColor READ caseLabelBorderColor WRITE setCaseLabelBorderColor NOTIFY caseLabelBorderColorChanged)
    Q_PROPERTY(float lineWidth READ lineWidth WRITE setLineWidth NOTIFY lineWidthChanged)
    Q_PROPERTY(int stableLineStyle READ stableLineStyle WRITE setStableLineStyle NOTIFY lineStyleChanged)
    Q_PROPERTY(int unstableLineStyle READ unstableLineStyle WRITE setUnstableLineStyle NOTIFY lineStyleChanged)
    Q_PROPERTY(int boundaryLineStyle READ boundaryLineStyle WRITE setBoundaryLineStyle NOTIFY lineStyleChanged)
    Q_PROPERTY(bool boundaryLinesVisible READ areBoundaryLinesVisible WRITE setBoundaryLinesVisible NOTIFY boundaryLinesVisibilityChanged)
    Q_PROPERTY(bool stateLabelsVisible READ areStateLabelsVisible WRITE setStateLabelsVisible NOTIFY stateLabelsVisibilityChanged)
    Q_PROPERTY(bool caseLabelsVisible READ areCaseLabelsVisible WRITE setCaseLabelsVisible NOTIFY caseLabelsVisibilityChanged)

public:
    enum
    {
        Type = InnerType + 11
    };

    enum class State
    {
        Unstable = 0,
        Stable = 1
    };
    Q_ENUM(State)

    struct Segment
    {
        double x0 { 0.0 };
        double x1 { 1.0 };
        State state { State::Stable };
        QColor color;
        int lineStyle { -1 };  ///< -1 means use the node-level style for the segment state.
    };

    struct CaseLabel
    {
        double x0 { 0.0 };
        double x1 { 1.0 };
        QString text;
    };

    explicit QImPlotStateOverviewItemNode(QObject* parent = nullptr);
    ~QImPlotStateOverviewItemNode() override;

    int type() const override
    {
        return Type;
    }

    void setSegments(const QVector< Segment >& segments);
    QVector< Segment > segments() const;
    int addSegment(const Segment& segment);
    int addSegment(double x0, double x1, State state);
    int addStableSegment(double x0, double x1);
    int addUnstableSegment(double x0, double x1);
    void clearSegments();

    void setCaseLabels(const QVector< CaseLabel >& labels);
    QVector< CaseLabel > caseLabels() const;
    int addCaseLabel(double x0, double x1, const QString& text);
    void clearCaseLabels();

    void setStableValue(double value);
    double stableValue() const;
    void setUnstableValue(double value);
    double unstableValue() const;

    void setStableColor(const QColor& color);
    QColor stableColor() const;
    void setUnstableColor(const QColor& color);
    QColor unstableColor() const;
    void setBoundaryColor(const QColor& color);
    QColor boundaryColor() const;
    void setLabelColor(const QColor& color);
    QColor labelColor() const;
    void setCaseLabelFillColor(const QColor& color);
    QColor caseLabelFillColor() const;
    void setCaseLabelBorderColor(const QColor& color);
    QColor caseLabelBorderColor() const;

    void setLineWidth(float width);
    float lineWidth() const;
    void setStableLineStyle(int style);
    int stableLineStyle() const;
    void setUnstableLineStyle(int style);
    int unstableLineStyle() const;
    void setBoundaryLineStyle(int style);
    int boundaryLineStyle() const;

    void setBoundaryLinesVisible(bool visible);
    bool areBoundaryLinesVisible() const;
    void setStateLabelsVisible(bool visible);
    bool areStateLabelsVisible() const;
    void setCaseLabelsVisible(bool visible);
    bool areCaseLabelsVisible() const;

Q_SIGNALS:
    void segmentsChanged();
    void caseLabelsChanged();
    void geometryChanged();
    void stableColorChanged(const QColor& color);
    void unstableColorChanged(const QColor& color);
    void boundaryColorChanged(const QColor& color);
    void labelColorChanged(const QColor& color);
    void caseLabelFillColorChanged(const QColor& color);
    void caseLabelBorderColorChanged(const QColor& color);
    void lineWidthChanged(float width);
    void lineStyleChanged();
    void boundaryLinesVisibilityChanged(bool visible);
    void stateLabelsVisibilityChanged(bool visible);
    void caseLabelsVisibilityChanged(bool visible);

protected:
    bool beginDraw() override;
};

}  // namespace QIM

Q_DECLARE_METATYPE(QIM::QImPlotStateOverviewItemNode::Segment)
Q_DECLARE_METATYPE(QIM::QImPlotStateOverviewItemNode::CaseLabel)

#endif  // QIMPLOTSTATEOVERVIEWITEMNODE_H
