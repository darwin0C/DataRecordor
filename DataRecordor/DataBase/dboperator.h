#ifndef DBOPERATOR_H
#define DBOPERATOR_H

#include <QObject>
#include "dbDatas.h"
#include "dbmanipulation.h"


class DbOperator : public QObject
{
    Q_OBJECT

public:
    explicit DbOperator(QObject *parent = nullptr) ;


    bool DeleteTaskData(QString id) ;
    bool DeleteEquAbilityData(QString id) ;
    bool DeleteEquStatData(QString id) ;
    bool DeleteTarData(QString id) ;


signals:

};

#endif // DBOPERATOR_H
