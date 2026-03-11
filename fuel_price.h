#ifndef FUEL_PRICE_H
#define FUEL_PRICE_H

#include <QDate>
#include <QString>

struct FuelPrice {
  QString fuel_type;
  QDate date;
  double price = 0.0;
};

#endif
