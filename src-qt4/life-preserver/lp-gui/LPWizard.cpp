#include "LPWizard.h"
#include "ui_LPWizard.h"

LPWizard::LPWizard(QWidget *parent) : QWizard(parent), ui(new Ui::LPWizard){
  //Initialize the graphical items
  ui->setupUi(this);  //load the mainUI.ui file
  cancelled = true; //Make sure this is always set by default
  connect(this,SIGNAL(accepted()), this,SLOT(slotFinished()) );
  connect(this,SIGNAL(rejected()),this,SLOT(slotCancelled()) );
  connect(ui->push_scanNetwork, SIGNAL(clicked()), this, SLOT(scanNetwork()) );
}

LPWizard::~LPWizard(){
	
}

void LPWizard::setDataset(QString ds){
  ui->label_dataset->setText(ds);
}
	
void LPWizard::slotFinished(){
  qDebug() << "Wizard Finished";
  cancelled = false; //use the values from the UI
  //Now load the information from the UI
  enableReplication = ui->groupReplicate->isChecked();
  if(enableReplication){
    remotePort = ui->spinPort->value();
    int remoteTime = ui->combo_remote_freq->currentIndex();
    if(remoteTime==0){ remoteTime = -999; } //Sync
    else if(remoteTime==1){ remoteTime =  ui->time_replicate->time().hour(); } //Daily @
    else if(remoteTime==2){ remoteTime = -60; } //hourly
    else if(remoteTime==3){ remoteTime = -30; } //30min
    else{ remoteTime = -10; } //10min
    remoteHost = ui->lineHostName->text();
    remoteUser = ui->lineUserName->text();
    remoteDataset = ui->lineRemoteDataset->text();
    //Prompt for the SSH key generation
    LPBackend::setupSSHKey(remoteHost, remoteUser, remotePort);
  }
  if(ui->radio_automatic->isChecked()){ localTime = -999; }
  if(ui->radioDaily->isChecked()){ localTime = ui->timeEdit->time().hour(); }
  else if( ui->radio10Min->isChecked()){ localTime = -10; }
  else if( ui->radio30Min->isChecked()){ localTime = -30; }
  else if( ui->radio5Min->isChecked()){ localTime = -5; }
  else{ localTime = -60; } //hourly
  if( ui->radioKeepTotal->isChecked() ){
    totalSnapshots = ui->spin_keepTotal->value();
  }else{
    if(localTime >0){ totalSnapshots = ui->spin_keepDays->value(); }
    else{ totalSnapshots = ui->spin_keepDays->value() * (1440/(-localTime)); } //convert to number of snapshots a day
  }

  //Now close the UI
  this->close();
}

void LPWizard::slotCancelled(){
  qDebug() << "Wizard Cancelled";
  cancelled = true; //just to make sure
  this->close();
}

void LPWizard::scanNetwork(){
  QStringList targs = LPGUtils::scanNetworkSSH(); // <name>:::<address>:::<port>
  if(targs.isEmpty()){
    QMessageBox::warning(this,tr("No Network Targets"), tr("We could not find any systems on the local network with SSH availability (port 22)") );
    return;
  }
  //Ask the user to select a target
  QStringList targets;
  for(int i=0; i<targs.length(); i++){
    targets << targs[i].section(":::",0,0);
  }
  bool ok;
  QString target = QInputDialog::getItem(this, tr("Select Replication Target"), tr("Hostname:"), targets, 0, false, &ok);
  if(!ok || target.isEmpty() ){ return; } //cancelled
  //Now look for that target in the list of info
  for(int i=0; i<targs.length(); i++){
    if(targs[i].startsWith(target+":::")){
      ui->lineHostName->setText(targs[i].section(":::",1,1));
      ui->spinPort->setValue( targs[i].section(":::",2,2).toInt() );
      break;
    }
  }
}

void LPWizard::on_combo_remote_freq_itemChanged(int index){
  //Adjust the visibility of the replication time selector
  ui->time_replicate->setVisible( (index == 1) );
}

int LPWizard::nextId() const{
	int cpg = currentId();
	if(cpg == 1 && ui->radio_automatic->isChecked() ){
	  //Automatic snapshot schedule - skip the snapshot pruning page (not needed)
	  return 3;
	}else if(cpg==4){ //last page
	  return -1;
	}else{
	  return (cpg+1); //go to the next page
	}
}