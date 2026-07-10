#include "StatusBarWidget.h"

#include <QHBoxLayout>
#include <QLabel>

StatusBarWidget::StatusBarWidget(QWidget *parent)
    : QWidget(parent)
{
    setMaximumHeight(30);

    auto *layout = new QHBoxLayout(this);

    statusLabel = new QLabel("Disconnected");

    layout->addWidget(statusLabel);

    layout->addStretch();
}

void StatusBarWidget::setStatus(const QString &text)
{
    statusLabel->setText(text);
}
