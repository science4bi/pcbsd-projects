#include "LPGUtils.h"

LPDataset LPGUtils::loadPoolData(QString zpool){
  //Load the current information for the given zpool
  qDebug() << "New Dataset: " << zpool;
  LPDataset DSC;
  //List all the mountpoints in this dataset
  QStringList subsets = LPBackend::listDatasetSubsets(zpool);
  QStringList lpsnaps = LPBackend::listLPSnapshots(zpool);
  //populate the list of snapshots available for each mountpoint
  for(int i=0; i<subsets.length(); i++){
    //qDebug() << "Subset:" << subsets[i];
    QStringList snaps = LPBackend::listSnapshots(subsets[i]);
    //qDebug() << " - Snapshots:" << snaps;
    if(snaps.isEmpty()){
      //invalid subset - remove it from the list
      subsets.removeAt(i);
      i--;
    }else{
      QStringList subsnaps;
      //only list the valid snapshots that life preserver created
      for(int s=0; s<lpsnaps.length(); s++){
	int index = snaps.indexOf(lpsnaps[s]);
        if(index > -1){ subsnaps << lpsnaps[s]; snaps.removeAt(index); }
      }
      /*//Now list all the other available snapshots (no certain ordering)
      if(!snaps.isEmpty()){try 
	subsnaps << "--"; //so we know that this is a divider between the sections
	subsnaps << snaps;
      }*/
      DSC.subsetHash.insert(subsets[i],subsnaps); //add it to the internal container hash
    }
  }
  //Parse "zpool status <pool>"
  QStringList zstat = LPBackend::getCmdOutput("zpool status "+zpool);
  //qDebug() << "zpool status "+zpool+":\n" << zstat.join("\n");
  bool atheader=false;
  QStringList disks, diskstates, running, errors, finished;
  for(int i=0; i<zstat.length(); i++){
    if(zstat[i].isEmpty()){ continue; }
    else if(zstat[i].startsWith("  scan:")){
      QString line = zstat[i].section(":",1,50).replace("\t"," ").simplified();
      if(line.contains("scrub repaired ")){
	QString timestamp = line.section(" ",9,13,QString::SectionSkipEmpty);
	QString numerrors = line.section(" ",6,6,QString::SectionSkipEmpty);
        finished << QString(QObject::tr("Scrub Finished: %1 (%2 errors)")).arg(timestamp, numerrors);
      }else if(line.contains("scrub")){
	QString timestamp = line.section(" ",4,8,QString::SectionSkipEmpty);
        running << QString(QObject::tr("Scrub Started: %1")).arg(timestamp);      
      }else if(line.contains("resilvered")){
	QString timestamp = line.section(" ",9,13,QString::SectionSkipEmpty);
	QString numerrors = line.section(" ",6,6,QString::SectionSkipEmpty);
        finished << QString(QObject::tr("Resilver Finished: %1 (%2 errors)")).arg(timestamp, numerrors);
      }
    }else if(zstat[i].contains("NAME") && zstat[i].contains("STATE") && zstat[i].contains("READ") ){
      //qDebug() <<"Found header"; 
      atheader=true;
    }else if(zstat[i].startsWith("errors:")){ 
      atheader=false; 
    }else if(atheader){
      QString line = zstat[i].replace("\t"," ").simplified();
      QString dev = line.section(" ",0,0,QString::SectionSkipEmpty);
      QString state = line.section(" ",1,1,QString::SectionSkipEmpty);
      //qDebug() << "Found device:" << dev << state;
      if(dev == zpool){
	DSC.poolStatus = state;
      }else if(line.contains("(resilvering)")){
	disks << dev; diskstates << state; //record this disk and state
	running << QString(QObject::tr("%1: Currently Resilvering")).arg(dev);
      }else{
	disks << dev; diskstates << state; //record this disk and state
	if(state != "ONLINE"){
	  errors << QString(QObject::tr("%1: %2")).arg(dev, state);
	}
      }
    }
  }
  //Now get the latest Snapshot/Replication information
  QStringList lpstat = LPBackend::listCurrentStatus();
  for(int i=0; i<lpstat.length(); i++){
    if(lpstat[i].section(":::",0,0) == zpool){
      QString lastSnap = lpstat[i].section(":::",1,1);
      QString lastRep = lpstat[i].section(":::",2,2);
      if(lastSnap=="-"){ DSC.latestSnapshot = QObject::tr("No Snapshots Available"); }
      else{ DSC.latestSnapshot = lastSnap; }
      if(lastRep!="-"){
        finished << QString(QObject::tr("Latest Replication: %1")).arg(lastRep);
      }else if(LPBackend::listReplicationTargets().contains(zpool) ){
	errors << QObject::tr("No Successful Replication");
      }
    }
  }
  //Now save the info to the dataset
  DSC.harddisks = disks;
  DSC.harddiskStatus = diskstates;
  DSC.errorStatus = errors.join("\n");
  DSC.runningStatus = running.join("\n");
  DSC.finishedStatus = finished.join("\n");
  //Return the dataset
  return DSC;
}

QString LPGUtils::generateReversionFileName(QString fileName, QString destDir){
  fileName = fileName.section("/",-1); //Make sure we only have the filename (no paths)
  if( !destDir.endsWith("/") ){ destDir.append("/"); }
  //Quick check if that file already exists in the destination directory
  if( !QFile::exists(destDir+fileName) ){ return fileName; }
  //Change the filename to prevent overwriting an existing file
  QString ext = fileName.section(".",-1); //Get the extension for the filename
  if( fileName != ext && fileName != ("."+ext) ){
    //Extension found - need careful adjustment of filename
    QString newFileName = fileName;
    newFileName.replace("."+ext, "-reversion."+ext);
    int i=2;
    while( QFile::exists(destDir+newFileName) ){
      //Also need to append a reversion number
      newFileName = fileName; 
      newFileName.replace("."+ext, "-reversion"+QString::number(i)+"."+ext);
      i++; //for the next time around
    }
    fileName = newFileName;
  }else{
    //no extension - just append the reversion
    fileName.append("-reversion");
    if(QFile::exists(destDir+fileName) ){
      //Also need to append a reversion number
      int i=2;
      while( QFile::exists(destDir+fileName+QString::number(i)) ){ i++; }
      fileName.append( QString::number(i) );
    }
  }
  return fileName;
}

bool LPGUtils::revertFile(QString oldPath, QString newPath){
  bool ok = QFile::copy(oldPath,newPath);
  //return the path to the new file if the copy was successful
  if(ok){ 
    //reset the permissions on the reverted file to match the original
    QFile::setPermissions(newPath, QFile::permissions(oldPath));
    QFileInfo FI(oldPath);
      system( QString("chown "+FI.owner()+":"+FI.group()+" "+newPath).toUtf8() );
  }
  return ok;
}

QStringList LPGUtils::revertDir(QString oldPath, QString newPath){
  //Note: this is a recursive function and can take quite a while to perform lots of file copies

  //Load the directories and create it if necessary
  QDir oDir(oldPath);
  QDir nDir(newPath);
  bool ok=true;
  if( !nDir.exists() ){
    //Create the new Directory
    nDir.cdUp();
    ok = nDir.mkdir(newPath.section("/",-1));
    if(ok){ 
      nDir.cd(newPath.section("/",-1)); 
      QFile::setPermissions(newPath, QFile::permissions(oldPath)); //make sure the new dir has the old permissions
      QFileInfo FI(oldPath);
      system( QString("chown "+FI.owner()+":"+FI.group()+" "+newPath).toUtf8() );
    }
  }
  //Get a list of any files that error
  QStringList errors;
  if(!ok){
    errors << newPath;
    return errors;
  }
  //Get a list of all the files in the old dir and copy them over
  QStringList fList = oDir.entryList(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDir::Name);
  for(int i=0; i<fList.length(); i++){
    if( !revertFile(oldPath+"/"+fList[i], newPath+"/"+fList[i]) ){
       errors << newPath+"/"+fList[i];
    }
  }
  //Now list all the directories in the old dir and recursively copy them over
  fList = oDir.entryList(QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot, QDir::Name);
  for(int i=0; i<fList.length(); i++){
    QStringList errs = revertDir(oldPath+"/"+fList[i], newPath+"/"+fList[i]);
    if( !errs.isEmpty() ){ errors << errs; }
  }
  return errors;
}
