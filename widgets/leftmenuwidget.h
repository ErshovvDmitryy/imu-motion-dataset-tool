#pragma once

#include <QWidget>

class QPushButton;
class QVBoxLayout;

class LeftMenuWidget : public QWidget
{
    Q_OBJECT

public:

    explicit LeftMenuWidget(QWidget *parent = nullptr);

signals:

    void connectionClicked();

    void liveClicked();

    void sessionClicked();

    void datasetClicked();

private:

    QPushButton *connectionButton;

    QPushButton *liveButton;

    QPushButton *sessionButton;

    QPushButton *datasetButton;
};
