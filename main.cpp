#include "main_window.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QFileDialog>
#include <QTextStream>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  app.setApplicationName("FuelPrices");

  QCommandLineParser parser;
  parser.addHelpOption();
  QCommandLineOption log_option("log", "Файл для записи лога", "file");
  parser.addOption(log_option);
  parser.process(app);

  if (parser.isSet(log_option)) {
    QString log_path = parser.value(log_option);
    QFile log_file(log_path);
    if (log_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      QTextStream out(&log_file);
      out.setEncoding(QStringConverter::Utf8);
      out << "привет\n";
      log_file.close();
    }
  }

  MainWindow window;
  window.show();

  QString file_path = QFileDialog::getOpenFileName(
      &window, "Открыть файл с ценами на топливо", QString(),
      "Текстовые файлы (*.txt);;Все файлы (*)");
  if (!file_path.isEmpty()) {
    window.LoadFromFile(file_path);
  }

  return app.exec();
}
