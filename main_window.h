#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>

#include "fuel_price.h"

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

 private:
  void SetupUi();

  void SetupTableModel();

  void AppendRowToTable(const FuelPrice &fuel_price);

  int GetSelectedRow() const;

  void SaveCurrentStateToFile();

  FuelPrice ReadRowFromModel(int row) const;

  QTableView *table_view_;
  QPushButton *add_button_;
  QPushButton *delete_button_;
  QLabel *status_label_;
  QStandardItemModel *model_;

  QString current_file_path_;
};

#endif 
