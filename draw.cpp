#include "draw.h"
#include "widget.h"
#include <QPainter>

Draw::Draw(Widget *parentWidget, QWidget *parent) : QWidget(parent), widget(parentWidget), passenger(nullptr) {}

// Drawing a propeller
void Draw::drawPropeller(QPainter& painter, const QPoint& center, int radius)
{
    painter.setBrush(Qt::lightGray);
    painter.setPen(Qt::black);

    // Four propeller blades
    for (int i = 0; i < 4; i++) {
        int angle = i * 90; // Angle of rotation
        QPoint start = center + QPoint(radius * cos(qDegreesToRadians(angle)), radius * sin(qDegreesToRadians(angle)));
        QPoint end = center + QPoint(2 * radius * cos(qDegreesToRadians(angle)), 2 * radius * sin(qDegreesToRadians(angle)));
        painter.drawLine(start, end);
    }

    // Propeller center
    painter.setBrush(Qt::black);
    painter.drawEllipse(center, radius / 4, radius / 4);
}

void Draw::drawArrow(QPainter& painter, const QPoint& start, const QPoint& end, int rectSize, int offsetX, int offsetY)
{
    // We draw an arrow between the start and end points
    QLine arrowLine(start.y() * rectSize + offsetX + rectSize / 2, start.x() * rectSize + offsetY + rectSize / 2,
                    end.y() * rectSize + offsetX + rectSize / 2, end.x() * rectSize + offsetY + rectSize / 2);
    painter.setPen(Qt::red);
    painter.drawLine(arrowLine);

    // Arrowhead (arrow drawing)
    QPolygon arrowHead;
    arrowHead << QPoint(end.y() * rectSize + offsetX, end.x() * rectSize + offsetY);
    arrowHead << QPoint(end.y() * rectSize + offsetX - 5, end.x() * rectSize + offsetY - 5);
    arrowHead << QPoint(end.y() * rectSize + offsetX + 5, end.x() * rectSize + offsetY - 5);
    painter.setBrush(Qt::red);
    painter.drawPolygon(arrowHead);
}

void Draw::drawRearWings(QPainter& painter, int offsetX, int offsetY, int rowCount, int colCount, int rectSize)
{
    int rect4CornerX = offsetX + 3 * rectSize;
    int rect4CornerY = offsetY + 3 * rectSize - 90;

    // Longer and slanted triangle pointing left, starting one square further left
    QPoint triangleLeftBase(rect4CornerX + rectSize / 2, rect4CornerY);
    QPoint triangleLeftTip(rect4CornerX - 150, rect4CornerY - 120);
    QPoint triangleLeftBottom(offsetX, offsetY);

    painter.setBrush(Qt::darkGray);
    painter.setPen(Qt::black);
    painter.drawPolygon(QPolygon({ triangleLeftBase, triangleLeftTip, triangleLeftBottom }));

    // Longer and slanted triangle pointing right
    QPoint triangleRightBase(rect4CornerX + rectSize - rectSize / 2, rect4CornerY);
    QPoint triangleRightTip(rect4CornerX + 150 + rectSize, rect4CornerY - 120);
    QPoint triangleRightBottom(offsetX + 7 * rectSize + rectSize, offsetY);

    // Adjust the rightmost point to move it one square back to the left
    triangleRightBottom.setX(triangleRightBottom.x() - rectSize);

    painter.drawPolygon(QPolygon({ triangleRightBase, triangleRightTip, triangleRightBottom }));
}
