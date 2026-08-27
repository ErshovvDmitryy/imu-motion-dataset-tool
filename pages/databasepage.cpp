#include "pages/databasepage.h"
#include "core/databasemanager.h"

#include "qcustomplot/qcustomplot.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTreeView>
#include <QStandardItemModel>
#include <QStringList>
#include <QMessageBox>
#include <QFileInfo>
#include <QSplitter>
#include <QLineEdit>
#include <QTableView>
#include <QSqlTableModel>

DataBasePage::DataBasePage(DatabaseManager *dbManager, QWidget *parent)
    : QWidget(parent)
    , m_dbManager(dbManager)
    , m_currentDatabase(QString())
{
    createWidgets();
    createLayouts();
    connectSignals();

    if (m_dbManager->isInitialized()) {
        refreshDatabaseList();
    }
}

void DataBasePage::createWidgets() {

    mainLayout = new QHBoxLayout(this);

    leftLayout = new QVBoxLayout();
    rigthLayout = new QVBoxLayout();

    leftWidget = new QWidget();
    rightWidget = new QWidget();
    leftWidget->setMinimumWidth(250);
    leftWidget->setMaximumWidth(500);

    mainSplitter = new QSplitter(Qt::Horizontal, this);

    samplesModel = new QSqlTableModel(this);
    motionDataModel = new QSqlTableModel(this);

// =================== LEFT SIDE

    graphLayout = new QHBoxLayout();

    btnOpenDB = new QPushButton("Open DB");
    btnDeleteDB = new QPushButton("Delete DB");
    btnEditDB = new QPushButton("Edit DB");
    btnCreateDB = new QPushButton("Create DB");
    btnRefreshDB = new QPushButton("Refresh DB");

    treeView = new QTreeView();

    treeModel = new QStandardItemModel();

    treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    treeModel->setHorizontalHeaderLabels({"Name", "Count writes"});
    treeView->setModel(treeModel);
    treeView->setHeaderHidden(false);

    treeView->resizeColumnToContents(0);
    treeView->resizeColumnToContents(1);

    parentItem = new QStandardItem();
    parentItem = treeModel->invisibleRootItem();

    if (!m_dbManager->isInitialized()) {
        btnOpenDB->setEnabled(false);
        btnDeleteDB->setEnabled(false);
        btnEditDB->setEnabled(false);
        btnCreateDB->setEnabled(false);
    }

// =================== RIGHT SIDE

    sampleDo = new QHBoxLayout();
    pathLayout = new QHBoxLayout();
    samplesArea = new QHBoxLayout();

    pathArea = new QHBoxLayout();
    pathAreaRight = new QVBoxLayout();
    pathAreaLeft = new QVBoxLayout();

    areaBtnGraph = new QHBoxLayout();

    labelStatsChanged = new QLabel("Total added: %1");

    samplesAreaRight = new QVBoxLayout();

    m_pathEdit = new QLineEdit;
    m_pathEdit->setPlaceholderText("Selected file path will appear here...");
    m_pathEdit->setReadOnly(true);

    samplesView = new QTableView();
    samplesView->setModel(samplesModel);
    samplesView->setSelectionMode(QAbstractItemView::MultiSelection);
    samplesView->setSelectionBehavior(QAbstractItemView::SelectRows);
    samplesView->horizontalHeader()->setStretchLastSection(true);
    samplesView->verticalHeader()->setVisible(false);
    samplesView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    motionDataView = new QTableView();
    motionDataView->setModel(motionDataModel);
    motionDataView->setSelectionMode(QAbstractItemView::SingleSelection);
    motionDataView->setSelectionBehavior(QAbstractItemView::SelectRows);
    motionDataView->horizontalHeader()->setStretchLastSection(true);
    motionDataView->verticalHeader()->setVisible(false);
    motionDataView->setEditTriggers(QAbstractItemView::AllEditTriggers);

    gyroGraph = new QCustomPlot();
    accelGraph = new QCustomPlot();
    gyroGraph->setMinimumSize(200, 200);
    accelGraph->setMinimumSize(200, 200);

    btnRefreshGraph = new QPushButton("Refresh");
    btnCutGraph = new QPushButton("Cut");
    btnExportToDataSet = new QPushButton("Export this");
    btnExportAllToDataSet = new QPushButton("Export all");
    btnDeleteFromDataSet = new QPushButton("Remove from data set pack");
    btnSetPath = new QPushButton("Set path");
    btnNextSample = new QPushButton("Next sample");
    btnUndoSample = new QPushButton("Undo sample");

    gyroGraph->xAxis->setLabel("Time (s)");
    gyroGraph->yAxis->setLabel("Gyro (°/с)");
    accelGraph->xAxis->setLabel("Time (s)");
    accelGraph->yAxis->setLabel("Accel (g)");

    for (int i = 0; i < 3; i++){
        gyroGraph->addGraph();
        accelGraph->addGraph();
    }

    gyroGraph->graph(0)->setPen(QPen(Qt::red));
    accelGraph->graph(0)->setPen(QPen(Qt::red));

    gyroGraph->graph(1)->setPen(QPen(Qt::green));
    accelGraph->graph(1)->setPen(QPen(Qt::green));

    gyroGraph->graph(2)->setPen(QPen(Qt::blue));
    accelGraph->graph(2)->setPen(QPen(Qt::blue));


}

void DataBasePage::createLayouts() {

// =================== LEFT SIDE

    leftLayout->addWidget(btnOpenDB);
    leftLayout->addWidget(btnDeleteDB);
    leftLayout->addWidget(btnEditDB);
    leftLayout->addWidget(btnCreateDB);
    leftLayout->addWidget(btnRefreshDB);
    leftLayout->addWidget(treeView);

// =================== RIGHT SIDE

    pathLayout->addWidget(m_pathEdit);
    pathLayout->addWidget(btnSetPath);

    sampleDo->addWidget(btnExportToDataSet);
    sampleDo->addWidget(btnExportAllToDataSet);
    sampleDo->addWidget(btnDeleteFromDataSet);

    pathAreaLeft->addLayout(pathLayout);
    pathAreaLeft->addLayout(sampleDo);
    pathAreaRight->addWidget(labelStatsChanged);

    pathArea->addLayout(pathAreaLeft);
    pathArea->addLayout(pathAreaRight);
    pathArea->addStretch();

    samplesArea->addWidget(samplesView);

    graphLayout->addWidget(accelGraph);
    graphLayout->addWidget(gyroGraph);

    areaBtnGraph->addWidget(btnRefreshGraph);
    areaBtnGraph->addWidget(btnCutGraph);
    areaBtnGraph->addStretch();
    areaBtnGraph->addWidget(btnUndoSample);
    areaBtnGraph->addWidget(btnNextSample);

    rigthLayout->addLayout(pathArea);
    rigthLayout->addLayout(samplesArea);
    rigthLayout->addLayout(graphLayout);
    rigthLayout->addLayout(areaBtnGraph);

    rigthLayout->addWidget(motionDataView);

    rigthLayout->addStretch();

// =================== SETUP PAGE

    leftWidget->setLayout(leftLayout);
    rightWidget->setLayout(rigthLayout);

    mainSplitter->addWidget(leftWidget);
    mainSplitter->addWidget(rightWidget);

    mainLayout->addWidget(mainSplitter);
    setLayout(mainLayout);

}

void DataBasePage::connectSignals() {
    connect(btnRefreshDB,
            &QPushButton::clicked,
            this,
            &DataBasePage::refreshDatabaseList);

    connect(btnOpenDB,
            &QPushButton::clicked,
            this,
            &DataBasePage::openDBDialog);

    connect(treeView, &QTreeView::doubleClicked,
            this, &DataBasePage::onTreeViewDoubleClicked);

    connect(btnCreateDB,
            &QPushButton::clicked,
            this,
            &DataBasePage::createDBDialog);

    connect(btnDeleteDB,
            &QPushButton::clicked,
            this,
            [this]() {
                QModelIndex index = treeView->currentIndex();
                if (!index.isValid()) {
                    QMessageBox::warning(this, "No Selection", "Select a database to delete.");
                    return;
                }

                QString dbName = index.data(Qt::DisplayRole).toString();
                QMessageBox::StandardButton reply = QMessageBox::question(
                    this, "Delete Database",
                    QString("Delete '%1' and its file?").arg(dbName),
                    QMessageBox::Yes | QMessageBox::No);

                if (reply == QMessageBox::Yes) {
                    if (m_dbManager->removeDatabase(dbName, true)) {
                        refreshDatabaseList();
                    } else {
                        QMessageBox::warning(this, "Error", "Failed to delete database.");
                    }
                }
    });
}

void DataBasePage::openDBDialog()
{
    QStringList registeredDbs = m_dbManager->getRegisteredDatabases();

    OpenDBDialog dialog(registeredDbs, this);

    if (dialog.exec() == QDialog::Accepted) {
        QString selected = dialog.getSelectedDatabase();
        if (selected.isEmpty()) {
            return;
        }

        if (dialog.isCustomPath()) {
            QFileInfo fi(selected);
            QString dbName = fi.completeBaseName();

            if (!m_dbManager->getRegisteredDatabases().contains(dbName)) {
                if (!m_dbManager->importDatabase(selected)) {
                    QMessageBox::warning(this, "Error",
                        QString("Failed to import database:\n%1").arg(selected));
                    return;
                }
            }
            m_currentDatabase = dbName;
        } else {
            m_currentDatabase = selected;
        }

        refreshDatabaseList();
    }
}

void DataBasePage::createDBDialog()
{
    CreateDBDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QStringList data = dialog.returnDBdata();
        QString fileName = data[0];
        QString path = data[1];
        QString description = data[2];

        if (m_dbManager->createDatasetDatabase(fileName, path, description)) {
            refreshDatabaseList();
        } else {
            QMessageBox::warning(this, "Error", "Failed to create database.");
        }
    }
}

void DataBasePage::refreshDatabaseList() {
    treeModel->removeRows(0, treeModel->rowCount());

    QStringList databases = m_dbManager->getRegisteredDatabases();

    for (const QString &dbName : databases) {
        QList<QStandardItem*> rowItems;

        QStandardItem *dbNameItem = new QStandardItem(dbName);
        rowItems.append(dbNameItem);

        QStandardItem *countItem = new QStandardItem("0");
        rowItems.append(countItem);

        treeModel->appendRow(rowItems);

        QStringList tableNames = m_dbManager->getTableNames(dbName);

        for (const QString &table : tableNames) {
            QList<QStandardItem*> tableRowItems;

            QStandardItem *tableItem = new QStandardItem(table);
            tableRowItems.append(tableItem);

            tableRowItems.append(new QStandardItem(""));

            int rowCount = m_dbManager->getTableRowCount(dbName, table);
            tableRowItems.append(new QStandardItem(QString::number(rowCount)));

            dbNameItem->appendRow(tableRowItems);

            QStringList columnsInTable = m_dbManager->getTableRowsNames(dbName, table);

            for (const QString &column : columnsInTable) {
                QList<QStandardItem*> columnRowItems;
                columnRowItems.append(new QStandardItem(column));
                columnRowItems.append(new QStandardItem(""));

                tableItem->appendRow(columnRowItems);
            }
        }

        int totalCount = 0;
        for (const QString &table : tableNames) {
            totalCount += m_dbManager->getTableRowCount(dbName, table);
        }
        countItem->setText(QString::number(totalCount));
    }
}

void DataBasePage::refreshSamplesModel(const QString &dbName, const QString &selectTable) {

    samplesView->reset();

    samplesModel = new QSqlTableModel(this, m_dbManager->getDatabase(dbName));
    samplesModel->setTable("samples");
    samplesModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    samplesModel->select();

    motionDataModel = new QSqlTableModel(this, m_dbManager->getDatabase(dbName));
    motionDataModel->setTable("motion_data");
    motionDataModel->setEditStrategy(QSqlTableModel::OnRowChange);
    motionDataModel->select();

    motionDataView->setModel(motionDataModel);
    samplesView->setModel(samplesModel);
}

void DataBasePage::onTreeViewDoubleClicked(const QModelIndex &index) {

    if (!index.isValid()) return;

    QStandardItem *item = treeModel->itemFromIndex(index);
    if (!item) return;

    QString itemText = item->text();

    if ( itemText != "samples" && itemText != "motion_data") return;

    QStandardItem *dbItem = treeModel->itemFromIndex(index.parent());
    QString dbName = dbItem->text();
    QString tableName = itemText;
    refreshSamplesModel(dbName, tableName);
}
