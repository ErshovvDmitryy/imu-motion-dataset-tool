#include "widgets/consolewidget.h"

#include <QTextEdit>
#include <QVBoxLayout>
#include <QTime>

ConsoleWidget::ConsoleWidget(QWidget *parent)
    : QWidget(parent)
{
    setMaximumHeight(180);

    console = new QTextEdit;

    console->setReadOnly(true);

    auto *layout = new QVBoxLayout(this);

    layout->addWidget(console);
}

void ConsoleWidget::logMessage(LogLevel level, const QString &text)
{
    const QString time = QTime::currentTime().toString("HH:mm:ss.zzz");
    const QString color = colorForLevel(level);
    const QString tag = levelTag(level);

    console->append(QString("<font color=\"%1\">[%2] %3:</font> %4")
                        .arg(color, time, tag, text.toHtmlEscaped()));
}

void ConsoleWidget::log(const QString &text)
{
    logMessage(LogLevel::Info, text);
}

void ConsoleWidget::warning(const QString &text)
{
    logMessage(LogLevel::Warning, text);
}

void ConsoleWidget::error(const QString &text)
{
    logMessage(LogLevel::Error, text);
}

void ConsoleWidget::debug(const QString &text)
{
    logMessage(LogLevel::Debug, text);
}

QString ConsoleWidget::levelTag(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Info:    return "INFO";
    case LogLevel::Warning: return "WARNING";
    case LogLevel::Error:   return "ERROR";
    case LogLevel::Debug:   return "DEBUG";
    }

    return "INFO";
}

QString ConsoleWidget::colorForLevel(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Info:    return "#c0c0c0";
    case LogLevel::Warning: return "#ffcc00";
    case LogLevel::Error:   return "#ff5555";
    case LogLevel::Debug:   return "#66ccff";
    }

    return "#c0c0c0";
}
