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

public slots:
    void addUnit(IoTropolisUnitConnection* unit);
    void removeUnit(IoTropolisUnitConnection* unit);

private:
    QTableWidget* unitTable;

    // Helper to find a row by UnitID
    int findRowByUnitID(UnitID id) const;
};

#endif // IOTROPOLISGUI_H
