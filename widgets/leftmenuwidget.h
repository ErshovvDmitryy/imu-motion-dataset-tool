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

    void messagesClicked();

    void exportClicked();

    void datasetClicked();

private:

    QPushButton *connectionButton;

    QPushButton *messagesButton;

    QPushButton *exportButton;

    QPushButton *datasetButton;
};
