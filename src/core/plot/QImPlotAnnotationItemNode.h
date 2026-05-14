#ifndef QIMPLOTANNOTATIONITEMNODE_H
#define QIMPLOTANNOTATIONITEMNODE_H

#include <QColor>
#include <QPointF>
#include <QString>
#include <Qt>
#include <QVector>
#include "QImPlotItemNode.h"

namespace QIM
{

class QIM_CORE_API QImPlotAnnotationItemNode : public QImPlotItemNode
{
    Q_OBJECT
    QIM_DECLARE_PRIVATE(QImPlotAnnotationItemNode)

    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor NOTIFY fillColorChanged)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)
    Q_PROPERTY(int lineStyle READ lineStyle WRITE setLineStyle NOTIFY lineStyleChanged)
    Q_PROPERTY(float lineWidth READ lineWidth WRITE setLineWidth NOTIFY lineWidthChanged)
    Q_PROPERTY(float handleSize READ handleSize WRITE setHandleSize NOTIFY handleSizeChanged)
    Q_PROPERTY(bool editable READ isEditable WRITE setEditable NOTIFY editableChanged)
    Q_PROPERTY(bool filled READ isFilled WRITE setFilled NOTIFY filledChanged)
    Q_PROPERTY(int selectedAnnotationIndex READ selectedAnnotationIndex WRITE setSelectedAnnotationIndex NOTIFY
                   selectedAnnotationChanged)

public:
    enum
    {
        Type = InnerType + 10
    };

    enum class AnnotationType
    {
        Circle,
        Square,
        Triangle,
        Arrow,
        TextArrow,
        DoubleArrow,
        DoubleTextArrow
    };
    Q_ENUM(AnnotationType)

    struct Annotation
    {
        AnnotationType type { AnnotationType::Circle };
        QPointF p1;
        QPointF p2;
        QString text;
    };

    explicit QImPlotAnnotationItemNode(QObject* parent = nullptr);
    ~QImPlotAnnotationItemNode() override;

    int type() const override
    {
        return Type;
    }

    void setAnnotations(const QVector< Annotation >& annotations);
    QVector< Annotation > annotations() const;
    int annotationCount() const;
    Annotation annotationAt(int index) const;
    void setAnnotationAt(int index, const Annotation& annotation);
    int addAnnotation(const Annotation& annotation);
    void clearAnnotations();

    int addCircle(const QPointF& center, const QPointF& radiusHandle, const QString& text = QString());
    int addSquare(const QPointF& cornerA, const QPointF& cornerB, const QString& text = QString());
    int addTriangle(const QPointF& center, const QPointF& sizeHandle, const QString& text = QString());
    int addArrow(const QPointF& from, const QPointF& to, const QString& text = QString());
    int addTextArrow(const QPointF& from, const QPointF& to, const QString& text);
    int addDoubleArrow(const QPointF& from, const QPointF& to, const QString& text = QString());
    int addDoubleTextArrow(const QPointF& from, const QPointF& to, const QString& text);

    void setColor(const QColor& color);
    QColor color() const;

    void setFillColor(const QColor& color);
    QColor fillColor() const;

    void setTextColor(const QColor& color);
    QColor textColor() const;

    void setLineStyle(int style);
    int lineStyle() const;

    void setLineWidth(float width);
    float lineWidth() const;

    void setHandleSize(float size);
    float handleSize() const;

    void setEditable(bool editable);
    bool isEditable() const;

    void setFilled(bool filled);
    bool isFilled() const;

    void setSelectedAnnotationIndex(int index);
    int selectedAnnotationIndex() const;
    void clearSelection();

Q_SIGNALS:
    void annotationsChanged();
    void annotationEdited(int index);
    void selectedAnnotationChanged(int index);
    void colorChanged(const QColor& color);
    void fillColorChanged(const QColor& color);
    void textColorChanged(const QColor& color);
    void lineStyleChanged(int style);
    void lineWidthChanged(float width);
    void handleSizeChanged(float size);
    void editableChanged(bool editable);
    void filledChanged(bool filled);

protected:
    bool beginDraw() override;
};

}  // namespace QIM

Q_DECLARE_METATYPE(QIM::QImPlotAnnotationItemNode::Annotation)

#endif  // QIMPLOTANNOTATIONITEMNODE_H
