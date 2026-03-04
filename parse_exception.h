#ifndef PARSE_EXCEPTION_H
#define PARSE_EXCEPTION_H

#include <stdexcept>
#include <QString>

class ParseException : public std::runtime_error {
 public:
  explicit ParseException(const QString &message)
      : std::runtime_error(message.toStdString()),
        message_(message) {}

  const QString &qwhat() const noexcept { return message_; }

 private:
  QString message_;
};

#endif 
