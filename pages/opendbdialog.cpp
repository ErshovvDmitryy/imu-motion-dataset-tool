#include "pages/opendbdialog.h"

#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>


OpenDBDialog::OpenDBDialog(const QStringList &registeredDbs,
                                       QWidget *parent)
    : QDialog(parent)
    , m_registeredDbs(registeredDbs)
    , m_selectedDatabase(QString())
    , m_isCustomPath(false)
{
    setupUI();
    loadRegisteredDatabases(registeredDbs);
}

void OpenDBDialog::setupUI() {
    setWindowTitle("Open Database");
    setMinimumSize(500, 400);
    setModal(true);

    m_mainLayout = new QVBoxLayout(this);

    m_titleLabel = new QLabel("Select a database to open:");
    m_mainLayout->addWidget(m_titleLabel);

    m_topLayout = new QHBoxLayout();

    m_dbList = new QListWidget();
    m_dbList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_dbList->setMinimumHeight(200);
    m_topLayout->addWidget(m_dbList, 2);

    QVBoxLayout *rightLayout = new QVBoxLayout();

    m_refreshBtn = new QPushButton("Refresh");
    m_customBtn = new QPushButton("Browse...");
    m_customBtn->setToolTip("Select database file from filesystem");

    rightLayout->addWidget(m_refreshBtn);
    rightLayout->addWidget(m_customBtn);
    rightLayout->addStretch();

    m_topLayout->addLayout(rightLayout, 1);
    m_mainLayout->addLayout(m_topLayout);

    QHBoxLayout *pathLayout = new QHBoxLayout();
    QLabel *pathLabel = new QLabel("Path:");
    pathLayout->addWidget(pathLabel);

    m_pathEdit = new QLineEdit();
    m_pathEdit->setPlaceholderText("Selected file path will appear here...");
    m_pathEdit->setReadOnly(true);
    pathLayout->addWidget(m_pathEdit);

    m_mainLayout->addLayout(pathLayout);

    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->addStretch();

    m_okBtn = new QPushButton("OK");
    m_okBtn->setEnabled(false);

    m_cancelBtn = new QPushButton("Cancel");

    m_buttonLayout->addWidget(m_okBtn);
    m_buttonLayout->addWidget(m_cancelBtn);
    m_mainLayout->addLayout(m_buttonLayout);

    connect(m_dbList, &QListWidget::itemDoubleClicked,
            this, &OpenDBDialog::onItemDoubleClicked);
    connect(m_dbList, &QListWidget::itemSelectionChanged,
            [this]() {
                if (m_dbList->currentItem()) {
                    m_okBtn->setEnabled(true);
                    m_isCustomPath = false;
                    m_pathEdit->clear();
                }
            });

    connect(m_customBtn, &QPushButton::clicked,
            this, &OpenDBDialog::onSelectCustom);
    connect(m_okBtn, &QPushButton::clicked,
            this, &OpenDBDialog::onOkClicked);
    connect(m_cancelBtn, &QPushButton::clicked,
            this, &OpenDBDialog::onCancelClicked);
}

void OpenDBDialog::loadRegisteredDatabases(const QStringList &dbs) {
    m_dbList->clear();

    if (dbs.isEmpty()) {
        m_dbList->addItem("No registered databases found");
        m_dbList->item(0)->setFlags(Qt::NoItemFlags);
        return;
    }

    for (const QString &db : dbs) {
        m_dbList->addItem(db);
    }

    if (m_dbList->count() > 0) {
        m_dbList->setCurrentRow(0);
        m_okBtn->setEnabled(true);
    }
}

void OpenDBDialog::onSelectFromList() {
    // Этот слот вызывается при выборе из списка
    // Реализовано через сигнал itemSelectionChanged
}

void OpenDBDialog::onSelectCustom() {
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Select Database File",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "Database Files (*.db *.sqlite *.sqlite3);;All Files (*)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    m_pathEdit->setText(filePath);

    m_dbList->clearSelection();

    m_okBtn->setEnabled(true);
    m_isCustomPath = true;
    m_selectedDatabase = filePath;
}

void OpenDBDialog::onItemDoubleClicked() {
    if (m_dbList->currentItem()) {
        m_selectedDatabase = m_dbList->currentItem()->text();
        m_isCustomPath = false;
        accept();
    }
}

void OpenDBDialog::onOkClicked() {
    if (m_isCustomPath) {
        if (m_pathEdit->text().isEmpty()) {
            QMessageBox::warning(this, "No File", "Please select a file!");
            return;
        }
        m_selectedDatabase = m_pathEdit->text();
    } else {
        if (!m_dbList->currentItem()) {
            QMessageBox::warning(this, "No Selection", "Please select a database!");
            return;
        }
        m_selectedDatabase = m_dbList->currentItem()->text();
    }

    accept();
}

void OpenDBDialog::onCancelClicked() {
    reject();
}

QString OpenDBDialog::getSelectedDatabase() const {
    return m_selectedDatabase;
}
