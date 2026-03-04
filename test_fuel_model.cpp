#include <QtTest/QtTest>
#include <QTemporaryFile>
#include <QTextStream>

#include "fuel_model.h"
#include "parse_exception.h"

class TestFuelModel : public QObject {
  Q_OBJECT

 private slots:


  void parseLine_validBasic() {
    FuelPrice fp = FuelModel::ParseLine(R"("АИ-95" 2024.03.15 58.40)");
    QCOMPARE(fp.fuel_type, QString("АИ-95"));
    QCOMPARE(fp.date, QDate(2024, 3, 15));
    QCOMPARE(fp.price, 58.40);
  }

  void parseLine_validWithSpacesInFuelType() {
    FuelPrice fp = FuelModel::ParseLine(R"("Дизельное топливо" 2023.11.01 72.10)");
    QCOMPARE(fp.fuel_type, QString("Дизельное топливо"));
    QCOMPARE(fp.date, QDate(2023, 11, 1));
    QCOMPARE(fp.price, 72.10);
  }

  void parseLine_validIntegerPrice() {
    FuelPrice fp = FuelModel::ParseLine(R"("АИ-92" 2024.01.01 55)");
    QCOMPARE(fp.price, 55.0);
  }

  void parseLine_validLeadingTrailingSpaces() {
    FuelPrice fp = FuelModel::ParseLine(R"(  "АИ-98"  2024.06.10  75.00  )");
    QCOMPARE(fp.fuel_type, QString("АИ-98"));
    QCOMPARE(fp.price, 75.0);
  }

  void parseLine_emptyLine_throws() {
    QVERIFY_THROWS_EXCEPTION(ParseException,
                             FuelModel::ParseLine(""));
  }

  void parseLine_missingQuotes_throws() {
    QVERIFY_THROWS_EXCEPTION(ParseException,
                             FuelModel::ParseLine("АИ-95 2024.03.15 58.40"));
  }

  void parseLine_onlyOpenQuote_throws() {
    QVERIFY_THROWS_EXCEPTION(ParseException,
                             FuelModel::ParseLine(R"x('АИ-95 2024.03.15 58.40)x"));
  }

  void parseLine_emptyFuelType_throws() {
    QVERIFY_THROWS_EXCEPTION(ParseException,
                             FuelModel::ParseLine(R"("" 2024.03.15 58.40)"));
  }

  void parseLine_missingDate_throws() {
    QVERIFY_THROWS_EXCEPTION(ParseException,
                             FuelModel::ParseLine(R"("АИ-95" 58.40)"));
  }

  void parseLine_invalidDateFormat_throws() {
    QVERIFY_THROWS_EXCEPTION(ParseException,
                             FuelModel::ParseLine(R"("АИ-95" 15.03.2024 58.40)"));
  }

  void parseLine_invalidPrice_throws() {
    QVERIFY_THROWS_EXCEPTION(ParseException,
                             FuelModel::ParseLine(R"("АИ-95" 2024.03.15 abc)"));
  }

  void parseLine_negativePrice_throws() {
    QVERIFY_THROWS_EXCEPTION(ParseException,
                             FuelModel::ParseLine(R"("АИ-95" 2024.03.15 -5.00)"));
  }

  void parseLine_missingPriceField_throws() {
    QVERIFY_THROWS_EXCEPTION(ParseException,
                             FuelModel::ParseLine(R"("АИ-95" 2024.03.15)"));
  }


  void loadFromFile_validFile() {
    QTemporaryFile tmp;
    QVERIFY(tmp.open());
    {
      QTextStream out(&tmp);
      out.setEncoding(QStringConverter::Utf8);
      out << R"("АИ-95" 2024.01.10 58.00)" << '\n';
      out << R"("АИ-92" 2024.01.10 54.50)" << '\n';
      out << R"("Дизель" 2024.01.10 70.20)" << '\n';
    }
    tmp.close();

    FuelModel model;
    QStringList errors;
    int loaded = model.LoadFromFile(tmp.fileName(), &errors);

    QCOMPARE(loaded, 3);
    QCOMPARE(errors.size(), 0);
    QCOMPARE(model.count(), 3);
    QCOMPARE(model.prices().at(0).fuel_type, QString("АИ-95"));
    QCOMPARE(model.prices().at(2).price, 70.20);
  }


  void loadFromFile_mixedFile_skipsInvalidLines() {
    QTemporaryFile tmp;
    QVERIFY(tmp.open());
    {
      QTextStream out(&tmp);
      out.setEncoding(QStringConverter::Utf8);
      out << R"("АИ-95" 2024.01.10 58.00)"   << '\n'; // OK
      out << "плохая строка без кавычек"       << '\n'; // ошибка
      out << R"("АИ-92" 15.03.2024 54.50)"   << '\n'; // плохая дата
      out << R"("Дизель" 2024.01.10 70.20)"  << '\n'; // OK
      out << R"("" 2024.05.01 60.00)"         << '\n'; // пустой тип
    }
    tmp.close();

    FuelModel model;
    QStringList errors;
    int loaded = model.LoadFromFile(tmp.fileName(), &errors);

    QCOMPARE(loaded, 2);
    QCOMPARE(errors.size(), 3);
    QCOMPARE(model.count(), 2);
  }


  void loadFromFile_nonexistentFile_throws() {
    FuelModel model;
    QVERIFY_THROWS_EXCEPTION(std::runtime_error,
        model.LoadFromFile("/tmp/no_such_file_xyz_12345.txt"));
  }


  void saveAndLoad_roundTrip() {
    FuelModel model_save;
    FuelPrice fp1; fp1.fuel_type = "АИ-95"; fp1.date = QDate(2024,3,1);  fp1.price = 58.0;
    FuelPrice fp2; fp2.fuel_type = "Дизель"; fp2.date = QDate(2024,4,15); fp2.price = 72.5;
    model_save.AddEntry(fp1);
    model_save.AddEntry(fp2);

    QTemporaryFile tmp;
    QVERIFY(tmp.open());
    tmp.close();
    model_save.SaveToFile(tmp.fileName());

    FuelModel model_load;
    QStringList errors;
    int loaded = model_load.LoadFromFile(tmp.fileName(), &errors);

    QCOMPARE(loaded, 2);
    QCOMPARE(errors.size(), 0);
    QCOMPARE(model_load.prices().at(0).fuel_type, QString("АИ-95"));
    QCOMPARE(model_load.prices().at(0).date, QDate(2024, 3, 1));
    QCOMPARE(model_load.prices().at(0).price, 58.0);
    QCOMPARE(model_load.prices().at(1).fuel_type, QString("Дизель"));
    QCOMPARE(model_load.prices().at(1).price, 72.5);
  }


  void saveToFile_invalidPath_throws() {
    FuelModel model;
    QVERIFY_THROWS_EXCEPTION(std::runtime_error,
        model.SaveToFile("/no_such_dir_xyz/file.txt"));
  }


  void addEntry_increasesCount() {
    FuelModel model;
    QCOMPARE(model.count(), 0);

    FuelPrice fp; fp.fuel_type = "АИ-98"; fp.date = QDate(2024,1,1); fp.price = 80.0;
    model.AddEntry(fp);
    QCOMPARE(model.count(), 1);
    model.AddEntry(fp);
    QCOMPARE(model.count(), 2);
  }


  void removeEntry_validIndex() {
    FuelModel model;
    FuelPrice fp1; fp1.fuel_type = "А";  fp1.date = QDate(2024,1,1); fp1.price = 1.0;
    FuelPrice fp2; fp2.fuel_type = "Б";  fp2.date = QDate(2024,1,2); fp2.price = 2.0;
    model.AddEntry(fp1);
    model.AddEntry(fp2);

    model.RemoveEntry(0);
    QCOMPARE(model.count(), 1);
    QCOMPARE(model.prices().at(0).fuel_type, QString("Б"));
  }

  void removeEntry_invalidIndex_throws() {
    FuelModel model;
    QVERIFY_THROWS_EXCEPTION(std::out_of_range, model.RemoveEntry(0));
    QVERIFY_THROWS_EXCEPTION(std::out_of_range, model.RemoveEntry(-1));

    FuelPrice fp; fp.fuel_type = "X"; fp.date = QDate(2024,1,1); fp.price = 1.0;
    model.AddEntry(fp);
    QVERIFY_THROWS_EXCEPTION(std::out_of_range, model.RemoveEntry(1));
  }
};

QTEST_MAIN(TestFuelModel)
#include "test_fuel_model.moc"
