#include "main_window.h"
#include <QApplication>
#include <QFileDialog>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  app.setApplicationName("FuelPrices");

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
