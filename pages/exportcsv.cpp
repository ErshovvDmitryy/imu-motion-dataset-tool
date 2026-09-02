#include "exportcsv.h"

#include "core/databasemanager.h"
#include "models/MotionType.h"

#include <QDebug>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QGroupBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>

#include <QItemSelectionModel>
#include <QSqlRecord>
#include <QSqlError>
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QTableView>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QTreeView>
#include <QSplitter>
#include <QFileInfo>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>


ExportCSV::ExportCSV(DatabaseManager *dbManager, QWidget *parent)
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

void ExportCSV::createWidgets() {

    mainLayout = new QHBoxLayout(this);

    leftLayout = new QVBoxLayout();
    rightLayout = new QVBoxLayout();

    leftWidget = new QWidget();
    rightWidget = new QWidget();

    mainSplitter = new QSplitter(Qt::Horizontal, this);

// ====================== TREE VIEW +++++++++++++++++++++

    treeView = new QTreeView;
    treeModel = new QStandardItemModel;

    updateDB = new QPushButton("Refresh");
    setCustomDB = new QPushButton("Open db");

    m_btnSelectAllFromParent = new QPushButton("Select all in table");
    m_btnUnselectAllFromParent = new QPushButton("Unselect all in select table");

    statLabel = new QLabel("Selected: 0");

    treeModel->setHorizontalHeaderLabels({"Name", "Count"});
    treeView->setModel(treeModel);
    treeView->setHeaderHidden(false);
    treeView->setRootIsDecorated(true);
    treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    treeView->setSelectionBehavior(QAbstractItemView::SelectRows);

    treeView->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    treeView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    treeView->header()->resizeSection(0, 350);

    parentItem = treeModel->invisibleRootItem();

// ====================== EXPORT SETTINGS +++++++++++++++++++++

    rightSplitter = new QSplitter(Qt::Vertical, this);

    QLabel *setupWindowSizeLabel = new QLabel("Window size: ");
    QLabel *setupBiasWindowSizeLabel = new QLabel("Bias window size: ");
    QLabel *setupDescriptionBlockLabel = new QLabel("Write description: ");
    QLabel *setupExportFloderName = new QLabel("Export floder name: ");
    QLabel *setupExportPathFloder = new QLabel("Export path: ");

    m_windowSize = new QLineEdit();
    m_biasWindow = new QLineEdit();
    m_exportPath = new QLineEdit();
    m_exportFloderName = new QLineEdit();
    m_descriptionBlock = new QTextEdit();
    m_descriptionBlock->setMaximumHeight(250);

    m_btnExportPath = new QPushButton("Path");
    btnExport = new QPushButton("Export");
    m_btnRefreshInfo = new QPushButton("Refresh export info");

    topWidget = new QGroupBox("Export settings");
    QGridLayout *topGridLayout = new QGridLayout(topWidget);
    topGridLayout->setContentsMargins(6, 6, 6, 6);
    topGridLayout->addWidget(setupWindowSizeLabel, 0, 0);
    topGridLayout->addWidget(m_windowSize, 0, 1);
    topGridLayout->addWidget(setupBiasWindowSizeLabel, 1, 0);
    topGridLayout->addWidget(m_biasWindow, 1, 1);
    topGridLayout->addWidget(setupDescriptionBlockLabel, 2, 0);
    topGridLayout->addWidget(m_descriptionBlock, 2, 1);
    topGridLayout->addWidget(setupExportFloderName, 3, 0);
    topGridLayout->addWidget(m_exportFloderName, 3, 1);
    topGridLayout->addWidget(setupExportPathFloder, 4, 0);
    topGridLayout->addWidget(m_exportPath, 4, 1);
    topGridLayout->addWidget(m_btnExportPath, 4, 2);
    topGridLayout->addWidget(m_btnRefreshInfo, 5, 1);
    topGridLayout->addWidget(btnExport, 5, 2);

    m_downInfo = new QTextEdit();

    downWidget = new QGroupBox("Export info");
    QVBoxLayout *downLayout = new QVBoxLayout(downWidget);
    downLayout->setContentsMargins(6, 6, 6, 6);
    downLayout->addWidget(m_downInfo);
    setupInfo();

    rightSplitter->addWidget(topWidget);
    rightSplitter->addWidget(downWidget);

    rightLayout->addWidget(rightSplitter);
}

void ExportCSV::createLayouts() {

    QHBoxLayout *selectButtons = new QHBoxLayout();
    selectButtons->addWidget(m_btnSelectAllFromParent);
    selectButtons->addWidget(m_btnUnselectAllFromParent);

    QHBoxLayout *statLayout = new QHBoxLayout();
    statLayout->addWidget(statLabel);
    statLayout->addStretch();

    leftLayout->addWidget(setCustomDB);
    leftLayout->addWidget(updateDB);
    leftLayout->addWidget(treeView, 1);
    leftLayout->addLayout(selectButtons);
    leftLayout->addLayout(statLayout);

// =================== SETUP PAGE ++++++++++++++++++

    leftWidget->setLayout(leftLayout);
    rightWidget->setLayout(rightLayout);

    mainSplitter->addWidget(leftWidget);
    mainSplitter->addWidget(rightWidget);

    mainLayout->addWidget(mainSplitter);
    setLayout(mainLayout);
}

void ExportCSV::connectSignals() {

    connect(treeView, &QTreeView::doubleClicked,
            this, &ExportCSV::onTreeViewDoubleClicked);

    connect(m_btnSelectAllFromParent, &QPushButton::clicked,
            this, &ExportCSV::selectAllInTable);

    connect(updateDB, &QPushButton::clicked,
            this, &ExportCSV::refreshDatabaseList);

    connect(setCustomDB, &QPushButton::clicked,
            this, &ExportCSV::openDBDialog);

    connect(m_btnUnselectAllFromParent, &QPushButton::clicked,
            this, &ExportCSV::unselectAllInTable);

    connect(m_btnExportPath, &QPushButton::clicked,
            this, &ExportCSV::onEditPathClicked);

    connect(m_btnRefreshInfo, &QPushButton::clicked,
            this, &ExportCSV::onRefreshInfoClicked);

    connect(treeModel, &QStandardItemModel::itemChanged,
            this, &ExportCSV::onItemCheckChanged);

}

void ExportCSV::refreshDatabaseList() {
    treeModel->blockSignals(true);
    treeModel->removeRows(0, treeModel->rowCount());
    m_selectedRecords.clear();

    QStringList databases = m_dbManager->getRegisteredDatabases();

    for (const QString &dbName : databases) {
        QStandardItem *dbNameItem = new QStandardItem(dbName);
        dbNameItem->setEditable(false);

        QStandardItem *dbCountItem = new QStandardItem();
        dbCountItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        treeModel->appendRow({dbNameItem, dbCountItem});

        QStringList tableNames = m_dbManager->getTableNames(dbName);
        tableNames = tableNames.filter("samples");

        int dbTotalCount = 0;

        for (const QString &table : tableNames) {
            QStandardItem *tableItem = new QStandardItem(table);
            tableItem->setEditable(false);

            QStandardItem *tableCountItem = new QStandardItem();
            tableCountItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

            int rowCount = m_dbManager->getTableRowCount(dbName, table);
            dbTotalCount += rowCount;

            tableCountItem->setText(QString::number(rowCount));
            dbNameItem->appendRow({tableItem, tableCountItem});

            QStringList idList = m_dbManager->getIdFromTable(dbName, table);

            for (const QString &id : idList) {
                QStringList rowData = m_dbManager->getTableRows(dbName, table, id.toInt());

                if (rowData.isEmpty()) continue;

                QString displayText = QString("ID:%1 | motion_type:%2 | sample_count:%3 | crc16:%4")
                    .arg(rowData.value(0, ""))
                    .arg(rowData.value(1, ""))
                    .arg(rowData.value(2, ""))
                    .arg(rowData.value(3, ""));

                QStandardItem *rowItem = new QStandardItem(displayText);
                rowItem->setEditable(false);
                rowItem->setCheckable(true);
                rowItem->setCheckState(Qt::Unchecked);
                rowItem->setData(id.toInt(), Qt::UserRole);
                rowItem->setData(dbName, Qt::UserRole + 1);
                rowItem->setData(table, Qt::UserRole + 2);

                tableItem->appendRow(rowItem);
            }
        }

        dbCountItem->setText(QString::number(dbTotalCount));
    }
    treeModel->blockSignals(false);
    updateStatLabel();
}

void ExportCSV::onTreeViewDoubleClicked(const QModelIndex &index) {
    if (!index.isValid()) return;

    QStandardItem *item = treeModel->itemFromIndex(index);
    if (!item) return;

    if (!item->isCheckable()) return;

    Qt::CheckState newState = (item->checkState() == Qt::Checked) ? Qt::Unchecked : Qt::Checked;
    item->setCheckState(newState);
}

void ExportCSV::onItemCheckChanged(QStandardItem *item) {
    if (!item || !item->isCheckable()) return;

    int id = item->data(Qt::UserRole).toInt();
    QString dbName = item->data(Qt::UserRole + 1).toString();
    QString tableName = item->data(Qt::UserRole + 2).toString();

    if (item->checkState() == Qt::Checked) {
        bool found = false;
        for (const auto &rec : m_selectedRecords) {
            if (rec.dbName == dbName && rec.tableName == tableName && rec.id == id) {
                found = true;
                break;
            }
        }
        if (!found) {
            m_selectedRecords.append({dbName, tableName, id});
        }
    } else {
        for (int i = m_selectedRecords.size() - 1; i >= 0; i--) {
            const auto &rec = m_selectedRecords[i];
            if (rec.dbName == dbName && rec.tableName == tableName && rec.id == id) {
                m_selectedRecords.removeAt(i);
                break;
            }
        }
    }

    updateStatLabel();
}

void ExportCSV::selectAllInTable() {
    QStandardItem *tableItem = findSelectedTableItem();
    if (!tableItem) return;
    treeModel->blockSignals(true);
    for (int i = 0; i < tableItem->rowCount(); i++) {
        QStandardItem *child = tableItem->child(i, 0);
        if (child && child->isCheckable()) {
            child->setCheckState(Qt::Checked);
        }
    }
    treeModel->blockSignals(false);

    rebuildSelectedRecords();
    updateStatLabel();
}

void ExportCSV::unselectAllInTable() {
    QStandardItem *tableItem = findSelectedTableItem();
    if (!tableItem) return;

    treeModel->blockSignals(true);
    for (int i = 0; i < tableItem->rowCount(); i++) {
        QStandardItem *child = tableItem->child(i, 0);
        if (child && child->isCheckable()) {
            child->setCheckState(Qt::Unchecked);
        }
    }
    treeModel->blockSignals(false);

    rebuildSelectedRecords();
    updateStatLabel();
}

void ExportCSV::updateStatLabel() {
    statLabel->setText(QString("Selected: %1").arg(m_selectedRecords.size()));
}

void ExportCSV::rebuildSelectedRecords() {
    m_selectedRecords.clear();

    for (int dbIdx = 0; dbIdx < treeModel->rowCount(); dbIdx++) {
        QStandardItem *dbItem = treeModel->item(dbIdx, 0);
        if (!dbItem) continue;

        QString dbName = dbItem->text();

        for (int tblIdx = 0; tblIdx < dbItem->rowCount(); tblIdx++) {
            QStandardItem *tableItem = dbItem->child(tblIdx, 0);
            if (!tableItem) continue;

            QString tableName = tableItem->text();

            for (int rowIdx = 0; rowIdx < tableItem->rowCount(); rowIdx++) {
                QStandardItem *child = tableItem->child(rowIdx, 0);
                if (!child || !child->isCheckable()) continue;
                if (child->checkState() != Qt::Checked) continue;

                int id = child->data(Qt::UserRole).toInt();
                m_selectedRecords.append({dbName, tableName, id});
            }
        }
    }
}
QStandardItem* ExportCSV::findSelectedTableItem() const {
    QModelIndexList selected = treeView->selectionModel()->selectedIndexes();
    if (selected.isEmpty()) return nullptr;

    QModelIndex idx = selected.first();
    if (!idx.isValid()) return nullptr;

    QStandardItem *item = treeModel->itemFromIndex(idx);
    if (!item) return nullptr;

    while (item) {
        QStandardItem *par = item->parent();
        if (!par) return nullptr;
        if (!par->parent()) {
            return item;
        }
        item = par;
    }

    return nullptr;
}

void ExportCSV::setupInfo() {
}

void ExportCSV::openDBDialog() {

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

void ExportCSV::onEditPathClicked() {
    QString dirPath = QFileDialog::getExistingDirectory(
            this,
            tr("Select folder for database"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
        );

    if (dirPath.isEmpty()) {
        return;
    }

    m_exportPath->setText(dirPath);

}

void ExportCSV::onRefreshInfoClicked() {

    int windowSize = m_windowSize->text().toInt();
    int biasSize = m_biasWindow->text().toInt();
    QString descriptionUser = m_descriptionBlock->toHtml();
    QString floderName = m_exportFloderName->text();
    QString pathExport = m_exportPath->text();

    if ( windowSize == 0 && biasSize == 0 && descriptionUser.isEmpty() && floderName.isEmpty() && pathExport.isEmpty()) {
        return;
    }

    m_downInfo->setHtml(descriptionUser);
    m_downInfo->append(QString(""));
    m_downInfo->append(QString("Window size: %1").arg(windowSize));
    m_downInfo->append(QString("Bias window size: %1").arg(biasSize));
    m_downInfo->append(QString("Export path: %1/%2/").arg(pathExport).arg(floderName));

    // leter auto genered info by groups motions
}
