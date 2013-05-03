#ifndef _APPCAFE_EXTRAS_H
#define _APPCAFE_EXTRAS_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDebug>

class Extras{

public:
  static bool checkUser(bool wardenMode){
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString logname;
    if(env.contains("LOGNAME")){ logname = env.value("LOGNAME"); }
    else if(env.contains("USERNAME")){ logname = env.value("USERNAME"); }
    else if(env.contains("USER")){ logname = env.value("USER"); }
    bool ok = FALSE;
    if( logname.isEmpty() ){}
    else if( wardenMode && (logname=="root") ){ ok = TRUE;}
    else if( !wardenMode && (logname!="root") ){ ok = TRUE; }
    return ok;
  }
  
  static QString bytesToHumanReadable(float bytes){
    QStringList suffix;
    suffix << "B" << "KB" << "MB" << "GB" << "TB" << "PB";
    int i = 0;
    while((bytes >= 1000) && ( i < (suffix.size() - 1 )) ){
	bytes = bytes / 1024;  i++;
    }
    QString result = QString::number(bytes, 'f', 0);
    result += suffix[i];
    return result;
  }
  
  static QString getLineFromCommandOutput( QString command ){
	FILE *file = popen(command.toLatin1(),"r"); 
	char buffer[100];
	QString line = ""; 
	char firstChar;
	if ((firstChar = fgetc(file)) != -1){
		line += firstChar;
		line += fgets(buffer,100,file);
	}
	pclose(file);
	return line;
  }
  
  static QString getSystemArch(){
    return getLineFromCommandOutput("uname -m").simplified();
 }
 
 static QStringList readFile( QString filePath ){
   QFile file(filePath);
   QStringList output;
   if(file.open(QIODevice::ReadOnly | QIODevice::Text)){
     QTextStream in(&file);
     while(!in.atEnd()){ output << in.readLine(); }
     file.close();
   }
   return output;
 }
 
 static QString nameToID(QString name){
   QString out = name.toLower();
   out.remove(" ");
   out.remove("\t");
   out.simplified();
   return out;
 }
 
 static bool newerDateTime(QString check, QString standard){
   //Returns true if the first input is a later date/time than the second
   double chkNum = check.remove(" ").toDouble();
   double stdNum = standard.remove(" ").toDouble();
   return (chkNum > stdNum);
 }
 
 static QString datetimeToString(QString datetime){
   //datetime format: "YYYYMMDD HHMMSS"
   //converts a database date/time string to human readable form
   QString date = datetime.section(" ",0,0,QString::SectionSkipEmpty);
   //date format: YYYYMMDD
   QString year = date.left(4);
   QString day = QString::number(date.right(2).toInt());
   date.chop(2); // remove the day
   QString month = QString::number(date.right(2).toInt());
   
   QString output = month+"/"+day+"/"+year;
   return output;
 }
 
 static QString sizeKToDisplay(QString sizeK){
   double num = sizeK.toDouble();
   QStringList lab; lab << "KB" << "MB" << "GB" << "TB" << "PB";
   int i=0;
   while( (i<lab.length()) && (num > 1024) ){
     num=num/1024; i++;	   
   }
   //Round to 2 decimel places
   num = int(num*100)/100.0;
   QString output = QString::number(num)+" "+lab[i];
   //qDebug() << "Size calculation:" << sizeK << output;
   return output;
 }
 
};

#endif
