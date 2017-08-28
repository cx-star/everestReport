#include <QClipboard>
#include <QMap>
#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>
#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::on_pushButtonOpenReport_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,"打开everest报告",QCoreApplication::applicationDirPath(),"*.txt");
    if(fileName.isNull())
        return;
    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QTextStream in(&file);

    int start,end;
    QTreeWidgetItem *item1=0,*item2=0,*item3=0,*item4=0;

    //左侧视图
    ui->treeWidget->clear();
    QStringList headerLabelsList;
    headerLabelsList.append("项目");
    headerLabelsList.append("值");
    ui->treeWidget->setHeaderLabels(headerLabelsList);
    ui->treeWidget->setColumnCount(2);

    //四级标题最长长度
    int max4Title = 0;

    while(!in.atEnd())
    {
        QString line = in.readLine();
        if(line.isEmpty())
            continue;

        if(line.startsWith("--------[ "))//一级标题
        {
            start = line.indexOf("[ ")+2;
            end = line.indexOf(" ]")-1;
            item1 = new QTreeWidgetItem(ui->treeWidget);
            item1->setText(0,line.mid(start,end-start+1));
            item2=0;
        }
        else if(line.startsWith("  [ "))//二级标题
        {
            start = line.indexOf("[ ")+2;
            end = line.indexOf(" ]")-1;
            item2 = new QTreeWidgetItem(item1);
            item2->setText(0,line.mid(start,end-start+1));
        }
        else if(line.startsWith("    ")&&!line.startsWith("      "))//三级标题
        {
            start = line.indexOf(QRegularExpression("\\S"));//非空白
            end = line.indexOf("  ",start)-1;//没有找到则返回-1，
            item3 = new QTreeWidgetItem(item2==0?item1:item2);
            QString text = line.mid(start,end-start+1);
            item3->setText(0,text);//end=-1时，返回start后所有字符
            if(end>0)//有值
            {
                start = line.indexOf(QRegularExpression("\\S"),end+1);//非空白
                end = line.indexOf("  ",start)-1;//没有找到则返回-1，
                QString text = line.mid(start/*,end-start+1*/);
                item3->setText(1,text);//end=-1时，返回start后所有字符
            }
        }
        else if(line.startsWith("      "))//四级标题
        {
            start = line.indexOf(QRegularExpression("\\S"));
            end = line.indexOf("  ",start)-1;
            if(end==-2&&max4Title!=0){//期望第一次时不是‘标题过长’状态
                end = max4Title-1;
            }
            item4 = new QTreeWidgetItem(item3==0?item2:item3);
            QString text = line.mid(start,end-start+1);
            item4->setText(0,text);//end=-1时，返回start后所有字符
            if(end>0)//有值
            {
                start = line.indexOf(QRegularExpression("\\S"),end+1);//非空白
                if(max4Title==0){//期望第一次时不是‘标题过长’状态
                    max4Title=line.mid(0,start).toLocal8Bit().length();
                    qDebug()<<max4Title;
                }
                //end = line.indexOf("  ",start)-1;//没有找到则返回-1
                QString text = line.mid(start/*,end-start+1*/);//返回全部
                item4->setText(1,text);//end=-1时，返回start后所有字符
            }
        }


    }
}
QTreeWidgetItem* findFirstChildTreeWidgetItem(QTreeWidgetItem* parent,const QString match,int column=0){
    for(int i=0;i<parent->childCount();i++){
        if(match==parent->child(i)->text(column)){
            return parent->child(i);
        }
    }
    return 0;
}
QTreeWidgetItem* findFirstTopTreeWidgetItem(QTreeWidget* treeWidget,const QString match,int column=0){
    for(int i=0;i<treeWidget->topLevelItemCount();i++){
        if(match==treeWidget->topLevelItem(i)->text(column)){
            return treeWidget->topLevelItem(i);
        }
    }
    return 0;
}

void Widget::on_pushButtonRefresh_clicked()
{
    QFile file(QCoreApplication::applicationDirPath()+"/setting.ini");
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QTextStream in(&file);

    ui->tableWidget->clear();
    QList<QString> mapList;
    QMap<QString,QStringList> map;//
    //When iterating over a QHash, the items are arbitrarily ordered.
    //With QMap, the items are always sorted by key.

    while(!in.atEnd())//存储要显示的值列表和对应表达式，1操作系统名称="系统概述-计算机:-操作系统"
    {
        QString line = in.readLine();
        if(line.isEmpty())
            continue;
        int index = line.indexOf("=\"");
        QString name = line.mid(0,index);//1操作系统名称
        QString value = line.mid(index+2);
        value.resize(value.size()-1);
        QStringList valueList = value.split("-");//系统概述  计算机:  操作系统
        qDebug()<<name<<" "<<valueList;
        map[name] = valueList;
        mapList.append(name);
    }
    //设置视图行列
    ui->tableWidget->setColumnCount(2);
    ui->tableWidget->setRowCount(map.size());

    int i=0;
    //QMap<QString, QStringList>::const_iterator iterator = map.constBegin();
    bool hasMark=false;//是否有问号
    int childIndex = 0;//问号时，所有的子路径序号
    //while (iterator != mapList.constEnd()) {//setting.ini的每一行
    int mapListI;
    for(mapListI=0;mapListI<mapList.size();){

        QString key = mapList.at(mapListI);
        QStringList mapValue = map[key];
        if(!hasMark){
            //ui->tableWidget->setItem(i,0,new QTableWidgetItem(iterator.key()));//设置 名称
            ui->tableWidget->setItem(i,0,new QTableWidgetItem(key));//设置 名称
        }

        QTreeWidgetItem* parentItem;
        //for(int ii=0;ii<iterator.value().size();ii++){//遍历路径//eg:系统概述  计算机:  操作系统
        for(int ii=0;ii<mapValue.size();ii++){//遍历路径//eg:系统概述  计算机:  操作系统
            QString findTarget = mapValue.at(ii);//每一个路径

            if(ii==0){
                parentItem = findFirstTopTreeWidgetItem(ui->treeWidget,findTarget);
                qDebug()<<"findFirstTopTreeWidgetItem "<<findTarget;
            }
            else
            {
                if(findTarget=="?"){
                    hasMark = true;
                    findTarget = parentItem->child(childIndex)->text(0);
                    childIndex++;
                    if(childIndex==parentItem->childCount()){
                        hasMark = false;//
                    }
                    qDebug()<<"findTarget==? "<<childIndex<<findTarget;
                }
                parentItem = findFirstChildTreeWidgetItem(parentItem,findTarget,0);
                qDebug()<<"findFirstChildTreeWidgetItem "<<findTarget;
            }
            if(parentItem==0)//路径错误。没有找到返回0
                break;
        }
        if(parentItem!=0)//找到了
        {
            if(ui->tableWidget->item(i,1)){
                QString text = ui->tableWidget->item(i,1)->text();
                text.append("\r\n"+parentItem->text(1));
                ui->tableWidget->item(i,1)->setText(text);
            }
            else
            {
                ui->tableWidget->setItem(i,1,new QTableWidgetItem(parentItem->text(1)));//设置 名称
            }
        }
        if(!hasMark)//问号未处理完时，不增加
        {
            i++;
            //++iterator;
            mapListI++;
            childIndex=0;
        }
    }

}

void Widget::on_pushButtonPaste_clicked()
{
    int column = ui->tableWidget->columnCount();
    int row = ui->tableWidget->rowCount();
    qDebug()<<"column"<<column<<" row"<<row;
    QString result;
    for(int i=0;i<row;i++){
        if(i>0)
            result.append(ui->radioButtonH->isChecked()?"\t":"\r\n");
        QTableWidgetItem *item = ui->tableWidget->item(i,1);
        QString ss=item->text();
        if(ss.contains("\r\n")){
            ss.prepend("\"");
            ss.append("\"");
        }
        result.append(item==0?"?":ss);
    }
    QApplication::clipboard()->setText(result);
}
