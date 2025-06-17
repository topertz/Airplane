#include "ui_widget.h"
#include "passenger.h"
#include "database.h"
#include <QTime>
#include <QRandomGenerator>
#include <QMessageBox>
#include <numeric>

Passenger::Passenger(Widget *parentWidget, QObject *parent)
    : QObject(parent), widget(parentWidget), ui(parentWidget->ui), timer(new QTimer(this)), rowCount(10), colCount(7) {
    passengers = &parentWidget->passengers;
    connect(timer, &QTimer::timeout, this, &Passenger::movePassengers);
    for (int i = 0; i < rowCount; ++i) {
        for (int j = 0; j < colCount; ++j) {
            numbers[i][j] = INT_MIN;
            obstacle[i][j] = false;
        }
    }
    position = QPoint(exitRow, exitCol);
    for (int i = 0; i < rowCount; i++) {
        for (int j = 0; j < colCount; j++) {
            if (i % 2 == 0 && (j < 3 || j > 3)) {
                obstacle[i][j] = true;
            }
        }
    }
    RecursiveMethod(position, -1);
}

Passenger::~Passenger()
{
    delete ui;
}

QList<QPoint> Passenger::getPassengers() {
    return *passengers;
}

int Passenger::getPassengerCount() {
    return passengers->size();
}

void Passenger::RecursiveMethod(QPoint position, int val) {
    if (obstacle[position.x()][position.y()]) {
        numbers[position.x()][position.y()] = INT_MAX;
        return;
    } else {
        numbers[position.x()][position.y()] = val+1;
    }

    QList<QPoint> XY;
    XY.append(QPoint(position.x() + 1, position.y()));
    XY.append(QPoint(position.x() - 1, position.y()));
    XY.append(QPoint(position.x(), position.y() + 1));
    XY.append(QPoint(position.x(), position.y() - 1));

    for (const QPoint& p : XY) {
        if (p.x() >= 0 && p.x() < 10 && p.y() >= 0 && p.y() < 7) {
            if (numbers[p.x()][p.y()] < 0) {
                RecursiveMethod(p, numbers[position.x()][position.y()]);
            }
        }
    }
}

void Passenger::stopTimer()
{
    if (timer->isActive()) {
        timer->stop();  // We stop the movement
    }
}

void Passenger::resetStatistics() {
    onboardPassengers = 0;
    onboardSteps = 0;
    onboardWaits = 0;
    onboardAvgWaitTime = 0.0;
    disembarkedPassengers = 0;
    disembarkedAvgYourney = 0.0;
    disembarkedAvgWaitTime = 0.0;
    disembarkedWaits = 0;
    totalTime = 0;
    widget->ui->onBoardPassengersLabel->setText(QString("Onboard Passengers: %1").arg(onboardPassengers));
    widget->ui->onBoardStepsLabel->setText(QString("Onboard Steps: %1").arg(onboardSteps));
    widget->ui->onBoardWaitsLabel->setText(QString("Onboard Waits: %1").arg(onboardWaits));
    widget->ui->onBoardAverageWaitTimeLabel->setText(QString("Onboard Avg Wait Time: %1").arg(onboardAvgWaitTime));
    widget->ui->disembarkedPassengersLabel->setText(QString("Disembarked Passengers: %1").arg(disembarkedPassengers));
    widget->ui->disembarkedAvgYourneyLabel->setText(QString("Disembarked Avg Yourney: %1").arg(disembarkedAvgYourney));
    widget->ui->disembarkedAvgWaitTimeLabel->setText(QString("Disembarked Avg Wait Time: %1").arg(disembarkedAvgWaitTime));
    widget->ui->totalLabel->setText(QString("Total Time: %1").arg(totalTime));
}

void Passenger::updateStatisticsLabels() {
    // We update the labels with the current statistics
    QString onboardPassengersText = QString("Onboard Passengers: %1").arg(onboardPassengers);
    QString onboardStepsText = QString("Onboard Steps: %1").arg(onboardSteps);
    QString onboardWaitsText = QString("Onboard Waits: %1").arg(onboardWaits);
    QString onboardAvgWaitText = QString("Onboard Avg Wait Time: %1 sec").arg(onboardAvgWaitTime, 0, 'f', 2);
    widget->ui->onBoardAverageWaitTimeLabel->setText(onboardAvgWaitText);

    QString disembarkedPassengersText = QString("Disembarked Passengers: %1").arg(disembarkedPassengers);
    QString disembarkedAvgYourneyText = QString("Disembarked Avg Yourney: %1").arg(disembarkedAvgYourney);
    QString disembarkedAvgWaitText = QString("Disembarked Avg Wait Time: %1 sec").arg(disembarkedAvgWaitTime, 0, 'f', 2);
    widget->ui->disembarkedAvgWaitTimeLabel->setText(disembarkedAvgWaitText);

    QString totalTimeText = QString("Total Time: %1 sec").arg(totalTime);

    widget->ui->onBoardPassengersLabel->setText(onboardPassengersText);
    widget->ui->onBoardStepsLabel->setText(onboardStepsText);
    widget->ui->onBoardWaitsLabel->setText(onboardWaitsText);
    widget->ui->onBoardAverageWaitTimeLabel->setText(onboardAvgWaitText);

    widget->ui->disembarkedPassengersLabel->setText(disembarkedPassengersText);
    widget->ui->disembarkedAvgYourneyLabel->setText(disembarkedAvgYourneyText);
    widget->ui->disembarkedAvgWaitTimeLabel->setText(disembarkedAvgWaitText);

    widget->ui->totalLabel->setText(totalTimeText);
}

void Passenger::startEvacuation()
{
    if (!timer->isActive()) {
        //resetStatistics();
        passengerSteps.clear();
        for(int i=0; i<passengerCount; i++) {
            passengerSteps.append(0);
        }
        timer->start(1000);  // We restart the timer
        movePassengers();    // We will start moving the passengers
    }
}

void Passenger::initializePassengers(const std::vector<std::vector<int>>& arrangement, bool isBestSimulation, const QList<QPoint>& passengerPositions) {
    resetStatistics();
    passengers->clear();

    // If passengers are not yet initialized, create a new list
    if (!passengers) {
        passengers = new QList<QPoint>();
    }

    // Determination of valid positions
    QList<QPoint> validPositions;

    for (int i = 0; i < rowCount; ++i) {  // 10 rows
        for (int j = 0; j < colCount; ++j) {  // 7 cols
            if (i % 2 != 0 && j != 3 && !(i == 9)) {
                validPositions.append(QPoint(i, j));
            }
        }
    }

    // If there is no valid position, the process stops
    if (validPositions.isEmpty()) {
        qWarning() << "No valid positions available.";
        return;
    }

    QSet<QPoint> usedPositions;

    // If there is a specified passengerPositions list, we use it
    if (!passengerPositions.isEmpty()) {
        for (const QPoint& pos : passengerPositions) {
            if (validPositions.contains(pos) && !usedPositions.contains(pos)) {
                passengers->append(pos);
                usedPositions.insert(pos);
            }
        }
    } else {
        // We add random positions until we reach a passenger size
        while (passengers->size() < passengerCount) {
            QPoint additionalPos = validPositions[QRandomGenerator::global()->bounded(validPositions.size())];
            if (!usedPositions.contains(additionalPos)) {
                passengers->append(additionalPos);
                usedPositions.insert(additionalPos);
            }
        }
    }

    // If there are less than passengerCount passengers, a warning
    if (passengers->size() < passengerCount) {
        qWarning() << "Not enough passengers added. Total passengers: " << passengers->size();
    }

    // We will update the widget when available
    if (widget) {
        widget->update();
    } else {
        qWarning() << "widget is null.";
    }
}

void Passenger::movePassengers() {
    iterationCount++;

    timer->start(1000);  // Start or restart the timer
    if (passengers->isEmpty()) {
        return;
    }

    QList<QPoint> updatedPositions;
    QPoint fixedExit(exitRow, exitCol);  // Exit: row 9, column 6
    QList<QPoint> toDisembark;

    for (int passengerIndex = 0; passengerIndex < passengers->length(); passengerIndex++) {
        QPoint passenger = (*passengers)[passengerIndex];
        // If the passenger has reached the exit, handle disembarkation or wait
        if (passenger == fixedExit) {
            toDisembark.append(passenger);
            continue;
        }
    }

    // Remove passengers who disembarked from the list
    for (QPoint x : toDisembark) {
        int passengerIndex = passengers->indexOf(x);
        disembarkedPassengerSteps.append(passengerSteps[passengerIndex]);
        passengerSteps.removeAt(passengerIndex);
        passengers->removeOne(x);
        disembarkedPassengers++;
    }

    int counter = 0;
    QList<QPoint> nextStack;
    nextStack.append(fixedExit);
    // First, track passengers reaching the exit
    while(counter < passengers->length())
    {
        QPoint passenger = nextStack[0];
        nextStack.removeFirst();

        QList<QPoint> XY;
        XY.append(QPoint(passenger.x() + 1, passenger.y()));
        XY.append(QPoint(passenger.x(), passenger.y() + 1));
        XY.append(QPoint(passenger.x(), passenger.y() - 1));
        XY.append(QPoint(passenger.x() - 1, passenger.y()));

        for (const QPoint& p : XY) {
            if (p.x() >= 0 && p.x() < 10 && p.y() >= 0 && p.y() < 7) {
                if (numbers[p.x()][p.y()] != INT_MAX && numbers[p.x()][p.y()] > numbers[passenger.x()][passenger.y()]) {
                    nextStack.insert(0, p);
                }
            }
        }

        if (passengers->contains(passenger)) {
            counter++;
            int targetX = passenger.x();
            int targetY = passenger.y();
            int oldIndex = passengers->indexOf(passenger);

            QList<QPoint> possibleMoves;
            possibleMoves.append(QPoint(targetX + 1, targetY));
            possibleMoves.append(QPoint(targetX - 1, targetY));
            possibleMoves.append(QPoint(targetX, targetY + 1));
            possibleMoves.append(QPoint(targetX, targetY - 1));

            for (const QPoint& p : possibleMoves) {
                if (p.x() >= 0 && p.x() < 10 && p.y() >= 0 && p.y() < 7) {
                    if (numbers[targetX][targetY] > numbers[p.x()][p.y()]) {
                        targetX = p.x();
                        targetY = p.y();
                        break;
                    }
                }
            }

            QPoint target(targetX, targetY);
            // Add the planned move if possible
            if(!updatedPositions.contains(target)) {
                updatedPositions.append(target);
                onboardSteps++;
                passengerSteps[oldIndex]++;
            } else {
                updatedPositions.append(passenger);
                onboardWaits++;
            }
            int newIndex = updatedPositions.length()-1;
            int tmp = passengerSteps[oldIndex];
            passengerSteps[oldIndex] = passengerSteps[newIndex];
            passengerSteps[newIndex] = tmp;
        }
    }

    passengers->clear();
    for (const auto& pos : updatedPositions) {
        passengers->append(pos);
    }

    // Update onboard passengers count
    onboardPassengers = passengers->size();

    if (!passengers->isEmpty()) {
        onboardAvgWaitTime = (double)onboardWaits / (double)passengerCount;
    }

    // Calculate disembarked statistics
    if (disembarkedPassengers > 0) {
        disembarkedWaits += disembarkedPassengers;
        disembarkedAvgWaitTime = (double)disembarkedWaits / (double)disembarkedPassengers;
        disembarkedAvgYourney = (double)std::accumulate(disembarkedPassengerSteps.begin(), disembarkedPassengerSteps.end(), 0) / (double)disembarkedPassengers;
    }

    // Update the total evacuation time (number of iterations)
    totalTime = iterationCount;

    updateStatisticsLabels();  // Update UI labels

    // Check if all passengers have disembarked
    if (passengers->isEmpty()) {
        timer->stop();  // Stop the timer when evacuation is complete

        // Show the total evacuation time
        QMessageBox::information(widget, "Evacuation Complete",
                                 QString("All passengers have disembarked.\nTotal iterations: %1.\nTotal steps onboard: %2.")
                                     .arg(totalTime)
                                     .arg(onboardSteps));

        // Update the UI with the total evacuation time
        widget->ui->totalLabel->setText(QString("Total Iterations: %1\nOnboard Steps: %2").arg(totalTime).arg(onboardSteps));
        resetStatistics();
        iterationCount = 0;
    }

    widget->update();  // Redraw the widget
}

void Passenger::movePassengersStepByStep() {
    for(int i=0; i<passengerCount; i++) {
        passengerSteps.append(0);
    }
    iterationCount++;

    if (passengers->isEmpty()) {
        return;
    }

    QList<QPoint> updatedPositions;
    QPoint fixedExit(exitRow, exitCol);
    QList<QPoint> toDisembark;

    for (int passengerIndex = 0; passengerIndex < passengers->length(); passengerIndex++) {
        QPoint passenger = (*passengers)[passengerIndex];
        if (passenger == fixedExit) {
            toDisembark.append(passenger);
        }
    }

    for (QPoint x : toDisembark) {
        int passengerIndex = passengers->indexOf(x);
        if (passengerIndex != -1) {
            if (passengerIndex < passengerSteps.size()) {
                disembarkedPassengerSteps.append(passengerSteps[passengerIndex]);
                passengerSteps.removeAt(passengerIndex);
            }
            passengers->removeOne(x);
            disembarkedPassengers++;
        }
    }

    int counter = 0;
    QList<QPoint> nextStack;
    nextStack.append(fixedExit);

    while (counter < passengers->length()) {
        if (nextStack.isEmpty()) {
            break;
        }
        QPoint passenger = nextStack[0];
        nextStack.removeFirst();

        QList<QPoint> XY = {
            QPoint(passenger.x() + 1, passenger.y()),
            QPoint(passenger.x(), passenger.y() + 1),
            QPoint(passenger.x(), passenger.y() - 1),
            QPoint(passenger.x() - 1, passenger.y())
        };

        for (const QPoint& p : XY) {
            if (p.x() >= 0 && p.x() < 10 && p.y() >= 0 && p.y() < 7) {
                if (numbers[p.x()][p.y()] != INT_MAX && numbers[p.x()][p.y()] > numbers[passenger.x()][passenger.y()]) {
                    nextStack.insert(0, p);
                }
            }
        }

        if (passengers->contains(passenger)) {
            counter++;
            int oldIndex = passengers->indexOf(passenger);
            if (oldIndex == -1) continue;

            int targetX = passenger.x();
            int targetY = passenger.y();

            QList<QPoint> possibleMoves = {
                QPoint(targetX + 1, targetY),
                QPoint(targetX - 1, targetY),
                QPoint(targetX, targetY + 1),
                QPoint(targetX, targetY - 1)
            };

            for (const QPoint& p : possibleMoves) {
                if (p.x() >= 0 && p.x() < 10 && p.y() >= 0 && p.y() < 7) {
                    if (numbers[targetX][targetY] > numbers[p.x()][p.y()]) {
                        targetX = p.x();
                        targetY = p.y();
                        break;
                    }
                }
            }

            QPoint target(targetX, targetY);
            if (!updatedPositions.contains(target)) {
                updatedPositions.append(target);
                onboardSteps++;
                if (oldIndex < passengerSteps.size()) {
                    passengerSteps[oldIndex]++;
                }
            } else {
                updatedPositions.append(passenger);
                onboardWaits++;
            }
        }
    }

    passengers->clear();
    for (const auto& pos : updatedPositions) {
        passengers->append(pos);
    }

    onboardPassengers = passengers->size();

    if (!passengers->isEmpty()) {
        onboardAvgWaitTime = static_cast<double>(onboardWaits) / static_cast<double>(passengerCount);
    }

    if (disembarkedPassengers > 0) {
        disembarkedWaits += disembarkedPassengers;
        disembarkedAvgWaitTime = static_cast<double>(disembarkedWaits) / static_cast<double>(disembarkedPassengers);
        disembarkedAvgYourney = static_cast<double>(std::accumulate(disembarkedPassengerSteps.begin(), disembarkedPassengerSteps.end(), 0)) / static_cast<double>(disembarkedPassengers);
    }

    totalTime = iterationCount;
    updateStatisticsLabels();

    if (passengers->isEmpty()) {
        QMessageBox::information(widget, "Evacuation Completed",
                                 QString("All passengers have disembarked.\nTotal iterations: %1.\nTotal steps onboard: %2.")
                                     .arg(totalTime)
                                     .arg(onboardSteps));
        widget->ui->totalLabel->setText(QString("Total Iterations: %1\nOnboard Steps: %2").arg(totalTime).arg(onboardSteps));
        resetStatistics();
        iterationCount = 0;
    }

    widget->update();
}

bool Passenger::isCellWhite(int row, int col) {
    // We assume that the corridor starts from column 3
    if (row % 2 != 0 || col == 3) {
        return true;
    }
    return false;
}

QPoint Passenger::findClosestExit(const QPoint& passenger) {
    // Fixed exit: row 9 and column 6
    QPoint fixedExit(exitRow, exitCol);

    // We check whether the passenger is already at the exit
    if (passenger == fixedExit) {
        return fixedExit; // If the passenger is already at the exit, we return it
    }

    // In all cases, we return the fixed exit
    return fixedExit;
}
