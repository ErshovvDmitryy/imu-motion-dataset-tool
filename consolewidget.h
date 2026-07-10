#pragma once

#include <QWidget>

class QTextEdit;

class ConsoleWidget : public QWidget
{
    Q_OBJECT

public:

    explicit ConsoleWidget(QWidget *parent = nullptr);

    void log(const QString &text);

    void warning(const QString &text);

    void error(const QString &text);

private:

    QTextEdit *console;
};
