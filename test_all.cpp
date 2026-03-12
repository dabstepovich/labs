#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTextStream>
#include <QtTest/QtTest>

#include "command_processor.h"
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
    FuelPrice fp =
        FuelModel::ParseLine(R"("Дизельное топливо" 2023.11.01 72.10)");
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
    QVERIFY_THROWS_EXCEPTION(ParseException, FuelModel::ParseLine(""));
  }

  void parseLine_missingQuotes_throws() {
    QVERIFY_THROWS_EXCEPTION(ParseException,
                             FuelModel::ParseLine("АИ-95 2024.03.15 58.40"));
  }

  void parseLine_onlyOpenQuote_throws() {
    QVERIFY_THROWS_EXCEPTION(
        ParseException, FuelModel::ParseLine(R"x('АИ-95 2024.03.15 58.40)x"));
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
    QVERIFY_THROWS_EXCEPTION(
        ParseException, FuelModel::ParseLine(R"("АИ-95" 15.03.2024 58.40)"));
  }

  void parseLine_invalidPrice_throws() {
    QVERIFY_THROWS_EXCEPTION(ParseException,
                             FuelModel::ParseLine(R"("АИ-95" 2024.03.15 abc)"));
  }

  void parseLine_negativePrice_throws() {
    QVERIFY_THROWS_EXCEPTION(
        ParseException, FuelModel::ParseLine(R"("АИ-95" 2024.03.15 -5.00)"));
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
      out << R"("АИ-95" 2024.01.10 58.00)" << '\n';
      out << "плохая строка без кавычек" << '\n';
      out << R"("АИ-92" 15.03.2024 54.50)" << '\n';
      out << R"("Дизель" 2024.01.10 70.20)" << '\n';
      out << R"("" 2024.05.01 60.00)" << '\n';
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
    QVERIFY_THROWS_EXCEPTION(
        std::runtime_error,
        model.LoadFromFile("/tmp/no_such_file_xyz_12345.txt"));
  }

  void saveAndLoad_roundTrip() {
    FuelModel save;
    FuelPrice fp1;
    fp1.fuel_type = "АИ-95";
    fp1.date = QDate(2024, 3, 1);
    fp1.price = 58.0;
    FuelPrice fp2;
    fp2.fuel_type = "Дизель";
    fp2.date = QDate(2024, 4, 15);
    fp2.price = 72.5;
    save.AddEntry(fp1);
    save.AddEntry(fp2);

    QTemporaryFile tmp;
    QVERIFY(tmp.open());
    tmp.close();
    save.SaveToFile(tmp.fileName());

    FuelModel load;
    QStringList errors;
    int loaded = load.LoadFromFile(tmp.fileName(), &errors);

    QCOMPARE(loaded, 2);
    QCOMPARE(errors.size(), 0);
    QCOMPARE(load.prices().at(0).fuel_type, QString("АИ-95"));
    QCOMPARE(load.prices().at(0).date, QDate(2024, 3, 1));
    QCOMPARE(load.prices().at(0).price, 58.0);
    QCOMPARE(load.prices().at(1).fuel_type, QString("Дизель"));
    QCOMPARE(load.prices().at(1).price, 72.5);
  }

  void saveToFile_invalidPath_throws() {
    FuelModel model;
    QVERIFY_THROWS_EXCEPTION(std::runtime_error,
                             model.SaveToFile("/no_such_dir_xyz/file.txt"));
  }

  void addEntry_increasesCount() {
    FuelModel model;
    QCOMPARE(model.count(), 0);
    FuelPrice fp;
    fp.fuel_type = "АИ-98";
    fp.date = QDate(2024, 1, 1);
    fp.price = 80.0;
    model.AddEntry(fp);
    QCOMPARE(model.count(), 1);
    model.AddEntry(fp);
    QCOMPARE(model.count(), 2);
  }

  void removeEntry_validIndex() {
    FuelModel model;
    FuelPrice fp1;
    fp1.fuel_type = "А";
    fp1.date = QDate(2024, 1, 1);
    fp1.price = 1.0;
    FuelPrice fp2;
    fp2.fuel_type = "Б";
    fp2.date = QDate(2024, 1, 2);
    fp2.price = 2.0;
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
    FuelPrice fp;
    fp.fuel_type = "X";
    fp.date = QDate(2024, 1, 1);
    fp.price = 1.0;
    model.AddEntry(fp);
    QVERIFY_THROWS_EXCEPTION(std::out_of_range, model.RemoveEntry(1));
  }

  void removeIf_removesByPricePredicate() {
    FuelModel model;
    FuelPrice fp1;
    fp1.fuel_type = "А";
    fp1.date = QDate(2024, 1, 1);
    fp1.price = 50.0;
    FuelPrice fp2;
    fp2.fuel_type = "Б";
    fp2.date = QDate(2024, 1, 2);
    fp2.price = 80.0;
    FuelPrice fp3;
    fp3.fuel_type = "В";
    fp3.date = QDate(2024, 1, 3);
    fp3.price = 30.0;
    model.AddEntry(fp1);
    model.AddEntry(fp2);
    model.AddEntry(fp3);

    int removed =
        model.RemoveIf([](const FuelPrice &fp) { return fp.price < 55.0; });
    QCOMPARE(removed, 2);
    QCOMPARE(model.count(), 1);
    QCOMPARE(model.prices().at(0).fuel_type, QString("Б"));
  }

  void removeIf_nothingRemoved_whenNoMatch() {
    FuelModel model;
    FuelPrice fp;
    fp.fuel_type = "А";
    fp.date = QDate(2024, 1, 1);
    fp.price = 100.0;
    model.AddEntry(fp);
    int removed =
        model.RemoveIf([](const FuelPrice &fp) { return fp.price < 50.0; });
    QCOMPARE(removed, 0);
    QCOMPARE(model.count(), 1);
  }

  void removeIf_emptyModel_returnsZero() {
    FuelModel model;
    int removed = model.RemoveIf([](const FuelPrice &) { return true; });
    QCOMPARE(removed, 0);
  }
};

class TestCommandProcessor : public QObject {
  Q_OBJECT

  static FuelModel makeModel() {
    FuelModel m;
    FuelPrice fp1;
    fp1.fuel_type = "АИ-95";
    fp1.date = QDate(2024, 1, 10);
    fp1.price = 58.0;
    FuelPrice fp2;
    fp2.fuel_type = "АИ-92";
    fp2.date = QDate(2024, 2, 15);
    fp2.price = 52.0;
    FuelPrice fp3;
    fp3.fuel_type = "Дизель";
    fp3.date = QDate(2024, 3, 20);
    fp3.price = 72.0;
    m.AddEntry(fp1);
    m.AddEntry(fp2);
    m.AddEntry(fp3);
    return m;
  }

private slots:

  void parseAdd_validBasic() {
    FuelPrice fp =
        CommandProcessor::ParseAddArgument("АИ-95; 2024.03.15; 58.40");
    QCOMPARE(fp.fuel_type, QString("АИ-95"));
    QCOMPARE(fp.date, QDate(2024, 3, 15));
    QCOMPARE(fp.price, 58.40);
  }

  void parseAdd_spacesAroundSemicolon() {
    FuelPrice fp = CommandProcessor::ParseAddArgument(
        "  Дизель  ;  2024.06.01  ;  71.00  ");
    QCOMPARE(fp.fuel_type, QString("Дизель"));
    QCOMPARE(fp.price, 71.0);
  }

  void parseAdd_tooFewFields_throws() {
    QVERIFY_THROWS_EXCEPTION(ParseException, CommandProcessor::ParseAddArgument(
                                                 "АИ-95; 2024.03.15"));
  }

  void parseAdd_emptyFuelType_throws() {
    QVERIFY_THROWS_EXCEPTION(ParseException, CommandProcessor::ParseAddArgument(
                                                 "; 2024.03.15; 58.40"));
  }

  void parseAdd_invalidDate_throws() {
    QVERIFY_THROWS_EXCEPTION(ParseException, CommandProcessor::ParseAddArgument(
                                                 "АИ-95; 15.03.2024; 58.40"));
  }

  void parseAdd_invalidPrice_throws() {
    QVERIFY_THROWS_EXCEPTION(ParseException, CommandProcessor::ParseAddArgument(
                                                 "АИ-95; 2024.03.15; abc"));
  }

  void parseAdd_negativePrice_throws() {
    QVERIFY_THROWS_EXCEPTION(ParseException, CommandProcessor::ParseAddArgument(
                                                 "АИ-95; 2024.03.15; -10"));
  }

  void parseRem_price_lt() {
    auto pred = CommandProcessor::ParseRemCondition("price < 60");
    FuelPrice cheap;
    cheap.price = 50.0;
    FuelPrice expensive;
    expensive.price = 70.0;
    QVERIFY(pred(cheap));
    QVERIFY(!pred(expensive));
  }

  void parseRem_price_gte() {
    auto pred = CommandProcessor::ParseRemCondition("price >= 72.0");
    FuelPrice fp1;
    fp1.price = 72.0;
    FuelPrice fp2;
    fp2.price = 71.99;
    QVERIFY(pred(fp1));
    QVERIFY(!pred(fp2));
  }

  void parseRem_price_eq() {
    auto pred = CommandProcessor::ParseRemCondition("price == 58.0");
    FuelPrice fp;
    fp.price = 58.0;
    QVERIFY(pred(fp));
    fp.price = 57.9;
    QVERIFY(!pred(fp));
  }

  void parseRem_price_ne() {
    auto pred = CommandProcessor::ParseRemCondition("price != 58.0");
    FuelPrice fp;
    fp.price = 58.0;
    QVERIFY(!pred(fp));
    fp.price = 57.9;
    QVERIFY(pred(fp));
  }

  void parseRem_date_lt() {
    auto pred = CommandProcessor::ParseRemCondition("date < 2024.02.01");
    FuelPrice fp1;
    fp1.date = QDate(2024, 1, 10);
    FuelPrice fp2;
    fp2.date = QDate(2024, 3, 1);
    QVERIFY(pred(fp1));
    QVERIFY(!pred(fp2));
  }

  void parseRem_date_gt() {
    auto pred = CommandProcessor::ParseRemCondition("date > 2024.02.01");
    FuelPrice fp1;
    fp1.date = QDate(2024, 3, 1);
    FuelPrice fp2;
    fp2.date = QDate(2024, 1, 1);
    QVERIFY(pred(fp1));
    QVERIFY(!pred(fp2));
  }

  void parseRem_fuelType_eq() {
    auto pred = CommandProcessor::ParseRemCondition("fuel_type == АИ-95");
    FuelPrice fp1;
    fp1.fuel_type = "АИ-95";
    FuelPrice fp2;
    fp2.fuel_type = "АИ-92";
    QVERIFY(pred(fp1));
    QVERIFY(!pred(fp2));
  }

  void parseRem_fuelType_ne() {
    auto pred = CommandProcessor::ParseRemCondition("fuel_type != Дизель");
    FuelPrice fp1;
    fp1.fuel_type = "АИ-95";
    FuelPrice fp2;
    fp2.fuel_type = "Дизель";
    QVERIFY(pred(fp1));
    QVERIFY(!pred(fp2));
  }

  void parseRem_invalidCondition_throws() {
    QVERIFY_THROWS_EXCEPTION(
        ParseException,
        CommandProcessor::ParseRemCondition("no condition here!!!"));
  }

  void parseRem_unknownField_throws() {
    QVERIFY_THROWS_EXCEPTION(
        ParseException,
        CommandProcessor::ParseRemCondition("unknown_field < 10"));
  }

  void parseRem_fuelType_relationalOp_throws() {
    QVERIFY_THROWS_EXCEPTION(
        ParseException,
        CommandProcessor::ParseRemCondition("fuel_type < АИ-95"));
  }

  void parseRem_invalidDateValue_throws() {
    QVERIFY_THROWS_EXCEPTION(
        ParseException,
        CommandProcessor::ParseRemCondition("date < not-a-date"));
  }

  void parseRem_invalidPriceValue_throws() {
    QVERIFY_THROWS_EXCEPTION(
        ParseException, CommandProcessor::ParseRemCondition("price < abc"));
  }

  void executeLine_add_success() {
    FuelModel model;
    CommandProcessor cp(model);
    CommandResult r = cp.ExecuteLine("ADD АИ-95; 2024.05.01; 60.00");
    QVERIFY(r.success);
    QCOMPARE(model.count(), 1);
    QCOMPARE(model.prices().at(0).fuel_type, QString("АИ-95"));
  }

  void executeLine_add_badArgs_failure() {
    FuelModel model;
    CommandProcessor cp(model);
    CommandResult r = cp.ExecuteLine("ADD bad_csv_no_semicolons");
    QVERIFY(!r.success);
    QVERIFY(!r.error.isEmpty());
    QCOMPARE(model.count(), 0);
  }

  void executeLine_rem_removesMatchingRows() {
    FuelModel model = makeModel();
    CommandProcessor cp(model);
    CommandResult r = cp.ExecuteLine("REM price < 60");
    QVERIFY(r.success);
    QCOMPARE(model.count(), 1);
    QCOMPARE(model.prices().at(0).fuel_type, QString("Дизель"));
  }

  void executeLine_rem_badCondition_failure() {
    FuelModel model;
    CommandProcessor cp(model);
    CommandResult r = cp.ExecuteLine("REM ???");
    QVERIFY(!r.success);
  }

  void executeLine_save_callsSaveFn() {
    FuelModel model;
    bool called = false;
    QString savedPath;
    CommandProcessor cp(model, [&](FuelModel &, const QString &path) {
      called = true;
      savedPath = path;
    });
    CommandResult r = cp.ExecuteLine("SAVE /tmp/test_output.txt");
    QVERIFY(r.success);
    QVERIFY(called);
    QCOMPARE(savedPath, QString("/tmp/test_output.txt"));
  }

  void executeLine_save_noPath_failure() {
    FuelModel model;
    CommandProcessor cp(model);
    CommandResult r = cp.ExecuteLine("SAVE");
    QVERIFY(!r.success);
  }

  void executeLine_save_saveFnThrows_failure() {
    FuelModel model;
    CommandProcessor cp(model, [](FuelModel &, const QString &) {
      throw std::runtime_error("disk full");
    });
    CommandResult r = cp.ExecuteLine("SAVE /some/path.txt");
    QVERIFY(!r.success);
    QVERIFY(r.error.contains("disk full"));
  }

  void executeLine_unknownCommand_failure() {
    FuelModel model;
    CommandProcessor cp(model);
    CommandResult r = cp.ExecuteLine("UNKNOWN something");
    QVERIFY(!r.success);
  }

  void executeLine_emptyLine_success() {
    FuelModel model;
    CommandProcessor cp(model);
    CommandResult r = cp.ExecuteLine("   ");
    QVERIFY(r.success);
  }

  void executeLine_caseInsensitiveKeyword() {
    FuelModel model;
    CommandProcessor cp(model);
    CommandResult r = cp.ExecuteLine("add АИ-95; 2024.05.01; 60.00");
    QVERIFY(r.success);
    QCOMPARE(model.count(), 1);
  }

  void executeFile_mixedCommands() {
    FuelModel model = makeModel();
    QTemporaryFile tmp;
    QVERIFY(tmp.open());
    {
      QTextStream out(&tmp);
      out.setEncoding(QStringConverter::Utf8);
      out << "ADD Газ; 2024.07.01; 35.00\n";
      out << "REM price < 55\n";
      out << "# комментарий — пропустить\n";
      out << "\n";
      out << "ADD Керосин; 2024.08.15; 90.00\n";
    }
    tmp.close();

    bool save_called = false;
    CommandProcessor cp(
        model, [&](FuelModel &, const QString &) { save_called = true; });
    BatchResult result = cp.ExecuteFile(tmp.fileName());

    QCOMPARE(result.executed, 3);
    QCOMPARE(result.failed, 0);
    QCOMPARE(model.count(), 3);
    QVERIFY(!save_called);
  }

  void executeFile_withSaveCommand() {
    FuelModel model = makeModel();

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString save_path = dir.path() + "/out.txt";

    QTemporaryFile cmd_file;
    QVERIFY(cmd_file.open());
    {
      QTextStream out(&cmd_file);
      out.setEncoding(QStringConverter::Utf8);
      out << "REM price > 100\n";
      out << "SAVE " << save_path << "\n";
    }
    cmd_file.close();

    CommandProcessor cp(model);
    BatchResult result = cp.ExecuteFile(cmd_file.fileName());

    QCOMPARE(result.executed, 2);
    QCOMPARE(result.failed, 0);
    QVERIFY(QFile::exists(save_path));
  }

  void executeFile_withErrors_continuesExecution() {
    FuelModel model;
    QTemporaryFile tmp;
    QVERIFY(tmp.open());
    {
      QTextStream out(&tmp);
      out.setEncoding(QStringConverter::Utf8);
      out << "ADD АИ-95; 2024.01.01; 58.00\n";
      out << "UNKNOWN cmd\n";
      out << "ADD Дизель; 2024.02.01; 70.00\n";
      out << "REM price < abc\n";
    }
    tmp.close();

    CommandProcessor cp(model);
    BatchResult result = cp.ExecuteFile(tmp.fileName());

    QCOMPARE(result.executed, 2);
    QCOMPARE(result.failed, 2);
    QCOMPARE(model.count(), 2);
  }

  void executeFile_nonexistentFile_throws() {
    FuelModel model;
    CommandProcessor cp(model);
    QVERIFY_THROWS_EXCEPTION(
        std::runtime_error,
        cp.ExecuteFile("/tmp/no_such_commands_file_xyz.txt"));
  }

  void batchResult_summaryLines() {
    BatchResult r;
    r.results << CommandResult{true, "Добавлена запись: X", {}};
    r.results << CommandResult{false, {}, "Неверная команда"};
    r.executed = 1;
    r.failed = 1;

    QStringList lines = r.summaryLines();
    QCOMPARE(lines.size(), 2);
    QVERIFY(lines[0].contains("OK"));
    QVERIFY(lines[1].contains("ОШИБКА"));
  }

  void parseFilter_validArgs() {
    auto [sub, path] =
        CommandProcessor::ParseFilterArgument("\"АИ\" \"/tmp/out.txt\"");
    QCOMPARE(sub, QString("АИ"));
    QCOMPARE(path, QString("/tmp/out.txt"));
  }

  void parseFilter_substringWithSpaces() {
    auto [sub, path] = CommandProcessor::ParseFilterArgument(
        "\"Дизельное топливо\" \"/tmp/out.txt\"");
    QCOMPARE(sub, QString("Дизельное топливо"));
    QCOMPARE(path, QString("/tmp/out.txt"));
  }

  void parseFilter_pathWithSpaces() {
    auto [sub, path] =
        CommandProcessor::ParseFilterArgument("\"АИ\" \"/tmp/my file.txt\"");
    QCOMPARE(sub, QString("АИ"));
    QCOMPARE(path, QString("/tmp/my file.txt"));
  }

  void parseFilter_emptySubstring_valid() {
    auto [sub, path] =
        CommandProcessor::ParseFilterArgument("\"\" \"/tmp/out.txt\"");
    QCOMPARE(sub, QString(""));
    QCOMPARE(path, QString("/tmp/out.txt"));
  }

  void parseFilter_emptyArgs_throws() {
    QVERIFY_THROWS_EXCEPTION(ParseException,
                             CommandProcessor::ParseFilterArgument(""));
  }

  void parseFilter_noOpenQuote_throws() {
    QVERIFY_THROWS_EXCEPTION(
        ParseException,
        CommandProcessor::ParseFilterArgument("АИ \"/tmp/out.txt\""));
  }

  void parseFilter_noCloseQuoteSubstring_throws() {
    QVERIFY_THROWS_EXCEPTION(
        ParseException,
        CommandProcessor::ParseFilterArgument("\"АИ \"/tmp/out.txt\""));
  }

  void parseFilter_pathNotQuoted_throws() {
    QVERIFY_THROWS_EXCEPTION(
        ParseException,
        CommandProcessor::ParseFilterArgument("\"АИ\" /tmp/out.txt"));
  }

  void parseFilter_noCloseQuotePath_throws() {
    QVERIFY_THROWS_EXCEPTION(
        ParseException,
        CommandProcessor::ParseFilterArgument("\"АИ\" \"/tmp/out.txt"));
  }

  void parseFilter_emptyPath_throws() {
    QVERIFY_THROWS_EXCEPTION(
        ParseException, CommandProcessor::ParseFilterArgument("\"АИ\" \"\""));
  }

  void parseFilter_noFilepath_throws() {
    QVERIFY_THROWS_EXCEPTION(ParseException,
                             CommandProcessor::ParseFilterArgument("\"АИ\""));
  }

  void executeFilter_badArgs_failure() {
    FuelModel model;
    CommandProcessor cp(model);
    CommandResult r = cp.ExecuteLine("FILTER");
    QVERIFY(!r.success);
    QVERIFY(!r.error.isEmpty());
  }

  void executeFilter_invalidPath_failure() {
    FuelModel model = makeModel();
    CommandProcessor cp(model);
    CommandResult r =
        cp.ExecuteLine("FILTER \"АИ\" \"/no_such_dir_xyz/out.txt\"");
    QVERIFY(!r.success);
  }
};

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);

  int status = 0;
  {
    TestFuelModel t1;
    status |= QTest::qExec(&t1, argc, argv);
  }
  {
    TestCommandProcessor t2;
    status |= QTest::qExec(&t2, argc, argv);
  }
  return status;
}

#include "test_all.moc"
