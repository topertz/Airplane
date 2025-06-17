#include "widget.h"
#include "./ui_widget.h"
#include "passenger.h"
#include "database.h"
#include "draw.h"
#include <QPainter>
#include <QRandomGenerator>
#include <QPushButton>
#include <QMessageBox>
#include <QTimer>
#include <QTime>
#include <QList>
#include <QFont>
#include <QFile>
#include <QDateTime>
#include <QQueue>
#include <QHash>
#include <QColor>
#include <QPoint>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QRegularExpression>
#include <QStyledItemDelegate>
#include <QFileDialog>
#include <QSqlTableModel>
#include <QSqlQueryModel>
#include <QHeaderView>

Widget::Widget(QWidget *parent)
    : QWidget(parent),
    ui(new Ui::Widget),
    passenger(nullptr),
    database(nullptr),
    draw(nullptr),
    timer(new QTimer(this))
{
    ui->setupUi(this);
    setWindowTitle("Airplane Evacuation Simulation");
    this->resize(1500, 700);
    passenger = new Passenger(this);
    database = new Database(this);
    draw = new Draw(this);

    rowCount = 10;  // 10 rows
    colCount = 7;   // 2x3 chairs
    passengers.clear();

    connect(ui->drawGridButton, &QPushButton::clicked, this, [this]() {
        shouldDrawGrid = true; // Let's enable the drawing of the grid
        repaint();
    });

    std::vector<std::vector<int>> optimalArrangement(rowCount, std::vector<int>(colCount, 0));

    std::vector<std::vector<int>> arrangement(rowCount, std::vector<int>(colCount, 0));
    bool isBestSimulation = false;
    QList<QPoint> passengerPositions = passenger->getPassengers();
    connect(ui->generateButton, &QPushButton::clicked, this, [this, arrangement, isBestSimulation, passengerPositions]() mutable {
        passenger->initializePassengers(arrangement, isBestSimulation, passengerPositions);
    });
    connect(ui->startButton, &QPushButton::clicked, passenger, &Passenger::startEvacuation);
    connect(ui->stopButton, &QPushButton::clicked, passenger, &Passenger::stopTimer);
    connect(ui->saveInitialStateButton, &QPushButton::clicked, database, &Database::saveInitialState);
    connect(ui->loadInitialStateButton, &QPushButton::clicked, database, &Database::loadInitialState);
    connect(ui->saveInitialStateSQLButton, &QPushButton::clicked, database, &Database::saveInitialStateToDatabase);
    connect(ui->loadInitialStateSQLButton, &QPushButton::clicked, database, &Database::loadInitialStateFromDatabase);
    connect(ui->findOptimalButton, &QPushButton::clicked, database, &Database::findOptimalSeatingArrangement);
    connect(ui->loadOptimalSeatingArrangementSQLButton, &QPushButton::clicked, database, &Database::loadOptimalSeatingArrangementFromDatabase);
    connect(ui->saveOptimalSeatingArrangementSQLButton, &QPushButton::clicked, database, &Database::saveOptimalSeatingArrangementToDatabase);
    connect(ui->showOptimalButton, &QPushButton::clicked, database, &Database::showOptimalDatabaseTable);
    connect(ui->viewStatsButton, &QPushButton::clicked, this, [this]() {
        database->showDatabaseTables("onboard_state_stats", "disembarked_state_stats");
    });
    connect(ui->clearTextbox, &QPushButton::clicked, this, &Widget::clearTextbox);
    connect(ui->spinBox, &QSpinBox::valueChanged, this, &Widget::spinboxChanged);
    connect(ui->stepButton, &QPushButton::clicked, passenger, &Passenger::movePassengersStepByStep);

    ui->drawGridButton->setStyleSheet("background-color: #f0ed69;");
    ui->generateButton->setStyleSheet("background-color: #f0ed69;");
    ui->startButton->setStyleSheet("background-color: #66f549;");
    ui->stopButton->setStyleSheet("background-color: #f28f68;");
    ui->stepButton->setStyleSheet("background-color: #3cdbf0;");

    ui->loadInitialStateButton->setStyleSheet("background-color: #f598ef;");
    ui->saveInitialStateButton->setStyleSheet("background-color: #f598ef;");
    ui->loadInitialStateSQLButton->setStyleSheet("background-color: #f598ef;");
    ui->saveInitialStateSQLButton->setStyleSheet("background-color: #f598ef;");

    ui->findOptimalButton->setStyleSheet("background-color: #b8f5d1;");
    ui->viewStatsButton->setStyleSheet("background-color: #b8f5d1;");
    ui->loadOptimalSeatingArrangementSQLButton->setStyleSheet("background-color: #b8f5d1;");
    ui->saveOptimalSeatingArrangementSQLButton->setStyleSheet("background-color: #b8f5d1;");

    ui->clearTextbox->setStyleSheet("background-color: #b1db3d;");
    ui->showOptimalButton->setStyleSheet("background-color: #b1db3d;");
}

Widget::~Widget()
{
    delete ui;
}

void Widget::spinboxChanged(int value) {
    passenger->passengerCount = value;
}

void Widget::paintEvent(QPaintEvent *event)
{
    if (!shouldDrawGrid) {
        return; // If grid drawing is not enabled, we do nothing
    }

    QPainter painter(this);
    int rectSize = 30;
    int offsetX = 180;
    int offsetY = 140;

    // Let's draw the interior model of the airplane (chairs and aisle)
    for (int i = 0; i < rowCount; i++) {
        for (int j = 0; j < colCount; j++) {
            QRect rect(j * rectSize + offsetX, i * rectSize + offsetY, rectSize, rectSize);
            if (i % 2 == 0 && (j < 3 || j > 3)) {
                painter.setBrush(Qt::lightGray);  // Székek színe
            } else {
                painter.setBrush(Qt::white);  // Folyosó színe
            }
            painter.setPen(Qt::black);
            painter.drawRect(rect);

            QString text;
            if (passenger->numbers[i][j] == INT_MAX) {
                text = "";
            } else {
                text = QString::number(passenger->numbers[i][j]);
            }
            painter.setPen(Qt::red);
            painter.setFont(QFont("Arial", 8));
            painter.drawText(rect, Qt::AlignBottom | Qt::AlignRight, text);
        }
    }

    // Show more passengers
    for (const auto& passengerObj : passengers) {
        QRect rect(passengerObj.y() * rectSize + offsetX, passengerObj.x() * rectSize + offsetY, rectSize, rectSize);
        painter.setBrush(Qt::yellow);  // The color of the passenger's current position
        painter.setPen(Qt::black);
        painter.drawRect(rect);
        painter.setBrush(Qt::blue);  // Passenger color
        painter.drawEllipse(rect.center(), rectSize / 4, rectSize / 4);

        // We will find the passenger's nearest exit
        QPoint closestExit = passenger->findClosestExit(passengerObj);

        // Draw an arrow in the direction of the exit
        draw->drawArrow(painter, passengerObj, closestExit, rectSize, offsetX, offsetY);
    }

    // Drawing side wings
    int wingWidth = 30;
    int wingHeight = 120;

    // Left wing
    QPoint wingLeftTip(offsetX - wingWidth - 130, offsetY + (rowCount * rectSize) / 2 - 170);
    QPoint wingLeftTop(offsetX, offsetY + (rowCount * rectSize) / 2 - wingHeight / 2);
    QPoint wingLeftBottom(offsetX, offsetY + (rowCount * rectSize) / 2 + wingHeight / 2);

    painter.setBrush(Qt::darkGray);
    painter.setPen(Qt::black);
    painter.drawPolygon(QPolygon({ wingLeftTip, wingLeftTop, wingLeftBottom }));

    // Right wing
    QPoint wingRightTip(offsetX + colCount * rectSize + wingWidth + 130, offsetY + (rowCount * rectSize) / 2 - 170);
    QPoint wingRightTop(offsetX + colCount * rectSize, offsetY + (rowCount * rectSize) / 2 - wingHeight / 2);
    QPoint wingRightBottom(offsetX + colCount * rectSize, offsetY + (rowCount * rectSize) / 2 + wingHeight / 2);

    painter.drawPolygon(QPolygon({ wingRightTip, wingRightTop, wingRightBottom }));

    // Front of an airplane (with half-circle propeller)
    int noseDiameter = rectSize * 7; // Semicircle diameter (210 pixels)
    int noseRadius = noseDiameter / 2;
    QPoint noseCenter(offsetX - noseRadius + 105, offsetY + (rowCount * rectSize) / 2 + 148); // The center of a semicircle
    QRect noseRect(noseCenter.x(), noseCenter.y() - noseRadius, noseDiameter, noseDiameter);

    // Draw a semicircle
    painter.setBrush(Qt::lightGray);
    painter.setPen(Qt::black);
    painter.drawPie(noseRect, 180 * 16, 180 * 16);

    // Drawing a propeller on the semicircle
    QPoint propellerCenter(noseCenter.x() + noseRadius + 0.5, noseCenter.y() + 103); // The center of the propeller
    draw->drawPropeller(painter, propellerCenter, 30);

    // Drawing back wings
    draw->drawRearWings(painter, offsetX, offsetY, rowCount, colCount, rectSize);
}

void Widget::clearTextbox() {
    ui->textLoadingData->clear();
}
