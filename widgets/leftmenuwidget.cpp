#include "widgets/leftmenuwidget.h"

#include <QPushButton>
#include <QVBoxLayout>

LeftMenuWidget::LeftMenuWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumWidth(80);
    setMaximumWidth(100);

    auto *layout = new QVBoxLayout(this);

    connectionButton = new QPushButton("Connection");
    messagesButton = new QPushButton("Messages");
    exportButton = new QPushButton("Export to csv");
    datasetButton = new QPushButton("Datasets");

    layout->addWidget(connectionButton);
    layout->addWidget(messagesButton);
    layout->addWidget(exportButton);
    layout->addWidget(datasetButton);

    layout->addStretch();

    connect(connectionButton,&QPushButton::clicked,
            this,&LeftMenuWidget::connectionClicked);

    connect(messagesButton,&QPushButton::clicked,
            this,&LeftMenuWidget::messagesClicked);

    connect(exportButton,&QPushButton::clicked,
            this,&LeftMenuWidget::exportClicked);

    connect(datasetButton,&QPushButton::clicked,
            this,&LeftMenuWidget::datasetClicked);
}
