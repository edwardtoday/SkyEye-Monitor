#include "sqlexample.h"
#include "ui_sqlexample.h"
#include <QtSql>
#include <QDebug>
#include <QtWidgets>
#include <QJsonDocument>

SQLExample::SQLExample(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SQLExample)
{
    ui->setupUi(this);
}

SQLExample::~SQLExample()
{
    delete ui;
}

void SQLExample::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void SQLExample::connectDatabase()
{
  QFile jsonFile("config.json");

  if(!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    qDebug() << "Unable to open config file!";
    return;
  }

  QByteArray jsonData = jsonFile.readAll();
  jsonFile.close();
  QJsonParseError *err = new QJsonParseError();
  QJsonDocument doc = QJsonDocument::fromJson(jsonData, err);

  if (err->error != 0)
    qDebug() << err->errorString();

  if (doc.isNull())
  {
    qDebug() << "Invalid config file format!";
    return;
  }
  else if (doc.isObject())
  {
    QJsonObject jObject = doc.object();
    QVariantMap config = jObject.toVariantMap();
    QString dbmstype = config["dbmstype"].toString();
    QString hostname = config["hostname"].toString();
    QString database = config["database"].toString();
    QString username = config["username"].toString();
    QString password = config["password"].toString();
    db = new QSqlDatabase(QSqlDatabase::addDatabase(dbmstype));
    db->setHostName(hostname);
    db->setDatabaseName(database);
    db->setUserName(username);
    db->setPassword(password);
  }
    if (!db->open())
        qDebug() << "Failed to connect to database";
    else {
        qDebug() << "Database connection OK";
        qDebug() << db->databaseName();
        qDebug() << db->tables();
    }

    refresh();
}

static QString qDBCaption(const QSqlDatabase &db)
{
    QString dbCaption = db.driverName();
    dbCaption.append(QLatin1Char(':'));
    if (!db.userName().isEmpty())
        dbCaption.append(db.userName().append(QLatin1Char('@')));
    dbCaption.append(db.databaseName());
    return dbCaption;
}

void SQLExample::refresh()
{
    ui->dbTree->clear();
    QTreeWidgetItem *root = new QTreeWidgetItem(ui->dbTree);
    root->setText(0,qDBCaption(*db));
    if (db->isOpen()) {
        QStringList tables = db->tables();
        for (int t = 0; t < tables.count(); ++t) {
            QTreeWidgetItem *table = new QTreeWidgetItem(root);
            table->setText(0, tables.at(t));
        }
    }
    ui->dbTree->doItemsLayout();
}

void SQLExample::showTable(const QString &t)
{
    qDebug() << "showTable: " << t;
    QSqlTableModel *model = new QSqlTableModel(ui->dbTable, *db);
    model->setEditStrategy(QSqlTableModel::OnRowChange);
    model->setTable(db->driver()->escapeIdentifier(t, QSqlDriver::TableName));
    model->select();
//    if (model->lastError().type() != QSqlError.NoError)
//        emit statusMessage(model->lastError().text());
    ui->dbTable->setModel(model);
    ui->dbTable->setEditTriggers(QAbstractItemView::DoubleClicked|QAbstractItemView::EditKeyPressed);

    connect(ui->dbTable->selectionModel(),SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),this,SLOT(currentChanged()));
}

void SQLExample::on_dbTree_itemActivated(QTreeWidgetItem *item, int column)
{
    qDebug() << "itemActivated: " << item->text(0);
    if (!item)
        return;
    if (item->parent()) {
        showTable(item->text(0));
    }
}


void SQLExample::exec()
{
    QSqlQueryModel *model = new QSqlQueryModel(ui->dbTable);
    model->setQuery(QSqlQuery(ui->queryText->toPlainText(), *db));
    ui->dbTable->setModel(model);

    if (model->lastError().type() != QSqlError::NoError)
        emit statusMessage(model->lastError().text());
    else if (model->query().isSelect())
        emit statusMessage(tr("Query OK."));
    else
        emit statusMessage(tr("Query OK, number of rows affected: %1").arg(model->query().numRowsAffected()));
}

void SQLExample::on_clearButton_clicked()
{
    ui->queryText->clear();
}

void SQLExample::on_execButton_clicked()
{
    exec();
}
