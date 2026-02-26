#include "file_reader.h"

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

FuelPrice ParseLine(const QString &line) {
  FuelPrice fuel_price;

  int first_quote = line.indexOf('"');
  int last_quote = line.lastIndexOf('"');

  if (first_quote == -1 || last_quote == -1 || first_quote == last_quote) {
    return fuel_price;
  }

  fuel_price.fuel_type = line.mid(first_quote + 1, last_quote - first_quote - 1);

  QString remainder = line.mid(last_quote + 1).trimmed();
  QStringList parts = remainder.split(QRegularExpression("\\s+"),
                                      Qt::SkipEmptyParts);

  if (parts.size() >= 2) {
    fuel_price.date = QDate::fromString(parts[0], "yyyy.MM.dd");
    fuel_price.price = parts[1].toDouble();
  }

  return fuel_price;
}

QList<FuelPrice> ReadFuelPricesFromFile(const QString &file_path) {
  QList<FuelPrice> fuel_prices;
  QFile file(file_path);

  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return fuel_prices;
  }

  QTextStream in(&file);
  in.setEncoding(QStringConverter::Utf8);

  while (!in.atEnd()) {
    QString line = in.readLine().trimmed();
    if (!line.isEmpty()) {
      fuel_prices.append(ParseLine(line));
    }
  }

  file.close();
  return fuel_prices;
}

bool SaveFuelPricesToFile(const QString &file_path,
                          const QList<FuelPrice> &fuel_prices) {
  QFile file(file_path);

  if (!file.open(QIODevice::WriteOnly | QIODevice::Text |
                 QIODevice::Truncate)) {
    return false;
  }

  QTextStream out(&file);
  out.setEncoding(QStringConverter::Utf8);

  for (const FuelPrice &fp : fuel_prices) {
    out << '"' << fp.fuel_type << '"'
        << ' ' << fp.date.toString("yyyy.MM.dd")
        << ' ' << QString::number(fp.price, 'f', 2)
        << '\n';
  }

  file.close();
  return true;
}
