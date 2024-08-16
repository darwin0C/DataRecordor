#include "dboperator.h"

extern void PublishEvent(QString topic,QVariant data);

DbOperator::DbOperator(QObject *parent) : QObject(parent)
{

}
///***************************************
///*插入数据
///*****************************************
//保存目标数据
//bool DbOperator::saveTarData(const IDB_TargetData &tarData)
//{
//    return  DbManipulation::Get()->insertTargetData(tarData);
//}

////保存设备状态数据
//bool DbOperator::saveEquStatData(const IEqu_WorkStat &equData)
//{
//    return  DbManipulation::Get()->insertEquWorkStat(equData);
//}

///****************************************
///*更新数据
///******************************************/
//更新目标数据
//bool DbOperator::updateTarData(const IDB_TargetData &tarData)
//{
//    return  DbManipulation::Get()->updateTargetData(tarData);
//}




///****************************************
///*查询
///******************************************/

//查询设备状态数据
//QList<IEqu_WorkStat> DbOperator::queryEquStatData()
//{
//    return DbManipulation::Get()->fetchAllEquWorkStats();
//}



///***************************************
///*删除
///*****************************************

//删除目标数据
bool DbOperator::DeleteTarData(QString id)
{
    return DbManipulation::Get()->deleteData(DB_target_data,id);
}
//删除目标数据
bool DbOperator::DeleteTaskData(QString id)
{
    return DbManipulation::Get()->deleteData(DB_reconTask_data,id);
}
//删除设备属性数据
bool DbOperator::DeleteEquAbilityData(QString id)
{
    return DbManipulation::Get()->deleteData(DB_Equ_Ability,id);
}
//删除设备状态数据
bool DbOperator::DeleteEquStatData(QString id)
{
    return DbManipulation::Get()->deleteData(DB_Equ_WorkStat,id);
}
