#pragma once

#include <QWidget>

#include "models/loglevel.h"

class QTextEdit;

class ConsoleWidget : public QWidget
{
    Q_OBJECT

public:

    explicit ConsoleWidget(QWidget *parent = nullptr);

    void logMessage(LogLevel level, const QString &text);

    void log(const QString &text);

    void warning(const QString &text);

    void error(const QString &text);

    void debug(const QString &text);

private:

    QString levelTag(LogLevel level);
    QString colorForLevel(LogLevel level);

    QTextEdit *console;
};
