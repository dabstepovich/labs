#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>

#include "fuel_model.h"
#include "fuel_price.h"
#include "command_processor.h"

QT_BEGIN_NAMESPACE
class QTableView;
class QPushButton;
class QLabel;
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override = default;

  void LoadFromFile(const QString &file_path);

 private slots:
  void OnAddButtonClicked();
  void OnDeleteButtonClicked();
  void OnLoadCommandsClicked();

 private:
  void SetupUi();
  void SetupTableModel();
  void RefreshTable();
  void AppendRowToTable(const FuelPrice &fp);
  int  GetSelectedRow() const;
  void SaveCurrentState();
  void ShowParseErrors(const QStringList &errors);
  void ShowBatchResult(const BatchResult &result);

  QTableView         *table_view_;
  QPushButton        *add_button_;
  QPushButton        *delete_button_;
  QPushButton        *load_commands_button_;
  QLabel             *status_label_;
  QStandardItemModel *table_model_;

  FuelModel  fuel_model_;
  QString    current_file_path_;
};
#endif
