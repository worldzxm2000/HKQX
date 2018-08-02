#ifndef DICTIONARY_H
#define DICTIONARY_H

#include <QObject>
#include<QMap>
class Dictionary
{
public:
    Dictionary();
    QString Find(QString key);
	//QString FindCN(QString key);
protected:
   QMap<QString,QString> map;
  // QMap<QString, QString> map_cn;
};

#endif // DICTIONARY_H
