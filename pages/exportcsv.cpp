#include "exportcsv.h"

#include "core/databasemanager.h"
#include "models/MotionType.h"
#include "modules/slicer/windowslicer.h"

#include <QDir>
#include <QDebug>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QGroupBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QTextStream>
#include <QSet>
#include <QList>

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
#include <QCheckBox>


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

    m_updateDB = new QPushButton("Refresh");
    setCustomDB = new QPushButton("Open db");

    m_btnSelectAllFromParent = new QPushButton("Select all in table");
    m_btnUnselectAllFromParent = new QPushButton("Unselect all in select table");
    m_btnChooseAllSelected = new QPushButton("Choose selected");

    statLabel = new QLabel("Selected: 0");

    treeModel->setHorizontalHeaderLabels({"Name", "Count"});
    treeView->setModel(treeModel);
    treeView->setHeaderHidden(false);
    treeView->setRootIsDecorated(true);
    treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    treeView->setSelectionBehavior(QAbstractItemView::SelectRows);

    treeView->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    treeView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    treeView->header()->resizeSection(0, 350);

    parentItem = treeModel->invisibleRootItem();

// ====================== EXPORT SETTINGS +++++++++++++++++++++

    rightSplitter = new QSplitter(Qt::Vertical, this);

    QLabel *setupWindowSizeLabel = new QLabel("Window size: ");
    QLabel *setupBiasWindowSizeLabel = new QLabel("Bias window size: ");
    QLabel *setupMaxStartOffsetLabel = new QLabel("Max offset forward (+): ");
    QLabel *setupMinOffsetLabel = new QLabel("Max offset backward (-): ");
    QLabel *setupDescriptionBlockLabel = new QLabel("Write description: ");
    QLabel *setupExportFloderName = new QLabel("Export floder name: ");
    QLabel *setupExportPathFloder = new QLabel("Export path: ");

    m_windowSize = new QLineEdit();
    m_biasWindow = new QLineEdit();
    m_maxStartOffset = new QLineEdit("0");
    m_maxStartOffset->setToolTip("Max random offset forward from startFlag (0 = no offset)");
    m_minOffset = new QLineEdit("0");
    m_minOffset->setToolTip("Max random offset backward from startFlag (0 = no offset)");
    QRegularExpression numberRx("[0-9]+");
    QValidator *numberValidator = new QRegularExpressionValidator(numberRx, this);
    m_windowSize->setValidator(numberValidator);
    m_biasWindow->setValidator(numberValidator);
    m_maxStartOffset->setValidator(numberValidator);
    m_minOffset->setValidator(numberValidator);

    m_exportPath = new QLineEdit();
    m_exportPath->setPlaceholderText("Enter directory for export dataset");
    QRegularExpression pathRx("[a-zA-Z0-9а-яА-Я.:_ /\\\\-]+");
    QValidator *pathValidator = new QRegularExpressionValidator(pathRx, this);
    m_exportPath->setValidator(pathValidator);

    m_exportFloderName = new QLineEdit();
    m_exportFloderName->setPlaceholderText("Enter name for root floder");
    QRegularExpression fileFloderRx("[a-zA-Z0-9а-яА-Я._ -]+");
    QValidator *fileFloderValidator = new QRegularExpressionValidator(fileFloderRx, this);
    m_exportFloderName->setValidator(fileFloderValidator);


    m_descriptionBlock = new QTextEdit();
    m_descriptionBlock->setPlaceholderText("Write info about export dataset. \n This information will be placed in the root section under the name info.txt.");

    m_descriptionBlock->setMaximumHeight(250);

    m_btnExportPath = new QPushButton("Path");
    m_btnExport = new QPushButton("Export");
    m_btnRefreshInfo = new QPushButton("Refresh export info");

    m_normalize = new QCheckBox("Normalize to [-1, 1]");
    m_maxAccel = new QLineEdit("4.0");
    m_maxAccel->setToolTip("Max accel range (g): ±4g → 4.0");
    m_maxAccel->setMaximumWidth(80);
    m_maxGyro = new QLineEdit("1000.0");
    m_maxGyro->setToolTip("Max gyro range (deg/s): ±1000 → 1000.0");
    m_maxGyro->setMaximumWidth(80);

    topWidget = new QGroupBox("Export settings");
    QGridLayout *topGridLayout = new QGridLayout(topWidget);
    topGridLayout->setContentsMargins(6, 6, 6, 6);
    topGridLayout->addWidget(setupWindowSizeLabel, 0, 0);
    topGridLayout->addWidget(m_windowSize, 0, 1);
    topGridLayout->addWidget(setupBiasWindowSizeLabel, 1, 0);
    topGridLayout->addWidget(m_biasWindow, 1, 1);
    topGridLayout->addWidget(setupMaxStartOffsetLabel, 2, 0);
    topGridLayout->addWidget(m_maxStartOffset, 2, 1);
    topGridLayout->addWidget(setupMinOffsetLabel, 3, 0);
    topGridLayout->addWidget(m_minOffset, 3, 1);
    topGridLayout->addWidget(setupDescriptionBlockLabel, 4, 0);
    topGridLayout->addWidget(m_descriptionBlock, 4, 1);
    topGridLayout->addWidget(setupExportFloderName, 5, 0);
    topGridLayout->addWidget(m_exportFloderName, 5, 1);
    topGridLayout->addWidget(setupExportPathFloder, 6, 0);
    topGridLayout->addWidget(m_exportPath, 6, 1);
    topGridLayout->addWidget(m_btnExportPath, 6, 2);
    topGridLayout->addWidget(m_btnRefreshInfo, 7, 1);
    topGridLayout->addWidget(m_btnExport, 7, 2);
    topGridLayout->addWidget(m_normalize, 8, 0);
    topGridLayout->addWidget(new QLabel("Max accel (g):"), 8, 1);
    topGridLayout->addWidget(m_maxAccel, 8, 2);
    topGridLayout->addWidget(new QLabel("Max gyro (°/s):"), 9, 1);
    topGridLayout->addWidget(m_maxGyro, 9, 2);

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
    selectButtons->addWidget(m_btnChooseAllSelected);
    selectButtons->addWidget(m_btnSelectAllFromParent);
    selectButtons->addWidget(m_btnUnselectAllFromParent);

    QHBoxLayout *statLayout = new QHBoxLayout();
    statLayout->addWidget(statLabel);
    statLayout->addStretch();

    leftLayout->addWidget(setCustomDB);
    leftLayout->addWidget(m_updateDB);
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

    connect(m_updateDB, &QPushButton::clicked,
            this, &ExportCSV::refreshDatabaseList);

    connect(setCustomDB, &QPushButton::clicked,
            this, &ExportCSV::openDBDialog);

    connect(m_btnUnselectAllFromParent, &QPushButton::clicked,
            this, &ExportCSV::unselectAllInTable);

    connect(m_btnExportPath, &QPushButton::clicked,
            this, &ExportCSV::onEditPathClicked);

    connect(m_btnRefreshInfo, &QPushButton::clicked,
            this, &ExportCSV::onRefreshInfoClicked);

    connect(m_btnExport, &QPushButton::clicked,
            this, &ExportCSV::onExportClicked);

    connect(m_btnChooseAllSelected, &QPushButton::clicked,
            this, &ExportCSV::onChooseSelectedClicked);

    connect(treeModel, &QStandardItemModel::itemChanged,
            this, &ExportCSV::onItemCheckChanged);


}

void ExportCSV::refreshDatabaseList() {
    treeModel->blockSignals(true);

    treeModel->clear();
    treeModel->setHorizontalHeaderLabels({"Name", "Count writes"});
    treeView->reset();

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

                QString displayText = QString("ID:%1 | %2 | sample_count:%3 | %4")
                    .arg(rowData.value(0, ""))
                    .arg(motionTypeToString(rowData.value(1).toInt()))
                    .arg(rowData.value(2, ""))
                    .arg((rowData.value(4).toInt() != -1 || rowData.value(5).toInt() != -1) ? "MARKERED" : "NOT MARKERED");

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

    treeView->update();
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
    if (!tableItem) {
        qDebug() << "No table selected";
        return;
    }

    // Проверяем, что это действительно таблица
    if (!tableItem->hasChildren()) {
        qDebug() << "Selected item is not a table";
        return;
    }

    treeModel->blockSignals(true);

    int checkedCount = 0;
    for (int i = 0; i < tableItem->rowCount(); i++) {
        QStandardItem *child = tableItem->child(i, 0);
        if (child && child->isCheckable()) {
            child->setCheckState(Qt::Checked);
            checkedCount++;
        }
    }

    treeModel->blockSignals(false);

    qDebug() << "Selected" << checkedCount << "records in table" << tableItem->text();

    rebuildSelectedRecords();
    updateStatLabel();
}

void ExportCSV::unselectAllInTable() {
    QStandardItem *tableItem = findSelectedTableItem();
    if (!tableItem || !tableItem->hasChildren()) return;

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
    OpenDBDialog dialog(registeredDbs);

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
            tr("Select root for database"),
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

    if ( windowSize == 0 || biasSize == 0 || descriptionUser.isEmpty() || floderName.isEmpty() || pathExport.isEmpty()) {
        logMessage(LogLevel::Warning, "Fill all data fields.");
    }
    /*if (typeCounters.isEmpty()){
        logMessage(LogLevel::Warning, "Select files for generate dataset.");
    }*/

    m_downInfo->setHtml(descriptionUser);
    m_downInfo->append(QString(""));
    m_downInfo->append(QString("Window size: %1").arg(windowSize));
    m_downInfo->append(QString("Bias window size: %1").arg(biasSize));
    m_downInfo->append(QString("Max offset forward: %1").arg(m_maxStartOffset->text().toInt()));
    m_downInfo->append(QString("Max offset backward: %1").arg(m_minOffset->text().toInt()));
    m_downInfo->append(QString("Export path: %1/%2/").arg(pathExport).arg(floderName));

    if (m_normalize->isChecked()) {
        m_downInfo->append(QString("Normalize: ON (±%1g, ±%2°/s → [-1, 1])")
            .arg(m_maxAccel->text()).arg(m_maxGyro->text()));
    } else {
        m_downInfo->append(QString("Normalize: OFF"));
    }

    QMap<int, int> gestureCount;
    QMap<int, int> totalSamplesPerType;

    for (const SelectedRecord &record : m_selectedRecords) {
        QStringList list = m_dbManager->getTableRows(record.dbName, record.tableName, record.id);
        if (list.isEmpty()) continue;

        int typeMotion = list.at(1).toInt();
        gestureCount[typeMotion]++;

        QVector<QVector<float>> samples = m_dbManager->getGestureSamples(record.dbName, record.id);
        totalSamplesPerType[typeMotion] += samples.size();
    }

    m_downInfo->append(QString(""));
    m_downInfo->append(QString("--- Gesture breakdown ---"));

    for (auto it = gestureCount.constBegin(); it != gestureCount.constEnd(); ++it) {
        int type = it.key();
        int count = it.value();
        int totalSamples = totalSamplesPerType[type];

        int windowsPerGesture = 0;
        if (windowSize > 0 && totalSamples / count >= windowSize) {
            int samplesPerGesture = totalSamples / count;
            windowsPerGesture = (samplesPerGesture - windowSize) / biasSize + 1;
        }

        m_downInfo->append(QString("%1: %2 gestures, %3 windows each. Total samples: %4")
            .arg(motionTypeToString(type))
            .arg(count)
            .arg(windowsPerGesture).arg(totalSamples));
    }
}

void ExportCSV::onExportClicked() {

    const int windowSize = m_windowSize->text().toInt();
    const int windowBias = m_biasWindow->text().toInt();
    const int maxOffset = m_maxStartOffset->text().toInt();
    const int minOffset = m_minOffset->text().toInt();
    const QString path = m_exportPath->text();
    const QString name = m_exportFloderName->text();

    if (path.isEmpty() || name.isEmpty()){
        logMessage(LogLevel::Error, QString("Parameters not specified. \n "
                                            "Export path: %1 \n"
                                            "Floder name: %2").arg(path).arg(name));
        return;
    }


    if (windowSize <= 0 || windowBias <= 0 || m_windowSize->text().isEmpty() || m_biasWindow->text().isEmpty()) {
        logMessage(LogLevel::Error, QString("Parameters not specified. \n "
                                            "Window size: %1 \n"
                                            "Bias window: %2").arg(windowSize).arg(windowBias));
        return;
    }

    /*if (typeCounters.isEmpty()) {
        logMessage(LogLevel::Error, "Select elements to generate the dataset.");
        return;
    }*/

    onRefreshInfoClicked();
    resetTypeCounter();

    QString msg = QString("Export dataset + i more info "
                          "\n Windows size: %1\n "
                          "Window bias: %2\n"
                          "Max offset: %3\n"
                          "Min offset: %4\n").arg(windowSize).arg(windowBias).arg(maxOffset).arg(maxOffset);
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Export dataset",
        msg,
        QMessageBox::Save | QMessageBox::Cancel);

    if (reply == QMessageBox::Save) {

        createFloders(path, m_exportFloderName->text());
        QString editPath = m_exportPath->text();
        editPath.append("/");
        editPath.append(m_exportFloderName->text());

        QSet<int> uniqueMotionTypes;

        for (const SelectedRecord &record : m_selectedRecords) {

            QStringList list = m_dbManager->getTableRows(record.dbName, record.tableName, record.id);



            if (!list.isEmpty()) {
                uniqueMotionTypes.insert(list.at(1).toInt());
            }
        }

        for (int motionType : uniqueMotionTypes) {
            createFloders(editPath, motionTypeToString(motionType));
        }
        createReport(editPath, windowSize, windowBias, maxOffset, minOffset);
        createReportMassage(editPath, m_downInfo->toPlainText());
    }
}

void ExportCSV::onChooseSelectedClicked() {
    QModelIndexList selectedIndexes = treeView->selectionModel()->selectedRows(0);

    treeModel->blockSignals(true);

    for (const QModelIndex &index : selectedIndexes) {
        QStandardItem *child = treeModel->itemFromIndex(index);
        if (child && child->isCheckable()) {
            child->setCheckState(Qt::Checked);
        }
    }

    treeModel->blockSignals(false);

    rebuildSelectedRecords();
    updateStatLabel();
}

void ExportCSV::createFloders(const QString path, const QString nameFolder) {

    if (path.length() + nameFolder.length() + 1 > 4096) {
        logMessage(LogLevel::Error, "Path is too long");
        return;
    }

    QRegularExpression invalidChars("[.?<>:\"/\\|?*]");

    if (nameFolder.contains(invalidChars)) {
        logMessage(LogLevel::Error, "Folder name contains invalid characters");
        return;
    }

    QString workPath = QDir::cleanPath(path);

    QDir dir(workPath);

    if (!dir.exists()) {
        logMessage(LogLevel::Error, QString("Parent directory does not exist: %1").arg(workPath));
        return;
    }

    if (!dir.isReadable()) {
        logMessage(LogLevel::Error, QString("Directory is not readable: %1").arg(workPath));
        return;
    }

    QString fullPath = QDir(workPath).filePath(nameFolder);

    QFileInfo fileInfo(fullPath);
    if (fileInfo.exists() && fileInfo.isFile()) {
        logMessage(LogLevel::Error, QString("Path points to a file, not a directory:: %1").arg(fullPath));
        return;
    }

    QDir workDir(workPath);
    if (workDir.mkpath(nameFolder)){

    }

}

void ExportCSV::createReport(const QString path, int windowSize, int windowBias, int maxOffset, int minOffset) {

    for (const SelectedRecord &record : m_selectedRecords) {

        QStringList list = m_dbManager->getTableRows(record.dbName, record.tableName, record.id);
        int typeMotion = list.at(1).toInt();
        int startFlag = list.at(4).toInt();
        int endFlag = list.at(5).toInt();
        QString typePath = path;
        typePath.append("/");
        typePath.append(motionTypeToString(typeMotion));

        createCVSFile(typePath, typeMotion, record, windowSize, windowBias, maxOffset, minOffset, startFlag, endFlag);

    }
}

void ExportCSV::createCVSFile(const QString path, int typeMotion, const SelectedRecord &record,
                              int windowSize, int windowBias, int maxOffset, int minOffset,
                              const int startFlag, const int endFlag) {

    QVector<QVector<float>> samples = m_dbManager->getGestureSamples(record.dbName, record.id);

    WindowSlicer slicer;
    slicer.setWindowSize(windowSize);
    slicer.setBias(windowBias);
    slicer.setMaxOffset(maxOffset);
    slicer.setMinOffset(minOffset);
    slicer.setStartFlag(startFlag);
    slicer.setEndFlag(endFlag);

    QVector<QVector<QVector<float>>> windows = slicer.slice(samples);

    bool doNormalize = m_normalize->isChecked();
    float maxAccel = m_maxAccel->text().toFloat();
    float maxGyro = m_maxGyro->text().toFloat();

    for (int i = 0; i < windows.size(); i++) {

        int fileIndex = typeCounters[typeMotion]++;

        QString filePath = path;
        filePath.append("/");
        filePath.append(motionTypeToString(typeMotion));
        filePath.append(QString("_%1").arg(fileIndex));
        filePath.append(QString(".csv"));

        QFile file(filePath);

        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out.setCodec("UTF-8");

            const QVector<QVector<float>> &win = windows[i];
            for (const QVector<float> &sample : win) {
                if (doNormalize) {
                    out << sample[0] / maxAccel << "," << sample[1] / maxAccel << "," << sample[2] / maxAccel << ","
                        << sample[3] / maxGyro << "," << sample[4] / maxGyro << "," << sample[5] / maxGyro << "\n";
                } else {
                    out << sample[0] << "," << sample[1] << "," << sample[2] << ","
                        << sample[3] << "," << sample[4] << "," << sample[5] << "\n";
                }
            }

            file.close();
        }
    }
}

void ExportCSV::createReportMassage(QString filePath, const QString reportText) {
    QString textToWrite = reportText;
    QFile file(filePath.append("/report.txt"));

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out.setCodec("UTF-8");

        out << reportText;

        file.close();
    }
    else {
        qDebug() << file.errorString();
    }

}

void ExportCSV::resetTypeCounter() { typeCounters.clear(); }
