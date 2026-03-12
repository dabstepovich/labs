#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include "fuel_model.h"
#include "fuel_price.h"
#include <QString>
#include <QStringList>
#include <functional>

struct CommandResult {
  bool success = true;
  QString message;
  QString error;
};

struct BatchResult {
  QList<CommandResult> results;
  int executed = 0;
  int failed = 0;
  QStringList summaryLines() const;
};

class CommandProcessor {
public:
  using SaveFn = std::function<void(FuelModel &, const QString &)>;

  explicit CommandProcessor(FuelModel &model, SaveFn save_fn = nullptr);

  BatchResult ExecuteFile(const QString &file_path);

  CommandResult ExecuteLine(const QString &line);

  static FuelPrice ParseAddArgument(const QString &csv);

  static std::function<bool(const FuelPrice &)>
  ParseRemCondition(const QString &condition);

  static std::pair<QString, QString> ParseFilterArgument(const QString &args);

private:
  FuelModel &model_;
  SaveFn save_fn_;

  CommandResult HandleAdd(const QString &args);
  CommandResult HandleRem(const QString &args);
  CommandResult HandleSave(const QString &args);
  CommandResult HandleFilter(const QString &args);
};

#endif
