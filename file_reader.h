#ifndef FILE_READER_H
#define FILE_READER_H

#include <QList>
#include <QString>

#include "fuel_price.h"

FuelPrice ParseLine(const QString &line);

QList<FuelPrice> ReadFuelPricesFromFile(const QString &file_path);

bool SaveFuelPricesToFile(const QString &file_path,
                          const QList<FuelPrice> &fuel_prices);

#endif  
