#include "command_processor.h"
#include "parse_exception.h"
#include <QDate>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

QStringList BatchResult::summaryLines() const {
  QStringList lines;
  for (int i = 0; i < results.size(); ++i) {
    const CommandResult &r = results[i];
    if (r.success) {
      lines << QString("[%1] OK: %2").arg(i + 1).arg(r.message);
    } else {
      lines << QString("[%1] ОШИБКА: %2").arg(i + 1).arg(r.error);
    }
  }
  return lines;
}

CommandProcessor::CommandProcessor(FuelModel &model, SaveFn save_fn)
    : model_(model), save_fn_(save_fn ? std::move(save_fn)
                                      : [](FuelModel &m, const QString &path) {
                                          m.SaveToFile(path);
                                        }) {}

BatchResult CommandProcessor::ExecuteFile(const QString &file_path) {
  BatchResult batch;
  QFile file(file_path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    throw std::runtime_error(QString("Не удалось открыть файл команд: %1")
                                 .arg(file_path)
                                 .toStdString());
  }
  QTextStream in(&file);
  in.setEncoding(QStringConverter::Utf8);
  while (!in.atEnd()) {
    QString line = in.readLine().trimmed();
    if (line.isEmpty() || line.startsWith('#'))
      continue;
    CommandResult r = ExecuteLine(line);
    batch.results << r;
    if (r.success)
      ++batch.executed;
    else
      ++batch.failed;
  }
  file.close();
  return batch;
}

CommandResult CommandProcessor::ExecuteLine(const QString &line) {
  QString trimmed = line.trimmed();
  if (trimmed.isEmpty()) {
    return {true, "Пустая строка пропущена"};
  }

  int space = trimmed.indexOf(' ');
  QString keyword = (space == -1 ? trimmed : trimmed.left(space)).toUpper();
  QString args = (space == -1 ? QString() : trimmed.mid(space + 1).trimmed());

  if (keyword == "ADD")
    return HandleAdd(args);
  if (keyword == "REM")
    return HandleRem(args);
  if (keyword == "SAVE")
    return HandleSave(args);

  return {false, {}, QString("Неизвестная команда: \"%1\"").arg(keyword)};
}

FuelPrice CommandProcessor::ParseAddArgument(const QString &csv) {
  QStringList parts = csv.split(';');
  if (parts.size() < 3) {
    throw ParseException(
        QString("ADD: ожидается 3 поля через ';', получено %1: \"%2\"")
            .arg(parts.size())
            .arg(csv));
  }

  QString fuel_type = parts[0].trimmed();
  if (fuel_type.isEmpty()) {
    throw ParseException("ADD: тип топлива не может быть пустым");
  }

  const QString kDateFormat = "yyyy.MM.dd";
  QDate date = QDate::fromString(parts[1].trimmed(), kDateFormat);
  if (!date.isValid()) {
    throw ParseException(
        QString("ADD: неверный формат даты \"%1\" (ожидается %2)")
            .arg(parts[1].trimmed(), kDateFormat));
  }

  bool ok = false;
  double price = parts[2].trimmed().toDouble(&ok);
  if (!ok) {
    throw ParseException(
        QString("ADD: неверный формат цены \"%1\"").arg(parts[2].trimmed()));
  }
  if (price < 0.0) {
    throw ParseException(
        QString("ADD: цена не может быть отрицательной (%1)").arg(price));
  }

  FuelPrice fp;
  fp.fuel_type = fuel_type;
  fp.date = date;
  fp.price = price;
  return fp;
}

CommandResult CommandProcessor::HandleAdd(const QString &args) {
  try {
    FuelPrice fp = ParseAddArgument(args);
    model_.AddEntry(fp);
    return {true, QString("Добавлена запись: %1 %2 %3")
                      .arg(fp.fuel_type)
                      .arg(fp.date.toString("yyyy.MM.dd"))
                      .arg(fp.price, 0, 'f', 2)};
  } catch (const ParseException &ex) {
    return {false, {}, ex.qwhat()};
  }
}

std::function<bool(const FuelPrice &)>
CommandProcessor::ParseRemCondition(const QString &condition) {
  static const QRegularExpression kRe(
      R"(^\s*(\w+)\s*(<=|>=|==|!=|<|>)\s*(.+?)\s*$)");

  QRegularExpressionMatch m = kRe.match(condition);
  if (!m.hasMatch()) {
    throw ParseException(
        QString("REM: неверный формат условия: \"%1\"").arg(condition));
  }

  QString field = m.captured(1).toLower();
  QString op = m.captured(2);
  QString value = m.captured(3).trimmed();

  if (field == "price") {
    bool ok = false;
    double threshold = value.toDouble(&ok);
    if (!ok) {
      throw ParseException(
          QString("REM: не удалось преобразовать \"%1\" к числу").arg(value));
    }
    return [op, threshold](const FuelPrice &fp) -> bool {
      if (op == "<")
        return fp.price < threshold;
      if (op == "<=")
        return fp.price <= threshold;
      if (op == ">")
        return fp.price > threshold;
      if (op == ">=")
        return fp.price >= threshold;
      if (op == "==")
        return fp.price == threshold;
      if (op == "!=")
        return fp.price != threshold;
      return false;
    };
  }

  if (field == "date") {
    QDate threshold = QDate::fromString(value, "yyyy.MM.dd");
    if (!threshold.isValid()) {
      throw ParseException(
          QString("REM: неверный формат даты \"%1\"").arg(value));
    }
    return [op, threshold](const FuelPrice &fp) -> bool {
      if (op == "<")
        return fp.date < threshold;
      if (op == "<=")
        return fp.date <= threshold;
      if (op == ">")
        return fp.date > threshold;
      if (op == ">=")
        return fp.date >= threshold;
      if (op == "==")
        return fp.date == threshold;
      if (op == "!=")
        return fp.date != threshold;
      return false;
    };
  }

  if (field == "fuel_type") {
    if (op != "==" && op != "!=") {
      throw ParseException(
          QString("REM: для поля fuel_type допустимы только == и !=, "
                  "получен: \"%1\"")
              .arg(op));
    }
    return [op, value](const FuelPrice &fp) -> bool {
      bool eq = (fp.fuel_type == value);
      return (op == "==") ? eq : !eq;
    };
  }

  throw ParseException(QString("REM: неизвестное поле \"%1\". "
                               "Допустимые: price, date, fuel_type")
                           .arg(field));
}

CommandResult CommandProcessor::HandleRem(const QString &args) {
  try {
    auto predicate = ParseRemCondition(args);
    int removed = model_.RemoveIf(predicate);
    return {
        true,
        QString("Удалено записей: %1 (условие: %2)").arg(removed).arg(args)};
  } catch (const ParseException &ex) {
    return {false, {}, ex.qwhat()};
  }
}

CommandResult CommandProcessor::HandleSave(const QString &args) {
  if (args.isEmpty()) {
    return {false, {}, "SAVE: не указан путь к файлу"};
  }
  try {
    save_fn_(model_, args);
    return {true, QString("Данные сохранены в: %1").arg(args)};
  } catch (const std::exception &ex) {
    return {false, {}, QString::fromStdString(ex.what())};
  }
}
