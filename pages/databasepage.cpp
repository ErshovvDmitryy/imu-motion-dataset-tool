#include "pages/databasepage.h"
#include "core/databasemanager.h"

#include <QPushButton>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QTreeView>
#include <QStandardItemModel>
#include <QStringList>
#include <QDebug>
#include <QMessageBox>
#include <QFileInfo>

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

void DataBasePage::createWidgets()
{

    mainLayout = new QHBoxLayout(this);
    leftLayout = new QVBoxLayout();
    rigthLayout = new QVBoxLayout ();

    btnOpenDB = new QPushButton("Open DB");
    btnDeleteDB = new QPushButton("Delete DB");
    btnEditDB = new QPushButton("Edit DB");
    btnCreateDB = new QPushButton("Create DB");

    btnFillRight = new QPushButton("Fill element");
    btnFillRight->setMinimumSize(700, 400);

    treeView = new QTreeView();
    treeModel = new QStandardItemModel();
    treeModel->setHorizontalHeaderLabels({"Name", "Table"});
    treeView->setModel(treeModel);
    treeView->setHeaderHidden(false);

    if (!m_dbManager->isInitialized()) {
        btnOpenDB->setEnabled(false);
        btnDeleteDB->setEnabled(false);
        btnEditDB->setEnabled(false);
        btnCreateDB->setEnabled(false);
    }
}

void DataBasePage::createLayouts()
{

    leftLayout->addWidget(btnOpenDB);
    leftLayout->addWidget(btnDeleteDB);
    leftLayout->addWidget(btnEditDB);
    leftLayout->addWidget(btnCreateDB);
    leftLayout->addWidget(treeView);

    rigthLayout->addWidget(btnFillRight);
    rigthLayout->addStretch();

    mainLayout->addLayout(leftLayout);
    mainLayout->addLayout(rigthLayout);

}

void DataBasePage::connectSignals()
{
    connect(btnOpenDB,
            &QPushButton::clicked,
            this,
            &DataBasePage::openDBDialog);

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

void DataBasePage::refreshDatabaseList()
{
    treeModel->removeRows(0, treeModel->rowCount());

    QStringList databases = m_dbManager->getRegisteredDatabases();

    for (const QString &dbName : databases) {
        QList<QStandardItem*> row;
        row.append(new QStandardItem(dbName));
        row.append(new QStandardItem(m_dbManager->getDatabaseDescription(dbName)));
        treeModel->appendRow(row);
    }
}
