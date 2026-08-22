#include "widgets/leftmenuwidget.h"

#include <QPushButton>
#include <QVBoxLayout>

LeftMenuWidget::LeftMenuWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumWidth(180);

    auto *layout = new QVBoxLayout(this);

    connectionButton = new QPushButton("Connection");
    liveButton = new QPushButton("Live Stream");
    sessionButton = new QPushButton("Session");
    datasetButton = new QPushButton("Datasets");

    layout->addWidget(connectionButton);
    layout->addWidget(liveButton);
    layout->addWidget(sessionButton);
    layout->addWidget(datasetButton);

    layout->addStretch();

    connect(connectionButton,&QPushButton::clicked,
            this,&LeftMenuWidget::connectionClicked);

    connect(liveButton,&QPushButton::clicked,
            this,&LeftMenuWidget::liveClicked);

    connect(sessionButton,&QPushButton::clicked,
            this,&LeftMenuWidget::sessionClicked);

    connect(datasetButton,&QPushButton::clicked,
            this,&LeftMenuWidget::datasetClicked);
}
