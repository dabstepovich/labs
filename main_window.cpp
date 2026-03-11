#include "main_window.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleValidator>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>
#include <QWidget>

namespace {

constexpr int kColumnCount = 3;
const QStringList kHeaders = {"Тип топлива", "Дата", "Цена (руб.)"};
const QString kDateFormat = "yyyy.MM.dd";

} // namespace
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), table_view_(nullptr), add_button_(nullptr),
      delete_button_(nullptr), load_commands_button_(nullptr),
      status_label_(nullptr), table_model_(nullptr),
      current_file_path_(QString()) {
  SetupUi();
  SetupTableModel();
}

void MainWindow::SetupUi() {
  setWindowTitle("Цены на топливо");
  resize(700, 500);

  QWidget *central = new QWidget(this);
  setCentralWidget(central);

  table_view_ = new QTableView(central);
  table_view_->setSelectionBehavior(QAbstractItemView::SelectRows);
  table_view_->setSelectionMode(QAbstractItemView::SingleSelection);
  table_view_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table_view_->horizontalHeader()->setStretchLastSection(true);
  table_view_->verticalHeader()->setVisible(false);

  add_button_ = new QPushButton("Добавить", central);
  delete_button_ = new QPushButton("Удалить", central);
  load_commands_button_ = new QPushButton("Загрузить команды…", central);
  status_label_ = new QLabel("Готово.", central);

  QHBoxLayout *btn_layout = new QHBoxLayout();
  btn_layout->addWidget(add_button_);
  btn_layout->addWidget(delete_button_);
  btn_layout->addWidget(load_commands_button_);
  btn_layout->addStretch();

  QVBoxLayout *main_layout = new QVBoxLayout(central);
  main_layout->addWidget(table_view_);
  main_layout->addLayout(btn_layout);
  main_layout->addWidget(status_label_);

  connect(add_button_, &QPushButton::clicked, this,
          &MainWindow::OnAddButtonClicked);
  connect(delete_button_, &QPushButton::clicked, this,
          &MainWindow::OnDeleteButtonClicked);
  connect(load_commands_button_, &QPushButton::clicked, this,
          &MainWindow::OnLoadCommandsClicked);
}

void MainWindow::SetupTableModel() {
  table_model_ = new QStandardItemModel(0, kColumnCount, this);
  table_model_->setHorizontalHeaderLabels(kHeaders);
  table_view_->setModel(table_model_);
}

void MainWindow::RefreshTable() {
  table_model_->removeRows(0, table_model_->rowCount());
  for (const FuelPrice &fp : fuel_model_.prices()) {
    AppendRowToTable(fp);
  }
}

void MainWindow::AppendRowToTable(const FuelPrice &fp) {
  QList<QStandardItem *> row;
  row.append(new QStandardItem(fp.fuel_type));
  row.append(new QStandardItem(fp.date.toString(kDateFormat)));
  row.append(new QStandardItem(QString::number(fp.price, 'f', 2)));
  table_model_->appendRow(row);
}

void MainWindow::LoadFromFile(const QString &file_path) {
  current_file_path_ = file_path;

  QStringList errors;
  int loaded = 0;

  try {
    loaded = fuel_model_.LoadFromFile(file_path, &errors);
  } catch (const std::exception &ex) {
    QMessageBox::critical(this, "Ошибка открытия файла",
                          QString::fromStdString(ex.what()));
    return;
  }

  RefreshTable();
  if (!errors.isEmpty())
    ShowParseErrors(errors);

  status_label_->setText(QString("Загружено записей: %1 (пропущено: %2)")
                             .arg(loaded)
                             .arg(errors.size()));
}

void MainWindow::ShowParseErrors(const QStringList &errors) {
  QMessageBox box(this);
  box.setWindowTitle("Ошибки при загрузке файла");
  box.setIcon(QMessageBox::Warning);
  box.setText(
      QString("Часть строк пропущена из-за ошибок (%1 шт.).\nПодробности:")
          .arg(errors.size()));
  box.setDetailedText(errors.join('\n'));
  box.exec();
}

void MainWindow::ShowBatchResult(const BatchResult &result) {
  QMessageBox box(this);
  box.setWindowTitle("Результат выполнения команд");
  if (result.failed == 0) {
    box.setIcon(QMessageBox::Information);
    box.setText(QString("Выполнено команд: %1").arg(result.executed));
  } else {
    box.setIcon(QMessageBox::Warning);
    box.setText(QString("Выполнено: %1, с ошибками: %2")
                    .arg(result.executed)
                    .arg(result.failed));
  }
  box.setDetailedText(result.summaryLines().join('\n'));
  box.exec();
}

void MainWindow::SaveCurrentState() {
  if (current_file_path_.isEmpty())
    return;
  try {
    fuel_model_.SaveToFile(current_file_path_);
  } catch (const std::exception &ex) {
    QMessageBox::warning(this, "Ошибка сохранения",
                         QString::fromStdString(ex.what()));
  }
}

int MainWindow::GetSelectedRow() const {
  QModelIndexList sel = table_view_->selectionModel()->selectedRows();
  return sel.isEmpty() ? -1 : sel.first().row();
}

void MainWindow::OnAddButtonClicked() {
  QDialog dialog(this);
  dialog.setWindowTitle("Добавить запись");

  QFormLayout *form = new QFormLayout(&dialog);

  QLineEdit *fuel_type_edit = new QLineEdit(&dialog);
  QLineEdit *date_edit = new QLineEdit(&dialog);
  date_edit->setPlaceholderText("ГГГГ.ММ.ДД");
  QLineEdit *price_edit = new QLineEdit(&dialog);
  price_edit->setValidator(new QDoubleValidator(0, 999999, 2, &dialog));

  form->addRow("Тип топлива:", fuel_type_edit);
  form->addRow("Дата:", date_edit);
  form->addRow("Цена (руб.):", price_edit);

  QDialogButtonBox *buttons = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  form->addRow(buttons);

  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  if (dialog.exec() != QDialog::Accepted)
    return;

  if (fuel_type_edit->text().trimmed().isEmpty() ||
      date_edit->text().trimmed().isEmpty() ||
      price_edit->text().trimmed().isEmpty()) {
    QMessageBox::warning(this, "Ошибка", "Все поля должны быть заполнены.");
    return;
  }

  QDate parsed_date =
      QDate::fromString(date_edit->text().trimmed(), kDateFormat);
  if (!parsed_date.isValid()) {
    QMessageBox::warning(this, "Ошибка",
                         "Неверный формат даты. Используйте: ГГГГ.ММ.ДД");
    return;
  }

  FuelPrice new_entry;
  new_entry.fuel_type = fuel_type_edit->text().trimmed();
  new_entry.date = parsed_date;
  new_entry.price = price_edit->text().toDouble();

  fuel_model_.AddEntry(new_entry);
  AppendRowToTable(new_entry);
  SaveCurrentState();

  status_label_->setText(
      QString("Записей в таблице: %1").arg(fuel_model_.count()));
}

void MainWindow::OnDeleteButtonClicked() {
  int row = GetSelectedRow();
  if (row == -1) {
    QMessageBox::information(this, "Удаление", "Выберите строку для удаления.");
    return;
  }

  try {
    fuel_model_.RemoveEntry(row);
  } catch (const std::out_of_range &ex) {
    QMessageBox::warning(this, "Ошибка удаления",
                         QString::fromStdString(ex.what()));
    return;
  }

  table_model_->removeRow(row);
  SaveCurrentState();

  status_label_->setText(
      QString("Записей в таблице: %1").arg(fuel_model_.count()));
}

void MainWindow::OnLoadCommandsClicked() {
  QString file_path =
      QFileDialog::getOpenFileName(this, "Открыть файл команд", QString(),
                                   "Текстовые файлы (*.txt);;Все файлы (*)");
  if (file_path.isEmpty())
    return;

  CommandProcessor processor(fuel_model_);
  BatchResult result;
  try {
    result = processor.ExecuteFile(file_path);
  } catch (const std::exception &ex) {
    QMessageBox::critical(this, "Ошибка открытия файла команд",
                          QString::fromStdString(ex.what()));
    return;
  }

  RefreshTable();
  SaveCurrentState();

  status_label_->setText(
      QString("После команд: записей %1 (выполнено %2, ошибок %3)")
          .arg(fuel_model_.count())
          .arg(result.executed)
          .arg(result.failed));

  ShowBatchResult(result);
}
