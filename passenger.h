#ifndef PASSENGER_H
#define PASSENGER_H

#include "widget.h"
#include <QWidget>
#include <QPoint>
#include <QList>
#include <QMap>
#include <QTime>
#include <QTimer>

class Passenger : public QObject
{
    Q_OBJECT

public:
    explicit Passenger(Widget *parentWidget, QObject *parent = nullptr);
    ~Passenger();
    Ui::Widget *ui;
    int onboardPassengers = 0;
    int onboardSteps = 0;
    int onboardWaits = 0;
    double onboardAvgWaitTime = 0.0f;
    int disembarkedPassengers = 0;
    double disembarkedAvgYourney = 0.0f;
    double disembarkedAvgWaitTime = 0.0f;
    int disembarkedWaits = 0;
    int totalTime = 0;
    int positionX;
    int positionY;
    QList<QPoint>* passengers;
    bool isInitialized = false;
    int passengerCount = 15;
    int numbers[10][7];
    bool obstacle[10][7];
    QPoint position;
    int iterationCount = 0;
    int exitRow = 9;
    int exitCol = 6;

public slots:
    void updateStatisticsLabels();
    void initializePassengers(const std::vector<std::vector<int>>& arrangement, bool isBestSimulation, const QList<QPoint>& passengerPositions);
    void startEvacuation();
    void stopTimer();
    void movePassengers();
    QPoint findClosestExit(const QPoint& passenger);
    void resetStatistics();
    QList<QPoint> getPassengers();
    int getPassengerCount();
    void RecursiveMethod(QPoint position, int val);
    void movePassengersStepByStep();

private slots:
    bool isCellWhite(int row, int col);

private:
    Widget *widget;
    Database *database;
    QTimer* timer;
    int rowCount = 0;
    int colCount = 0;
    QList<int> passengerSteps;
    QList<int> disembarkedPassengerSteps;
};


#endif // PASSENGER_H
