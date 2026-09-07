#include "pages/createdbdialog.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDebug>
#include <QStringList>
#include <QRegularExpressionValidator>

CreateDBDialog::CreateDBDialog(QWidget *parent) {
    setupUI();
}

QStringList CreateDBDialog::returnDBdata() {
    QStringList data;
    data.append(m_fileName->text());
    data.append(m_pathEdit->text());
    data.append(m_textBrowser->toPlainText());
    return data;
}

void CreateDBDialog::onOkClicked()
{
    if ( !m_pathEdit->text().isEmpty() && !m_textBrowser->toPlainText().isEmpty() && !m_fileName->text().isEmpty())
    {
        accept();
    }
    else {
        qDebug() << "File name, file path or description is empty";
    }

}

void CreateDBDialog::onCancelClicked() {
    reject();
}

void CreateDBDialog::onEditPathClicked() {
    QString dirPath = QFileDialog::getExistingDirectory(
            this,
            tr("Select folder for database"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
        );

    if (dirPath.isEmpty()) {
        return;
    }

    m_pathEdit->setText(dirPath);

}

void CreateDBDialog::setupUI() {

    setWindowTitle("Create database");
    setMinimumSize(400, 250);
    setModal(true);

    m_mainLayout = new QVBoxLayout(this);
    m_buttonsLayout = new QHBoxLayout;
    m_setPathLayout = new QHBoxLayout;

    m_textBrowser = new QTextEdit;
    m_textBrowser->setPlaceholderText("Description: ");

    m_info1 = new QLabel("Write description for database");
    m_info2 = new QLabel("Set file parametrs");

    m_pathEdit = new QLineEdit;
    m_pathEdit->setPlaceholderText("Set file path");
    QRegularExpression pathRx("[a-zA-Z0-9а-яА-Я.:_ /\\\\-]+");
    QValidator *pathValidator = new QRegularExpressionValidator(pathRx, this);
    m_pathEdit->setValidator(pathValidator);

    m_fileName = new QLineEdit;
    m_fileName->setPlaceholderText("Set file name");
    QRegularExpression fileNameRx("[a-zA-Z0-9а-яА-Я._ -]+");
    QValidator *fileNameValidator = new QRegularExpressionValidator(fileNameRx, this);
    m_fileName->setValidator(fileNameValidator);

    m_btnOk = new QPushButton("Ok");
    m_btnCancel = new QPushButton("Cancel");
    m_btnSetFilePath = new QPushButton("Set path db");

    m_setPathLayout->addWidget(m_pathEdit);
    m_setPathLayout->addWidget(m_btnSetFilePath);

    m_buttonsLayout->addWidget(m_btnOk);
    m_buttonsLayout->addWidget(m_btnCancel);

    m_mainLayout->addWidget(m_info1);
    m_mainLayout->addWidget(m_textBrowser);
    m_mainLayout->addWidget(m_info2);
    m_mainLayout->addWidget(m_fileName);
    m_mainLayout->addLayout(m_setPathLayout);
    m_mainLayout->addLayout(m_buttonsLayout);
    m_mainLayout->addStretch();

    connect(m_btnOk, &QPushButton::clicked,
            this, &CreateDBDialog::onOkClicked);
    connect(m_btnCancel, &QPushButton::clicked,
            this, &CreateDBDialog::onCancelClicked);
    connect(m_btnSetFilePath, &QPushButton::clicked,
            this, &CreateDBDialog::onEditPathClicked);
}
