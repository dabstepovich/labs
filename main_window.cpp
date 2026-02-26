
#include "main_window.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleValidator>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>

#include "file_reader.h"

namespace {

constexpr int kColumnCount = 3;
const QStringList kHeaders = {"Тип топлива", "Дата", "Цена (руб.)"};

constexpr int kColFuelType = 0;
constexpr int kColDate = 1;
constexpr int kColPrice = 2;

const QString kDateFormat = "yyyy.MM.dd";

}  

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      table_view_(nullptr),
      add_button_(nullptr),
      delete_button_(nullptr),
      status_label_(nullptr),
      model_(nullptr),
      current_file_path_(QString()) {
  SetupUi();
  SetupTableModel();
}

void MainWindow::SetupUi() {
  setWindowTitle("Цены на топливо");
  resize(640, 480);

  QWidget *central_widget = new QWidget(this);
  setCentralWidget(central_widget);

  table_view_ = new QTableView();
  table_view_->setSelectionBehavior(QAbstractItemView::SelectRows);
  table_view_->setSelectionMode(QAbstractItemView::SingleSelection);
  table_view_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table_view_->horizontalHeader()->setStretchLastSection(true);
  table_view_->verticalHeader()->setVisible(false);

  add_button_ = new QPushButton("Добавить", central_widget);
  delete_button_ = new QPushButton("Удалить", central_widget);

  status_label_ = new QLabel("Готово.", central_widget);

  QHBoxLayout *button_layout = new QHBoxLayout();
  button_layout->addWidget(add_button_);
  button_layout->addWidget(delete_button_);
  button_layout->addStretch();

  QVBoxLayout *main_layout = new QVBoxLayout(central_widget);
  main_layout->addWidget(table_view_);
  main_layout->addLayout(button_layout);
  main_layout->addWidget(status_label_);

  connect(add_button_, &QPushButton::clicked,
          this, &MainWindow::OnAddButtonClicked);
  connect(delete_button_, &QPushButton::clicked,
          this, &MainWindow::OnDeleteButtonClicked);
}

void MainWindow::SetupTableModel() {
  model_ = new QStandardItemModel(0, kColumnCount, this);
  model_->setHorizontalHeaderLabels(kHeaders);
  table_view_->setModel(model_);
}

void MainWindow::LoadFromFile(const QString &file_path) {
  current_file_path_ = file_path;
  model_->removeRows(0, model_->rowCount());

  QList<FuelPrice> fuel_prices = ReadFuelPricesFromFile(file_path);

  for (const FuelPrice &fp : fuel_prices) {
    AppendRowToTable(fp);
  }

  status_label_->setText(
      QString("Загружено записей: %1").arg(fuel_prices.size()));
}

void MainWindow::AppendRowToTable(const FuelPrice &fuel_price) {
  QList<QStandardItem *> row;
  row.append(new QStandardItem(fuel_price.fuel_type));
  row.append(new QStandardItem(fuel_price.date.toString(kDateFormat)));
  row.append(new QStandardItem(QString::number(fuel_price.price, 'f', 2)));
  model_->appendRow(row);
}

FuelPrice MainWindow::ReadRowFromModel(int row) const {
  FuelPrice fp;
  fp.fuel_type = model_->item(row, kColFuelType)->text();
  fp.date = QDate::fromString(model_->item(row, kColDate)->text(), kDateFormat);
  fp.price = model_->item(row, kColPrice)->text().toDouble();
  return fp;
}

void MainWindow::SaveCurrentStateToFile() {
  if (current_file_path_.isEmpty()) {
    return;
  }

  QList<FuelPrice> fuel_prices;
  for (int row = 0; row < model_->rowCount(); ++row) {
    fuel_prices.append(ReadRowFromModel(row));
  }

  bool ok = SaveFuelPricesToFile(current_file_path_, fuel_prices);

  if (!ok) {
    QMessageBox::warning(this, "Ошибка",
                         "Не удалось сохранить файл:\n" + current_file_path_);
  }
}

int MainWindow::GetSelectedRow() const {
  QModelIndexList selection = table_view_->selectionModel()->selectedRows();
  if (selection.isEmpty()) {
    return -1;
  }
  return selection.first().row();
}

void MainWindow::OnAddButtonClicked() {
  QDialog dialog(this);
  dialog.setWindowTitle("Добавить запись");

  QFormLayout *form = new QFormLayout(&dialog);

  QLineEdit *fuel_type_edit = new QLineEdit(&dialog);
  QLineEdit *date_edit = new QLineEdit(&dialog);
  date_edit->setPlaceholderText("ГГГГ.ММ.ДД");
  QLineEdit *price_edit = new QLineEdit(&dialog);
  price_edit->setValidator(new QDoubleValidator(0, 9999, 2, &dialog));

  form->addRow("Тип топлива:", fuel_type_edit);
  form->addRow("Дата:", date_edit);
  form->addRow("Цена (руб.):", price_edit);

  QDialogButtonBox *buttons = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  form->addRow(buttons);

  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  if (dialog.exec() != QDialog::Accepted) {
    return;
  }

  if (fuel_type_edit->text().trimmed().isEmpty() ||
      date_edit->text().trimmed().isEmpty() ||
      price_edit->text().trimmed().isEmpty()) {
    QMessageBox::warning(this, "Ошибка", "Все поля должны быть заполнены.");
    return;
  }

  QDate parsed_date = QDate::fromString(date_edit->text().trimmed(),
                                        kDateFormat);
  if (!parsed_date.isValid()) {
    QMessageBox::warning(this, "Ошибка",
                         "Неверный формат даты. Используйте: ГГГГ.ММ.ДД");
    return;
  }

  FuelPrice new_entry;
  new_entry.fuel_type = fuel_type_edit->text().trimmed();
  new_entry.date = parsed_date;
  new_entry.price = price_edit->text().toDouble();

  AppendRowToTable(new_entry);

  SaveCurrentStateToFile();

  status_label_->setText(
      QString("Записей в таблице: %1").arg(model_->rowCount()));
}

void MainWindow::OnDeleteButtonClicked() {
  int row = GetSelectedRow();
  if (row == -1) {
    QMessageBox::information(this, "Удаление",
                             "Выберите строку для удаления.");
    return;
  }

  model_->removeRow(row);

  SaveCurrentStateToFile();

  status_label_->setText(
      QString("Записей в таблице: %1").arg(model_->rowCount()));
}
