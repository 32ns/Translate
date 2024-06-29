/********************************************************************************
** Form generated from reading UI file 'widget.ui'
**
** Created by: Qt User Interface Compiler version 6.7.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_WIDGET_H
#define UI_WIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Widget
{
public:
    QGridLayout *gridLayout;
    QVBoxLayout *verticalLayout_2;
    QVBoxLayout *verticalLayout;
    QTextEdit *txt_source;
    QTextEdit *txt_target;
    QPushButton *btn_translate;

    void setupUi(QWidget *Widget)
    {
        if (Widget->objectName().isEmpty())
            Widget->setObjectName("Widget");
        Widget->resize(800, 600);
        gridLayout = new QGridLayout(Widget);
        gridLayout->setObjectName("gridLayout");
        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName("verticalLayout_2");
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName("verticalLayout");
        txt_source = new QTextEdit(Widget);
        txt_source->setObjectName("txt_source");
        txt_source->setMaximumSize(QSize(16777215, 100));
        QFont font;
        font.setPointSize(12);
        txt_source->setFont(font);

        verticalLayout->addWidget(txt_source);

        txt_target = new QTextEdit(Widget);
        txt_target->setObjectName("txt_target");
        txt_target->setFont(font);
        txt_target->setReadOnly(true);

        verticalLayout->addWidget(txt_target);


        verticalLayout_2->addLayout(verticalLayout);

        btn_translate = new QPushButton(Widget);
        btn_translate->setObjectName("btn_translate");
        btn_translate->setMinimumSize(QSize(0, 50));

        verticalLayout_2->addWidget(btn_translate);


        gridLayout->addLayout(verticalLayout_2, 0, 0, 1, 1);


        retranslateUi(Widget);

        QMetaObject::connectSlotsByName(Widget);
    } // setupUi

    void retranslateUi(QWidget *Widget)
    {
        Widget->setWindowTitle(QCoreApplication::translate("Widget", "Translate", nullptr));
        btn_translate->setText(QCoreApplication::translate("Widget", "\347\277\273\350\257\221", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Widget: public Ui_Widget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_WIDGET_H
