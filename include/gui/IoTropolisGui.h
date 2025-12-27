#ifndef IOTROPOLISGUI_H
#define IOTROPOLISGUI_H

#include <QMainWindow>
#include <QTableWidget>
#include "registration/IoTropolisUnitConnection.h"

class IoTropolisGui : public QMainWindow
{
    Q_OBJECT
public:
    explicit IoTropolisGui(QWidget *parent = nullptr);

public slots:
    void addUnit(IoTropolisUnitConnection* unit);
    void removeUnit(IoTropolisUnitConnection* unit);

private slots:
    void showAboutDialog();

private:
    QTableWidget* unitTable;

    int findRowByUnitID(UnitID id) const;
    void createMenus();
};

#endif // IOTROPOLISGUI_H
