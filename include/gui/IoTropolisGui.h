#ifndef IOTROPOLISGUI_H
#define IOTROPOLISGUI_H

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include "registration/IoTropolisUnitConnection.h"

class IoTropolisGui : public QWidget
{
    Q_OBJECT
public:
    explicit IoTropolisGui(QWidget *parent = nullptr);

    void addUnit(IoTropolisUnitConnection* unit);
    void removeUnit(IoTropolisUnitConnection* unit);

private:
    QTableWidget* unitTable;
};

#endif // IOTROPOLISGUI_H
