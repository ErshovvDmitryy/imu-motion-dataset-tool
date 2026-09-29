#include "pages/databasepage.h"
#include "core/databasemanager.h"

#include "qcustomplot/qcustomplot.h"
#include "modules/widgets/plotmanager.h"
#include "modules/plot/plotconfig.h"
#include "models/builtinschemas.h"
#include "models/imuadapter.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStringList>
#include <QMessageBox>
#include <QFileInfo>
#include <QSplitter>
#include <QLineEdit>
#include <QDebug>
#include <QMouseEvent>
#include <QPen>
#include <QColor>
#include <QItemSelectionModel>
#include <QSqlRecord>
#include <QSqlError>
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QTableView>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QTreeView>

namespace {

const char *kSlotDbAccel = "database/accel";
const char *kSlotDbGyro = "database/gyro";

PlotConfig defaultDbConfig(const char *title, const char *yLabel, const char *const *fields, int fieldCount) {
    PlotConfig config;
    config.schemaName = QString::fromLatin1(BuiltinSchemas::imuName);
    config.title = QString::fromLatin1(title);
    config.xMode = XAxisMode::Timestamp;
    config.relativeTime = true;
    config.liveWindowSec = 0.0;
    config.xLabel = QStringLiteral("Time (s)");
    config.yLabel = QString::fromLatin1(yLabel);
    for (int i = 0; i < fieldCount; ++i) {
        config.traces.append(PlotTrace{ QString::fromLatin1(fields[i]), QColor() });
    }
    config.applyDefaultColors();
    return config;
}

} // namespace

DataBasePage::DataBasePage(DatabaseManager *dbManager, PlotManager *plotManager,
                           PlotConfigStore *plotConfigStore, QWidget *parent)
    : QWidget(parent)
    , m_dbManager(dbManager)
    , m_plotManager(plotManager)
    , m_plotConfigStore(plotConfigStore)
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

// =================== GLOBAL PAGE SETTINGS ===================

    // The widgets that organaize the main layout page declared here

    mainLayout = new QHBoxLayout(this);

    leftLayout = new QVBoxLayout();
    rightLayout = new QVBoxLayout();

    leftWidget = new QWidget();
    rightWidget = new QWidget();
    leftWidget->setMinimumWidth(250);
    leftWidget->setMaximumWidth(500);

    mainSplitter = new QSplitter(Qt::Horizontal, this);

// =================== LEFT SIDE ===================

    // All widgets stacked in vertial layout
    // All buttuns has been declarede in func: setupButtons()

// =================== RIGHT SIDE ===================

    pathLayout = new QHBoxLayout();     // Include QLineEdit and "set path" button
    exportLayout = new QHBoxLayout();   // Include: Export,
                                        //export all and remove from db buttons
    graphLayout = new QHBoxLayout();    // Include graph

    pathArea = new QHBoxLayout();       //
    pathAreaRight = new QVBoxLayout();  // Create
    pathAreaLeft = new QVBoxLayout();   //

    areaBtnGraph = new QHBoxLayout();   // Horizontal layout include buttons func:
                                        // refresh, cut, motion type, accept/deny cut,
                                        // rndo/next motion, add/extrect selected

    labelStatsChanged = new QLabel("Total added: --");

    m_pathEdit = new QLineEdit;
    m_pathEdit->setPlaceholderText("Selected file path will appear here...");
    m_pathEdit->setReadOnly(true);

    motionTypeLegend = new QLabel();
    motionTypeLegend->setStyleSheet("font-size: 10px;");
    motionTypeLegend->setWordWrap(true);
    updateMotionTypeLegend();

    setupsTableView();
    setupButtonsOnPage();
    setupGraph();

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

    exportLayout->addWidget(btnExportToDataSet);
    exportLayout->addWidget(btnExportAllToDataSet);
    exportLayout->addWidget(btnDeleteFromDataSet);

    pathAreaLeft->addLayout(pathLayout);
    pathAreaLeft->addLayout(exportLayout);
    pathAreaRight->addWidget(labelStatsChanged);

    pathArea->addLayout(pathAreaLeft);
    pathArea->addLayout(pathAreaRight);
    pathArea->addStretch();

    graphLayout->addWidget(accelGraph);
    graphLayout->addWidget(gyroGraph);

    areaBtnGraph->addWidget(btnRefreshGraph);
    areaBtnGraph->addWidget(btnCutGraph);
    areaBtnGraph->addWidget(motionTypeLegend);
    areaBtnGraph->addWidget(btnTrimAccept);
    areaBtnGraph->addWidget(btnTrimDeny);
    areaBtnGraph->addWidget(btnFlagAccept);
    areaBtnGraph->addWidget(btnFlagDeny);
    areaBtnGraph->addStretch();

    areaBtnGraph->addWidget(btnUndoSample);
    areaBtnGraph->addWidget(btnNextSample);
    areaBtnGraph->addStretch();
    areaBtnGraph->addWidget(btnAddSelected);
    areaBtnGraph->addWidget(btnExtractSelected);

    rightLayout->addLayout(pathArea);
    rightLayout->addWidget(samplesView);
    rightLayout->addLayout(graphLayout);
    rightLayout->addLayout(areaBtnGraph);

    rightLayout->addWidget(motionDataView);

    rightLayout->addStretch();

// =================== SETUP PAGE

    leftWidget->setLayout(leftLayout);
    rightWidget->setLayout(rightLayout);

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

    connect(btnNextSample, &QPushButton::clicked, this, [this]() { navigateSample(1); });
    connect(btnUndoSample, &QPushButton::clicked, this, [this]() { navigateSample(-1); });

    connect(btnAddSelected, &QPushButton::clicked, this, &DataBasePage::onAddSelectedClicked);
    connect(btnExtractSelected, &QPushButton::clicked, this, &DataBasePage::onExtractSelectedClicked);
    connect(btnDeleteFromDataSet, &QPushButton::clicked, this, &DataBasePage::onDeleteFromDataSetClicked);

    connect(btnCutGraph, &QPushButton::clicked, this, &DataBasePage::onTrimButtonClicked);
    connect(btnTrimAccept, &QPushButton::clicked, this, &DataBasePage::onTrimAccept);
    connect(btnTrimDeny, &QPushButton::clicked, this, &DataBasePage::onTrimDeny);
    connect(btnFlagAccept, &QPushButton::clicked, this, &DataBasePage::onFlagAccept);
    connect(btnFlagDeny, &QPushButton::clicked, this, &DataBasePage::onFlagDeny);

    connect(accelGraph, &QCustomPlot::mousePress,
            this, [this](QMouseEvent *e) { onGraphMousePress(e, accelGraph); });
    connect(accelGraph, &QCustomPlot::mouseMove,
            this, [this](QMouseEvent *e) { onGraphMouseMove(e, accelGraph); });
    connect(accelGraph, &QCustomPlot::mouseRelease,
            this, [this](QMouseEvent *e) { onGraphMouseRelease(e, accelGraph); });

    connect(gyroGraph, &QCustomPlot::mousePress,
            this, [this](QMouseEvent *e) { onGraphMousePress(e, gyroGraph); });
    connect(gyroGraph, &QCustomPlot::mouseMove,
            this, [this](QMouseEvent *e) { onGraphMouseMove(e, gyroGraph); });
    connect(gyroGraph, &QCustomPlot::mouseRelease,
            this, [this](QMouseEvent *e) { onGraphMouseRelease(e, gyroGraph); });


    connectSampleSelectionHandler();
}

void DataBasePage::connectSampleSelectionHandler() {
    QItemSelectionModel *selection = samplesView->selectionModel();
    if (!selection) return;
    connect(selection, &QItemSelectionModel::selectionChanged,
            this, &DataBasePage::onSamplesSelectionChanged);
}

void DataBasePage::onSamplesSelectionChanged(const QItemSelection &selected) {
    updateSelectedHighlight();
    updateNavigationButtons();

    if (selected.indexes().isEmpty()) return;

    QModelIndex index = selected.indexes().first();
    if (!index.isValid()) return;

    int row = index.row();
    m_currentSampleRow = row;
    int sampleId = samplesModel->data(samplesModel->index(row, 0)).toInt();
    loadSampleToGraphs(sampleId);
}

void DataBasePage::openDBDialog()
{
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

    treeModel->blockSignals(true);

    treeModel->clear();
    treeModel->setHorizontalHeaderLabels({"Name", "Count writes"});
    treeView->reset();

    QStringList databases = m_dbManager->getRegisteredDatabases();

    for (const QString &dbName : databases) {
        QStandardItem *dbNameItem = new QStandardItem(dbName);

        QStandardItem *countItem = new QStandardItem();
        countItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        treeModel->appendRow({dbNameItem, countItem});

        QStringList tableNames = m_dbManager->getTableNames(dbName);

        int totalCount = 0;
        for (const QString &table : tableNames) {
            QStandardItem *tableItem = new QStandardItem(table);

            QStandardItem *tableCountItem = new QStandardItem();
            tableCountItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

            int rowCount = m_dbManager->getTableRowCount(dbName, table);
            totalCount += rowCount;

            tableCountItem->setText(QString::number(rowCount));
            dbNameItem->appendRow({tableItem, tableCountItem});

            QStringList columnsInTable = m_dbManager->getTableRowsNames(dbName, table);

            for (const QString &column : columnsInTable) {
                QStandardItem *columnItem = new QStandardItem(column);
                QStandardItem *blankItem = new QStandardItem("");
                tableItem->appendRow({columnItem, blankItem});
            }
        }

        countItem->setText(QString::number(totalCount));
    }

    treeView->update();
    treeModel->blockSignals(false);
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

void DataBasePage::refreshSamplesModel(const QString &dbName, const QString &) {

    if (dbName.isEmpty()) return;

    samplesModel->clear();

    m_currentDatabase = dbName;

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
    connectSampleSelectionHandler();

    m_currentSampleRow = -1;
    m_selectedList.clear();
    updateStatsLabel();
    updateNavigationButtons();
    updateMotionTypeLegend();
}

void DataBasePage::loadSampleToGraphs(int sampleId) {
    QSqlDatabase db = m_dbManager->getDatabase(m_currentDatabase);
    if (!db.isOpen()) return;

    reloadFrames(sampleId);

    QSqlQuery typeQuery(db);
    typeQuery.prepare("SELECT motion_type, start_flag, end_flag FROM samples WHERE id = :id");
    typeQuery.bindValue(":id", sampleId);
    if (typeQuery.exec() && typeQuery.next()) {
        const int motionTypeId = typeQuery.value(0).toInt();
        updateMotionTypeLegend(motionTypeId);

        const int startFlag = typeQuery.value(1).toInt();
        const int endFlag = typeQuery.value(2).toInt();

        if ((startFlag != -1 || endFlag != -1) && m_frames.isEmpty() == false) {
            bool firstOk = false;
            const double firstT = m_frames.first().timeSeconds(&firstOk);
            if (firstOk) {
                if (startFlag >= 0 && startFlag < m_frames.size()) {
                    bool ok = false;
                    m_flagStartSec = m_frames.at(startFlag).timeSeconds(&ok) - firstT;
                    if (ok) {
                        addFlagLine(m_flagStartSec, true);
                    }
                }
                if (endFlag >= 0 && endFlag < m_frames.size()) {
                    bool ok = false;
                    m_flagEndSec = m_frames.at(endFlag).timeSeconds(&ok) - firstT;
                    if (ok) {
                        addFlagLine(m_flagEndSec, false);
                    }
                }
            }
        }
    }
}

void DataBasePage::reloadFrames(int sampleId) {
    QSqlDatabase db = m_dbManager->getDatabase(m_currentDatabase);
    if (db.isOpen() == false) {
        return;
    }

    clearFlagSeparators();
    m_flagStartSec = m_flagEndSec = -1;
    btnFlagAccept->setEnabled(false);
    btnFlagDeny->setEnabled(false);

    QSqlQuery query(db);
    query.prepare("SELECT ax, ay, az, gx, gy, gz, timestamp FROM motion_data "
                  "WHERE sample_id = :sample_id ORDER BY sample_index");
    query.bindValue(":sample_id", sampleId);

    if (query.exec() == false) {
        qDebug() << "Failed to load motion data:" << query.lastError().text();
        return;
    }

    m_frames.clear();
    while (query.next()) {
        m_frames.append(ImuAdapter::makeImuFrame(
            static_cast<float>(query.value("ax").toDouble()),
            static_cast<float>(query.value("ay").toDouble()),
            static_cast<float>(query.value("az").toDouble()),
            static_cast<float>(query.value("gx").toDouble()),
            static_cast<float>(query.value("gy").toDouble()),
            static_cast<float>(query.value("gz").toDouble()),
            static_cast<quint32>(query.value("timestamp").toLongLong()),
            true));
    }

    if (m_plotManager == nullptr) {
        return;
    }
    m_plotManager->paintFrames(*accelGraph, m_frames);
    m_plotManager->paintFrames(*gyroGraph, m_frames);
}

void DataBasePage::navigateSample(int delta) {
    if (!samplesModel || samplesModel->rowCount() == 0) return;

    int newRow = m_currentSampleRow + delta;
    if (newRow < 0) newRow = 0;
    if (newRow >= samplesModel->rowCount()) newRow = samplesModel->rowCount() - 1;

    if (newRow == m_currentSampleRow) return;

    m_currentSampleRow = newRow;

    QModelIndex index = samplesModel->index(m_currentSampleRow, 0);
    samplesView->selectRow(m_currentSampleRow);
    samplesView->setCurrentIndex(index);
    samplesView->scrollTo(index, QAbstractItemView::PositionAtCenter);

    int sampleId = samplesModel->data(samplesModel->index(m_currentSampleRow, 0)).toInt();
    loadSampleToGraphs(sampleId);

    updateNavigationButtons();
}

void DataBasePage::updateNavigationButtons() {
    int rowCount = samplesModel ? samplesModel->rowCount() : 0;
    bool hasSelection = samplesView->selectionModel() && samplesView->selectionModel()->hasSelection();
    bool hasCurrent = m_currentSampleRow >= 0 && m_currentSampleRow < rowCount;
    
    btnUndoSample->setEnabled(hasCurrent && m_currentSampleRow > 0);
    btnNextSample->setEnabled(hasCurrent && m_currentSampleRow < rowCount - 1);
    
    if (hasSelection && !hasCurrent) {
        QModelIndexList selected = samplesView->selectionModel()->selectedRows();
        if (!selected.isEmpty()) {
            int row = selected.first().row();
            btnUndoSample->setEnabled(row > 0);
            btnNextSample->setEnabled(row < rowCount - 1);
        }
    }
}

void DataBasePage::onTrimButtonClicked() {
    if (m_currentSampleRow < 0) {
        QMessageBox::warning(this, "No Sample", "Select a sample first.");
        return;
    }

    if (m_trimState == TrimState::Off) {
        m_rangeDragAccel = accelGraph->interactions().testFlag(QCP::iRangeDrag);
        m_rangeDragGyro = gyroGraph->interactions().testFlag(QCP::iRangeDrag);
        clearTrimSeparators();
        m_trimStartSec = m_trimEndSec = -1;
        m_trimState = TrimState::AwaitStart;
        btnTrimAccept->setEnabled(false);
        btnTrimDeny->setEnabled(false);
        btnCutGraph->setText("Cancel Trim");
        setTrimInteractionEnabled(true);
    } else {
        resetTrim();
        btnCutGraph->setText("Cut");
    }
}

void DataBasePage::onGraphMousePress(QMouseEvent *event, QCustomPlot *plot) {
    if (m_trimState == TrimState::Off && m_flagStartSec < 0) return;

    const int sampleId = samplesModel->data(samplesModel->index(m_currentSampleRow, 0)).toInt();
    Q_UNUSED(sampleId)
    double sec = clampToData(plot->xAxis->pixelToCoord(event->pos().x()));

    if (m_trimState == TrimState::AwaitStart) {
        m_trimStartSec = sec;
        addTrimLine(sec, true);
        m_trimState = TrimState::AwaitEnd;
    }
    else if (m_trimState == TrimState::AwaitEnd) {
        m_trimEndSec = sec;
        addTrimLine(sec, false);
        m_trimState = TrimState::Adjust;
        btnTrimAccept->setEnabled(true);
        btnTrimDeny->setEnabled(true);
    }
    else if (m_trimState == TrimState::Adjust) {
        const double px = event->pos().x();
        if (m_trimStartSec >= 0) {
            double linePx = plot->xAxis->coordToPixel(m_trimStartSec);
            if (qAbs(linePx - px) <= TrimDragThresholdPx) {
                m_trimDragging = true;
                m_trimDragIsStart = true;
                return;
            }
        }
        if (m_trimEndSec >= 0) {
            double linePx = plot->xAxis->coordToPixel(m_trimEndSec);
            if (qAbs(linePx - px) <= TrimDragThresholdPx) {
                m_trimDragging = true;
                m_trimDragIsStart = false;
                return;
            }
        }
    }

    if (m_flagStartSec >= 0 || m_flagEndSec >= 0) {
        const double px = event->pos().x();
        if (m_flagStartSec >= 0) {
            double linePx = plot->xAxis->coordToPixel(m_flagStartSec);
            if (qAbs(linePx - px) <= FlagDragThresholdPx) {
                m_flagDragging = true;
                m_flagDragIsStart = true;
                m_flagDragAccel = accelGraph->interactions().testFlag(QCP::iRangeDrag);
                m_flagDragGyro = gyroGraph->interactions().testFlag(QCP::iRangeDrag);
                setFlagInteractionEnabled(true);
                return;
            }
        }
        if (m_flagEndSec >= 0) {
            double linePx = plot->xAxis->coordToPixel(m_flagEndSec);
            if (qAbs(linePx - px) <= FlagDragThresholdPx) {
                m_flagDragging = true;
                m_flagDragIsStart = false;
                m_flagDragAccel = accelGraph->interactions().testFlag(QCP::iRangeDrag);
                m_flagDragGyro = gyroGraph->interactions().testFlag(QCP::iRangeDrag);
                setFlagInteractionEnabled(true);
                return;
            }
        }
    }
}

void DataBasePage::onGraphMouseMove(QMouseEvent *event, QCustomPlot *plot) {
    double sec = clampToData(plot->xAxis->pixelToCoord(event->pos().x()));

    if (m_trimDragging) {
        if (m_trimDragIsStart) {
            m_trimStartSec = sec;
        } else {
            m_trimEndSec = sec;
        }
        updateTrimLine(sec, m_trimDragIsStart);
    }

    if (m_flagDragging) {
        if (m_flagDragIsStart) {
            m_flagStartSec = sec;
        } else {
            m_flagEndSec = sec;
        }
        updateFlagLine(sec, m_flagDragIsStart);
        btnFlagAccept->setEnabled(true);
        btnFlagDeny->setEnabled(true);
    }
}

void DataBasePage::onGraphMouseRelease(QMouseEvent *event, QCustomPlot *plot) {
    Q_UNUSED(event);
    Q_UNUSED(plot);
    m_trimDragging = false;
    if (m_flagDragging) {
        m_flagDragging = false;
        setFlagInteractionEnabled(false);
    }
}

void DataBasePage::addTrimLine(double sec, bool isStart) {
    if (m_plotManager == nullptr) {
        return;
    }
    const MarkerType type = isStart ? MarkerType::TrimStart : MarkerType::TrimEnd;
    m_plotManager->clearMarkers(*accelGraph, type);
    m_plotManager->clearMarkers(*gyroGraph, type);
    m_plotManager->addMarker(*accelGraph, sec, type);
    m_plotManager->addMarker(*gyroGraph, sec, type);
}

void DataBasePage::updateTrimLine(double sec, bool isStart) {
    if (m_plotManager == nullptr) {
        return;
    }
    const MarkerType type = isStart ? MarkerType::TrimStart : MarkerType::TrimEnd;
    m_plotManager->setMarkerX(*accelGraph, type, sec);
    m_plotManager->setMarkerX(*gyroGraph, type, sec);
}

void DataBasePage::clearTrimSeparators() {
    if (m_plotManager == nullptr) {
        return;
    }
    for (QCustomPlot *plot : { accelGraph, gyroGraph }) {
        m_plotManager->clearMarkers(*plot, MarkerType::TrimStart);
        m_plotManager->clearMarkers(*plot, MarkerType::TrimEnd);
    }
}

void DataBasePage::setTrimInteractionEnabled(bool on) {
    if (m_plotManager == nullptr) {
        return;
    }
    if (on) {
        m_plotManager->setRangeDragEnabled(*accelGraph, false);
        m_plotManager->setRangeDragEnabled(*gyroGraph, false);
    } else {
        m_plotManager->setRangeDragEnabled(*accelGraph, m_rangeDragAccel);
        m_plotManager->setRangeDragEnabled(*gyroGraph, m_rangeDragGyro);
    }
}

void DataBasePage::resetTrim() {
    clearTrimSeparators();
    m_trimStartSec = m_trimEndSec = -1;
    m_trimState = TrimState::Off;
    m_trimDragging = false;
    btnTrimAccept->setEnabled(false);
    btnTrimDeny->setEnabled(false);
    btnCutGraph->setText("Cut");
    setTrimInteractionEnabled(false);
}

void DataBasePage::addFlagLine(double sec, bool isStart) {
    if (m_plotManager == nullptr) {
        return;
    }
    const MarkerType type = isStart ? MarkerType::FlagStart : MarkerType::FlagEnd;
    m_plotManager->clearMarkers(*accelGraph, type);
    m_plotManager->clearMarkers(*gyroGraph, type);
    m_plotManager->addMarker(*accelGraph, sec, type);
    m_plotManager->addMarker(*gyroGraph, sec, type);
}

void DataBasePage::updateFlagLine(double sec, bool isStart) {
    if (m_plotManager == nullptr) {
        return;
    }
    const MarkerType type = isStart ? MarkerType::FlagStart : MarkerType::FlagEnd;
    m_plotManager->setMarkerX(*accelGraph, type, sec);
    m_plotManager->setMarkerX(*gyroGraph, type, sec);
}

void DataBasePage::clearFlagSeparators() {
    if (m_plotManager == nullptr) {
        return;
    }
    for (QCustomPlot *plot : { accelGraph, gyroGraph }) {
        m_plotManager->clearMarkers(*plot, MarkerType::FlagStart);
        m_plotManager->clearMarkers(*plot, MarkerType::FlagEnd);
    }
}

void DataBasePage::setFlagInteractionEnabled(bool on) {
    if (m_plotManager == nullptr) {
        return;
    }
    if (on) {
        m_plotManager->setRangeDragEnabled(*accelGraph, false);
        m_plotManager->setRangeDragEnabled(*gyroGraph, false);
    } else {
        m_plotManager->setRangeDragEnabled(*accelGraph, m_flagDragAccel);
        m_plotManager->setRangeDragEnabled(*gyroGraph, m_flagDragGyro);
    }
}

void DataBasePage::applyFlagsToDatabase(int sampleId, int startFlag, int endFlag) {
    m_dbManager->updateFlags(m_currentDatabase, sampleId, startFlag, endFlag);
}

double DataBasePage::clampToData(double sec) const {
    if (m_currentSampleRow < 0 || m_dbManager == nullptr) return sec;

    QSqlDatabase db = m_dbManager->getDatabase(m_currentDatabase);
    if (!db.isOpen()) return sec;

    int sampleId = samplesModel->data(samplesModel->index(m_currentSampleRow, 0)).toInt();

    QSqlQuery query(db);
    query.prepare("SELECT MIN(timestamp), MAX(timestamp) FROM motion_data WHERE sample_id = :id");
    query.bindValue(":id", sampleId);
    if (!query.exec() || !query.next()) return sec;
    if (query.value(0).isNull() || query.value(1).isNull()) return sec;

    const double first = query.value(0).toLongLong() / 1000000.0;
    const double last  = query.value(1).toLongLong() / 1000000.0;
    return qBound(0.0, sec, last - first);
}

void DataBasePage::onTrimAccept() {
    if (m_trimStartSec < 0 || m_trimEndSec < 0 || m_currentSampleRow < 0) return;

    int sampleId = samplesModel->data(samplesModel->index(m_currentSampleRow, 0)).toInt();

    double lo = qMin(m_trimStartSec, m_trimEndSec);
    double hi = qMax(m_trimStartSec, m_trimEndSec);

    applyTrimToDatabase(sampleId, lo, hi);

    loadSampleToGraphs(sampleId);

    resetTrim();
}

void DataBasePage::onTrimDeny() {
    resetTrim();
}

void DataBasePage::onFlagAccept() {
    if (m_flagStartSec < 0 || m_flagEndSec < 0 || m_currentSampleRow < 0) return;

    int sampleId = samplesModel->data(samplesModel->index(m_currentSampleRow, 0)).toInt();

    int startIdx = 0;
    int endIdx = 0;

    QSqlDatabase db = m_dbManager->getDatabase(m_currentDatabase);
    if (db.isOpen()) {
        QSqlQuery query(db);
        query.prepare("SELECT timestamp FROM motion_data WHERE sample_id = :id ORDER BY sample_index");
        query.bindValue(":id", sampleId);
        if (query.exec()) {
            QVector<double> timestamps;
            while (query.next()) {
                timestamps.append(query.value(0).toLongLong() / 1000000.0);
            }
            if (!timestamps.isEmpty()) {
                double firstTime = timestamps.first();
                double lo = qMin(m_flagStartSec, m_flagEndSec);
                double hi = qMax(m_flagStartSec, m_flagEndSec);
                for (int i = 0; i < timestamps.size(); ++i) {
                    if (timestamps[i] - firstTime >= lo) {
                        startIdx = i;
                        break;
                    }
                }
                for (int i = timestamps.size() - 1; i >= 0; --i) {
                    if (timestamps[i] - firstTime <= hi) {
                        endIdx = i;
                        break;
                    }
                }
            }
        }
    }

    applyFlagsToDatabase(sampleId, startIdx, endIdx);

    btnFlagAccept->setEnabled(false);
    btnFlagDeny->setEnabled(false);
}

void DataBasePage::onFlagDeny() {
    clearFlagSeparators();
    m_flagStartSec = m_flagEndSec = -1;
    m_flagDragging = false;
    btnFlagAccept->setEnabled(false);
    btnFlagDeny->setEnabled(false);
    setFlagInteractionEnabled(false);
}

void DataBasePage::applyTrimToDatabase(int sampleId, double loSec, double hiSec) {
    QSqlDatabase db = m_dbManager->getDatabase(m_currentDatabase);
    if (!db.isOpen()) return;

    if (!db.transaction()) {
        QMessageBox::warning(this, "Error", "Failed to start transaction.");
        return;
    }

    QSqlQuery query(db);

    QSqlQuery timeQuery(db);
    timeQuery.prepare("SELECT MIN(timestamp) FROM motion_data WHERE sample_id = :id");
    timeQuery.bindValue(":id", sampleId);
    if (!timeQuery.exec() || !timeQuery.next()) {
        db.rollback();
        QMessageBox::warning(this, "Error", "Failed to read timestamps.");
        return;
    }
    const double firstTimeUs = timeQuery.value(0).toDouble();

    query.prepare("DELETE FROM motion_data WHERE sample_id = :id AND (timestamp < :lo OR timestamp > :hi)");
    query.bindValue(":id", sampleId);
    query.bindValue(":lo", qint64(firstTimeUs + loSec * 1000000.0));
    query.bindValue(":hi", qint64(firstTimeUs + hiSec * 1000000.0));

    if (!query.exec()) {
        db.rollback();
        QMessageBox::warning(this, "Error", "Failed to trim motion data: " + query.lastError().text());
        return;
    }

    query.prepare("UPDATE samples SET sample_count = (SELECT COUNT(*) FROM motion_data WHERE sample_id = :id) WHERE id = :id");
    query.bindValue(":id", sampleId);

    if (!query.exec()) {
        db.rollback();
        QMessageBox::warning(this, "Error", "Failed to update sample count: " + query.lastError().text());
        return;
    }

    if (!db.commit()) {
        QMessageBox::warning(this, "Error", "Failed to commit: " + db.lastError().text());
        return;
    }

    samplesModel->select();
    QMessageBox::information(this, "Success", "Sample trimmed successfully.");
}

void DataBasePage::onAddSelectedClicked() {
    QItemSelectionModel *selection = samplesView->selectionModel();
    if (!selection || !selection->hasSelection()) {
        QMessageBox::information(this, "No Selection", "Select samples in the table first.");
        return;
    }

    QModelIndexList selectedRows = selection->selectedRows();
    int added = 0;
    for (const QModelIndex &index : selectedRows) {
        int sampleId = samplesModel->data(samplesModel->index(index.row(), 0)).toInt();
        if (!m_selectedList.contains(sampleId)) {
            m_selectedList.append(sampleId);
            added++;
        }
    }

    updateSelectedHighlight();
    updateStatsLabel();
    QMessageBox::information(this, "Added", QString("Added %1 samples to selection list. Total: %2").arg(added).arg(m_selectedList.size()));
}

void DataBasePage::onExtractSelectedClicked() {
    if (m_selectedList.isEmpty()) {
        QMessageBox::information(this, "Empty List", "No samples in selection list.");
        return;
    }

    QString msg = "Selected samples (" + QString::number(m_selectedList.size()) + "):\n";
    for (int id : m_selectedList) {
        msg += "  ID: " + QString::number(id) + "\n";
    }
    QMessageBox::information(this, "Extract Selected", msg);

    m_selectedList.clear();
    updateSelectedHighlight();
    updateStatsLabel();
}

void DataBasePage::onDeleteFromDataSetClicked() {
    if (!samplesModel || m_currentSampleRow < 0
        || m_currentSampleRow >= samplesModel->rowCount()) {
        QMessageBox::warning(this, "No Sample", "Select a sample first.");
        return;
    }

    QModelIndex idx = samplesModel->index(m_currentSampleRow, 0);
    if (!idx.isValid()) {
        QMessageBox::warning(this, "No Sample", "Select a sample first.");
        return;
    }

    int sampleId = samplesModel->data(idx).toInt();

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Delete Sample",
        QString("Delete sample #%1 and its motion data?").arg(sampleId),
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    QSqlDatabase db = m_dbManager->getDatabase(m_currentDatabase);
    if (!db.isOpen()) return;

    if (!db.transaction()) {
        QMessageBox::warning(this, "Error", "Failed to start transaction.");
        return;
    }

    QSqlQuery query(db);
    query.prepare("DELETE FROM motion_data WHERE sample_id = :id");
    query.bindValue(":id", sampleId);
    if (!query.exec()) {
        db.rollback();
        QMessageBox::warning(this, "Error", "Failed to delete motion data: " + query.lastError().text());
        return;
    }

    query.prepare("DELETE FROM samples WHERE id = :id");
    query.bindValue(":id", sampleId);
    if (!query.exec()) {
        db.rollback();
        QMessageBox::warning(this, "Error", "Failed to delete sample: " + query.lastError().text());
        return;
    }

    if (!db.commit()) {
        QMessageBox::warning(this, "Error", "Failed to commit: " + db.lastError().text());
        return;
    }

    m_selectedList.removeAll(sampleId);

    resetTrim();
    m_frames.clear();
    if (m_plotManager != nullptr) {
        m_plotManager->clearPlot(*accelGraph);
        m_plotManager->clearPlot(*gyroGraph);
    }

    samplesModel->select();
    motionDataModel->select();
    m_currentSampleRow = -1;
    updateStatsLabel();
    updateNavigationButtons();
    updateMotionTypeLegend();
}

void DataBasePage::updateStatsLabel() {
    labelStatsChanged->setText(QString("Total added: %1").arg(m_selectedList.size()));
}

void DataBasePage::updateSelectedHighlight() {
    samplesView->viewport()->update();
}

void DataBasePage::updateMotionTypeLegend(int highlightId) {
    if (highlightId >= 0) {
        QString name = motionTypeToString(highlightId);
        motionTypeLegend->setText(QString("Motion Type: %1 (%2)").arg(name).arg(highlightId));
    } else {
        motionTypeLegend->setText("Motion Type: -");
    }
}

void DataBasePage::setupGraph() {
    if (m_plotManager == nullptr) {
        return;
    }

    static const char *accelFields[] = { BuiltinSchemas::ImuField::ax, BuiltinSchemas::ImuField::ay,
                                         BuiltinSchemas::ImuField::az };
    static const char *gyroFields[] = { BuiltinSchemas::ImuField::gx, BuiltinSchemas::ImuField::gy,
                                        BuiltinSchemas::ImuField::gz };

    const QString accelSlot = QLatin1String(kSlotDbAccel);
    const QString gyroSlot = QLatin1String(kSlotDbGyro);

    const PlotConfig accelFallback =
        defaultDbConfig("Accelerometer", "Accel (g)", accelFields, 3);
    const PlotConfig gyroFallback = defaultDbConfig("Gyroscope", "Gyro (dps)", gyroFields, 3);

    const PlotConfig accelConfig =
        m_plotConfigStore != nullptr && m_plotConfigStore->has(accelSlot)
            ? m_plotConfigStore->config(accelSlot)
            : accelFallback;
    const PlotConfig gyroConfig =
        m_plotConfigStore != nullptr && m_plotConfigStore->has(gyroSlot)
            ? m_plotConfigStore->config(gyroSlot)
            : gyroFallback;

    accelGraph = m_plotManager->createPlot(accelSlot, accelConfig, 200, 200);
    gyroGraph = m_plotManager->createPlot(gyroSlot, gyroConfig, 200, 200);
}

void DataBasePage::setupButtonsOnPage() {

// =================== LEFT SIDE ===================

    btnOpenDB = new QPushButton("Open DB");
    btnDeleteDB = new QPushButton("Delete DB");
    btnEditDB = new QPushButton("Edit DB");
    btnCreateDB = new QPushButton("Create DB");
    btnRefreshDB = new QPushButton("Refresh DB");

    if (!m_dbManager->isInitialized()) {
        btnOpenDB->setEnabled(false);
        btnDeleteDB->setEnabled(false);
        btnEditDB->setEnabled(false);
        btnCreateDB->setEnabled(false);
    }

// =================== RIGHT SIDE ===================

    btnRefreshGraph = new QPushButton("Refresh");
    btnCutGraph = new QPushButton("Cut");
    btnExportToDataSet = new QPushButton("Export this");
    btnExportAllToDataSet = new QPushButton("Export all");
    btnDeleteFromDataSet = new QPushButton("Remove from data set pack");
    btnSetPath = new QPushButton("Set path");
    btnNextSample = new QPushButton("Next sample");
    btnUndoSample = new QPushButton("Undo sample");
    btnAddSelected = new QPushButton("Add selected");
    btnExtractSelected = new QPushButton("Extract selected");
    btnTrimAccept = new QPushButton("Accept");
    btnTrimDeny = new QPushButton("Deny");
    btnFlagAccept = new QPushButton("Accept flag");
    btnFlagDeny = new QPushButton("Deny flag");

    btnTrimAccept->setFixedWidth(80);
    btnTrimDeny->setFixedWidth(80);
    btnFlagAccept->setFixedWidth(80);
    btnFlagDeny->setFixedWidth(80);
    btnTrimAccept->setEnabled(false);
    btnTrimDeny->setEnabled(false);
    btnFlagAccept->setEnabled(false);
    btnFlagDeny->setEnabled(false);
    btnUndoSample->setEnabled(false);
    btnNextSample->setEnabled(false);
}

void DataBasePage::setupsTableView() {

    treeView = new QTreeView();

    treeModel = new QStandardItemModel();

    treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    treeModel->setHorizontalHeaderLabels({"Name", "Count writes"});
    treeView->setModel(treeModel);
    treeView->setHeaderHidden(false);

    treeView->header()->setSectionResizeMode(QHeaderView::Interactive);
    treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    treeView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    parentItem = new QStandardItem();
    parentItem = treeModel->invisibleRootItem();

// =================== RIGHT SIDE ===================

    samplesModel = new QSqlTableModel(this);
    motionDataModel = new QSqlTableModel(this);

    samplesView = new QTableView();
    samplesView->setModel(samplesModel);
    samplesView->setSelectionMode(QAbstractItemView::MultiSelection);
    samplesView->setSelectionBehavior(QAbstractItemView::SelectRows);
    samplesView->horizontalHeader()->setStretchLastSection(true);
    samplesView->verticalHeader()->setVisible(false);
    samplesView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    samplesView->setAlternatingRowColors(true);

    motionDataView = new QTableView();
    motionDataView->setModel(motionDataModel);
    motionDataView->setSelectionMode(QAbstractItemView::SingleSelection);
    motionDataView->setSelectionBehavior(QAbstractItemView::SelectRows);
    motionDataView->horizontalHeader()->setStretchLastSection(true);
    motionDataView->verticalHeader()->setVisible(false);
    motionDataView->setEditTriggers(QAbstractItemView::AllEditTriggers);
}
