#include "ConsoleWidget.h"

#include <QTextEdit>
#include <QVBoxLayout>

ConsoleWidget::ConsoleWidget(QWidget *parent)
    : QWidget(parent)
{
    setMaximumHeight(180);

    console = new QTextEdit;

    console->setReadOnly(true);

    auto *layout = new QVBoxLayout(this);

    layout->addWidget(console);
}

void ConsoleWidget::log(const QString &text)
{
    console->append("[INFO] " + text);
}

void ConsoleWidget::warning(const QString &text)
{
    console->append("[WARNING] " + text);
}

void ConsoleWidget::error(const QString &text)
{
    console->append("[ERROR] " + text);
}
