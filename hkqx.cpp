#include "hkqx.h"
#include"dictionary.h"
#include"qdatetime.h"
#include"qfile.h"
#include"qtextstream.h"
#include"qjsondocument.h"
#include "qdebug.h"
#include "qcoreapplication.h"
#include"qdir.h"
//获取端口号
int GetPort()
{
	return 1031;
}
//获取业务号
int GetServiceTypeID()
{
	return 5;
}

//获取业务名称
QString GetServiceTypeName()
{
	QString name = QString::fromLocal8Bit("航空气象业务");
	return name;
}

//获取版本号
QString GetVersionNo()
{
	QString Version = QString::fromLocal8Bit("1.0");
	return Version;
}
//解析数据
LRESULT Char2Json(QString &buff, QJsonObject &json)
{
	int Count = 0;//数据个数
	int Current_P = buff.length();//当前数据指

	//遍历查找数据
	for (int i = 0; i < buff.length() - 2; i++)
	{
		if (buff[i].toUpper() == 'B' && buff[i + 1].toUpper() == 'G'&&buff[i + 2] == ',')
		{
			Current_P = i;//指针指向帧头
			for (int j = i + 2; j < buff.length() - 2; j++)
			{
				if (buff[j] == ','&&buff[j + 1].toUpper() == 'E'&&buff[j + 2].toUpper() == 'D')
				{
					Current_P = j + 3;//指针移动到帧尾下一个字符
					Count += 1;//数据个数
					Dictionary dic;
					//根据“，”将字符串拆分成各个要素
					QString strBuff = buff.mid(i, j - i + 3);
					QStringList strlist = strBuff.split(",");
					QJsonObject SubJson;
					SubJson.insert("DataType", 1);//数据类型 观测数据
					//区站号
					SubJson.insert("StationID", strlist.at(1));
					//05航空气象站
					SubJson.insert("ServiceTypeID", HKQX);
					//数据类型 00主动传输 01被动传输
					SubJson.insert("DataTypeID", strlist.at(3));
					//时间
					QString time = Convert2Time(strlist.at(4));
					SubJson.insert("ObserveTime", time);

					//时间格式2
					QString timeform = Convert2TimeForm(strlist.at(4));
					SubJson.insert("UploadTime", timeform);
					bool ok;
					//观察要素个数
					int CountOfFeature = ((QString)strlist.at(5)).toInt(&ok, 10);
					SubJson.insert("CountOfFeature", CountOfFeature);
					int CountOfFacilitiesStatus = ((QString)strlist.at(6)).toInt(&ok, 10);
					//状态要素个数
					SubJson.insert("CountOfFacilitiesStatus", CountOfFacilitiesStatus);
					bool isAV_VV = false;
					//观察要素
					QString strFeatureName;
					for (int i = 7; i < CountOfFeature * 2 + 7; i += 2)
					{
						QString key = dic.Find(QString(strlist.at(i)).toLower());
						if (key != NULL)
						{
							if (strlist.at(i).toLower() == ("aoa") && strlist.at(i + 1) == "4")
								//云状态=4，垂直能见度
								isAV_VV = true;
							//第一层云高换成垂直能见度
							if (isAV_VV == true && (strlist.at(i).toLower().compare("aod") == 0))
							{
								key = "AV_VV";
								isAV_VV = false;
							}
							//质量控制码
							if (i == 7)
							{
								strFeatureName = key;
							}
							else
							{
								strFeatureName += ",";
								strFeatureName += key;
							}

							SubJson.insert(key, strlist.at(i + 1));
						}
					}
					SubJson.insert("StatusBitName", strFeatureName);
					//状态位
					SubJson.insert("StatusBit", strlist.at(CountOfFeature * 2 + 7));
					//设备状态
					for (int i = CountOfFeature * 2 + 8; i < CountOfFeature * 2 + CountOfFacilitiesStatus * 2 + 8; i += 2)
					{
						QString key = dic.Find(QString(strlist.at(i)).toLower());
						if (key != NULL)
							SubJson.insert(key, strlist.at(i + 1));
					}
					//数据备份
					QDateTime current_date_time = QDateTime::currentDateTime();
					QString current_date = current_date_time.toString("yyyy.MM.dd hh:mm:ss");
					QString current_day = current_date_time.toString("yyyy-MM-dd");
					QString fileName = QCoreApplication::applicationDirPath() + "\\HKQX\\" + strlist.at(1) + "\\" + current_day;
					QDir dir(fileName);
					if (!dir.exists())
						dir.mkpath(fileName);//创建多级目录
					fileName += "\\data.txt";
					QFile file(fileName);

					if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
					{}
					QTextStream in(&file);
					in << current_date << "\r\n" << strBuff<<"\r\n";
					file.close();
					json.insert(QString::number(Count), SubJson);
					i = j + 2;//当前循环
					break;
				}
			}
		}
		else if (buff[i] == '<')
		{
			Current_P = i;//指针指向帧头
			for (int j = i + 1; j < buff.length(); j++)
			{
				if (buff[j] == '>')
				{
					Current_P = j + 1;//指针移动到帧尾
					Count += 1;//数据个数
					QString subStr = buff.mid(i + 1, j - i - 1);
					QStringList strlist = subStr.split(",");
					QJsonObject SubJson;
					i = j;
					switch (strlist.count())
					{
					
					//心跳
					case 2:
					{
						SubJson.insert("DataType", 3);//数据类型 3心跳数据
						SubJson.insert("ServiceTypeID", strlist.at(0));
						SubJson.insert("StationID", strlist.at(1));
						json.insert(QString::number(Count), SubJson);
						break;
					}
					//操作返回值
					case 7:
					{
						SubJson.insert("DataType", 2);//数据类型 2操作数据
						SubJson.insert("ValueCount", 7);
						SubJson.insert("ServiceTypeID", strlist.at(0));
						SubJson.insert("StationID", strlist.at(1));
						SubJson.insert("Command", strlist.at(2));
						SubJson.insert("RecvValue1", strlist.at(3));
						SubJson.insert("RecvValue2", strlist.at(4));
						SubJson.insert("RecvValue3", strlist.at(5));
						SubJson.insert("RecvValue4", strlist.at(6));
						json.insert(QString::number(Count), SubJson);
						break;
					}
					//无效数据
					default:
						break;
					}
					break;
				}
			}
		}
	}
	json.insert("DataLength", Count);//JSON数据个数
	if (Current_P >= buff.length())//判断当前指针位置
	{
		buff.clear();
	}//清除内存
	else
	{
		buff.remove(0, Current_P);
	}//将剩余字节存入缓存
	return 1;
}

//字符串转成数据库时间格式
QString Convert2Time(QString strTime)
{
	QString tmp;
	tmp = "to_date('" + strTime.mid(0, 4) + "-" + strTime.mid(4, 2) + "-" + strTime.mid(6, 2) + " " + strTime.mid(8, 2) + ":" + strTime.mid(10, 2) + ":" + strTime.mid(12, 2) + "', 'yyyy-mm-dd hh24:mi:ss')";
	return tmp;
}

//字符串转成显示时间格式
QString Convert2TimeForm(QString strTime)
{
	QString tmp;
	tmp = strTime.mid(0, 4) + "-" + strTime.mid(4, 2) + "-" + strTime.mid(6, 2) + " " + strTime.mid(8, 2) + ":" + strTime.mid(10, 2) + ":" + strTime.mid(12, 2);
	return tmp;
}
