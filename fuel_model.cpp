#include "fuel_model.h"

#include <QDate>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>
#include <stdexcept>

#include "parse_exception.h"

FuelPrice FuelModel::ParseLine(const QString &line) {
  if (line.trimmed().isEmpty()) {
    throw ParseException("Пустая строка");
  }

  int first_quote = line.indexOf('"');
  int last_quote = line.lastIndexOf('"');

  if (first_quote == -1 || last_quote == -1 || first_quote == last_quote) {
    throw ParseException(
        QString("Отсутствуют или неверно расставлены кавычки: \"%1\"")
            .arg(line));
  }

  QString fuel_type =
      line.mid(first_quote + 1, last_quote - first_quote - 1).trimmed();
  if (fuel_type.isEmpty()) {
    throw ParseException(
        QString("Тип топлива не может быть пустым: \"%1\"").arg(line));
  }

  QString remainder = line.mid(last_quote + 1).trimmed();
  QStringList parts =
      remainder.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

  if (parts.size() < 2) {
    throw ParseException(
        QString("Ожидались дата и цена после типа топлива: \"%1\"").arg(line));
  }

  const QString kDateFormat = "yyyy.MM.dd";
  QDate date = QDate::fromString(parts[0], kDateFormat);
  if (!date.isValid()) {
    throw ParseException(
        QString("Неверный формат даты \"%1\" (ожидается %2) в строке: \"%3\"")
            .arg(parts[0], kDateFormat, line));
  }

  bool price_ok = false;
  double price = parts[1].toDouble(&price_ok);
  if (!price_ok) {
    throw ParseException(QString("Неверный формат цены \"%1\" в строке: \"%2\"")
                             .arg(parts[1], line));
  }
  if (price < 0.0) {
    throw ParseException(
        QString("Цена не может быть отрицательной (%1) в строке: \"%2\"")
            .arg(price)
            .arg(line));
  }

  FuelPrice fp;
  fp.fuel_type = fuel_type;
  fp.date = date;
  fp.price = price;
  return fp;
}

int FuelModel::LoadFromFile(const QString &file_path,
                            QStringList *parse_errors) {
  prices_.clear();

  QFile file(file_path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    throw std::runtime_error(
        QString("Не удалось открыть файл: %1").arg(file_path).toStdString());
  }

  QTextStream in(&file);
  in.setEncoding(QStringConverter::Utf8);

  int line_number = 0;
  int loaded = 0;

  while (!in.atEnd()) {
    ++line_number;
    QString line = in.readLine().trimmed();
    if (line.isEmpty())
      continue;
    try {
      prices_.append(ParseLine(line));
      ++loaded;
    } catch (const ParseException &ex) {
      QString error_msg =
          QString("[Строка %1] %2").arg(line_number).arg(ex.qwhat());
      if (parse_errors)
        parse_errors->append(error_msg);
      qWarning().noquote() << "FuelModel::LoadFromFile:" << error_msg;
    }
  }
  file.close();
  return loaded;
}

void FuelModel::SaveToFile(const QString &file_path) const {
  QFile file(file_path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text |
                 QIODevice::Truncate)) {
    throw std::runtime_error(
        QString("Не удалось сохранить файл: %1").arg(file_path).toStdString());
  }

  QTextStream out(&file);
  out.setEncoding(QStringConverter::Utf8);

  const QString kDateFormat = "yyyy.MM.dd";
  for (const FuelPrice &fp : prices_) {
    out << '"' << fp.fuel_type << '"' << ' ' << fp.date.toString(kDateFormat)
        << ' ' << QString::number(fp.price, 'f', 2) << '\n';
  }
  file.close();
}

void FuelModel::AddEntry(const FuelPrice &fp) { prices_.append(fp); }

void FuelModel::RemoveEntry(int index) {
  if (index < 0 || index >= prices_.size()) {
    throw std::out_of_range(
        QString("RemoveEntry: индекс %1 вне диапазона [0, %2)")
            .arg(index)
            .arg(prices_.size())
            .toStdString());
  }
  prices_.removeAt(index);
}

int FuelModel::RemoveIf(
    const std::function<bool(const FuelPrice &)> &predicate) {
  int removed = 0;
  for (int i = prices_.size() - 1; i >= 0; --i) {
    if (predicate(prices_[i])) {
      prices_.removeAt(i);
      ++removed;
    }
  }
  return removed;
}
