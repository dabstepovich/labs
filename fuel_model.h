#ifndef FUEL_MODEL_H
#define FUEL_MODEL_H

#include "fuel_price.h"
#include <QList>
#include <QString>
#include <functional>

class FuelModel {
public:
  FuelModel() = default;

  static FuelPrice ParseLine(const QString &line);

  int LoadFromFile(const QString &file_path,
                   QStringList *parse_errors = nullptr);
  void SaveToFile(const QString &file_path) const;

  void AddEntry(const FuelPrice &fp);
  void RemoveEntry(int index);

  int RemoveIf(const std::function<bool(const FuelPrice &)> &predicate);

  const QList<FuelPrice> &prices() const { return prices_; }
  int count() const { return prices_.size(); }

private:
  QList<FuelPrice> prices_;
};
#endif
