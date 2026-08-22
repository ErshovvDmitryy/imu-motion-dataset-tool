#pragma once

#include <QWidget>
#include <models/statusport.h>

class QLabel;

class StatusBarWidget : public QWidget
{
    Q_OBJECT

public:

    explicit StatusBarWidget(QWidget *parent = nullptr);

    void setStatus(const QString &text);

private:

    QLabel *statusLabel;
};
