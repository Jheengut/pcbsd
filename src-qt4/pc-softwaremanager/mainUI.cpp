/***************************************************************************
 *   Copyright (C) 2011 - iXsystems				 	  *
 *   kris@pcbsd.org  *
 *   tim@pcbsd.org   *
 *   ken@pcbsd.org   *
 *                                                                         *
 *   Permission is hereby granted, free of charge, to any person obtaining *
 *   a copy of this software and associated documentation files (the       *
 *   "Software"), to deal in the Software without restriction, including   *
 *   without limitation the rights to use, copy, modify, merge, publish,   *
 *   distribute, sublicense, and/or sell copies of the Software, and to    *
 *   permit persons to whom the Software is furnished to do so, subject to *
 *   the following conditions:                                             *
 *                                                                         *
 *   The above copyright notice and this permission notice shall be        *
 *   included in all copies or substantial portions of the Software.       *
 *                                                                         *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    *
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. *
 *   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR     *
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, *
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR *
 *   OTHER DEALINGS IN THE SOFTWARE.                                       *
 ***************************************************************************/
#include "mainUI.h"
#include "ui_mainUI.h"

MainUI::MainUI(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainUI){
  //initialization function
  ui->setupUi(this); //load the Qt-Designer file
  defaultIcon = ":/application.png";
  statusLabel = new QLabel();
  ui->statusbar->addWidget(statusLabel);
}

void MainUI::setWardenMode(QString dir, QString ip){
  wardenDir = dir; wardenIP = ip;
}

void MainUI::ProgramInit()
{ 
   qDebug("Application starting...");
   //Initial Checks
     bool wardenMode = !wardenDir.isEmpty();
     bool ok = Extras::checkUser(wardenMode);
     if(!ok && wardenMode){  //Not root
       QMessageBox::information( this, tr("Error!"), tr("The AppCafe must be run as root when in Warden compatibility mode") );
       close();	     
     }else if( !ok && !wardenMode){
       QMessageBox::information( this, tr("Error!"), tr("The AppCafe must be started with user permissions!")+"\n"+tr("The user must also be a part of the \"operator\" group") );
       close();
     }
   
   //Initialize the backend worker class
   //qDebug() << "Initialize Backend";
   PBI = new PBIBackend();
     if(wardenMode){ PBI->setWardenMode(wardenDir, wardenIP); }
     
   //Initialize the Installed tab
   //qDebug() << "Initialize Installed Tab";
   initializeInstalledTab();
   //Initialize the PBI Browser
   //qDebug() << "Initialize Browser Tab";
   initializeBrowserTab();
   
   //Now startup the backend
   //qDebug() << "Startup Backend";
     connect(PBI,SIGNAL(LocalPBIChanges()),this,SLOT(slotRefreshInstallTab()) );
     connect(PBI,SIGNAL(PBIStatusChange(QString)),this,SLOT(slotPBIStatusUpdate(QString)) );
     connect(PBI,SIGNAL(RepositoryInfoReady()),this,SLOT(slotEnableBrowser()) );
     connect(PBI,SIGNAL(NoRepoAvailable()),this,SLOT(slotDisableBrowser()) );
     connect(PBI,SIGNAL(SearchComplete(QStringList,QStringList)),this,SLOT(slotShowSearchResults(QStringList, QStringList)) );
     connect(PBI,SIGNAL(SimilarFound(QStringList)),this,SLOT(slotShowSimilarApps(QStringList)) );
     connect(PBI,SIGNAL(Error(QString,QString,QStringList)),this,SLOT(slotDisplayError(QString,QString,QStringList)) );
     PBI->start();

   //Make sure we start on the installed tab
   ui->tabWidget->setCurrentWidget(ui->tab_installed);
   ui->stackedWidget->setCurrentWidget(ui->page_install_list);
}

void MainUI::slotSingleInstance(){
  this->raise();
  this->showNormal();
  this->activateWindow();
}

void MainUI::closeEvent(QCloseEvent *event){
  bool safe = PBI->safeToQuit();
  if(!safe){
    //Verify that they want to continue
    QMessageBox::StandardButton button = QMessageBox::warning(this, tr("AppCafe Processes Running"), tr("The AppCafe currently has actions pending. Do you want to cancel all running processes and quit anyway?"), QMessageBox::Yes | QMessageBox::Cancel,QMessageBox::Cancel);
    if(button == QMessageBox::Yes){ //close down
      PBI->cancelActions( PBI->installedList() ); //close down safely
    }else{
      event->ignore();
      return;
    }
  }
  this->close();
}
// ========================
// ===== MENU OPTIONS =====
// ========================
void MainUI::on_actionImport_PBI_List_triggered(){
  QString file = QFileDialog::getOpenFileName( this, tr("Import PBI File List"), QDir::homePath(), tr("PBI List (*.pbilist)"));
  if(file.isEmpty()){ return; } //action cancelled
  bool ok = PBI->importPbiListFromFile(file);
  if(!ok){ qDebug() << QMessageBox::warning(this,tr("Import Error"),tr("There was an error importing the PBI list")+"\n"+tr("Please make sure that the file has not been corrupted and try again")); }
}

void MainUI::on_actionExport_PBI_List_triggered(){
  QString file = QFileDialog::getSaveFileName( this, tr("Export PBI File List"), QDir::homePath()+"/exportfile.pbilist", tr("PBI List (*.pbilist)"));
  if(file.isEmpty()){ return; } //action cancelled
  bool ok = PBI->exportPbiListToFile(file);
  if(!ok){ qDebug() << QMessageBox::warning(this,tr("Export Error"),tr("There was an error exporting the PBI list")+"\n"+tr("Please make sure that you have the proper directory permissions and try again")); }
}

void MainUI::on_actionQuit_triggered(){
  this->close();
}

void MainUI::on_actionAppCafe_Settings_triggered(){
  PBI->openConfigurationDialog();
}

void MainUI::on_actionInstall_From_File_triggered(){
  QStringList files = QFileDialog::getOpenFileNames(this, tr("Install PBI"), QDir::homePath(), tr("PBI Application (*.pbi)") );
  if(files.isEmpty()){ return; } //cancelled
  //Verify that they want to install these applications
  QStringList names;
  for(int i=0; i<files.length(); i++){ names << files[i].section("/",-1); }
  names.sort();
  if( QMessageBox::Yes == QMessageBox::question(this, tr("Verify Installation"), tr("Are you ready to begin installing these PBI's?")+"\n"+tr("NOTE: You will need to manually add desktop/menu icons through the AppCafe afterwards.")+"\n\n"+names.join("\n"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) ){
    //This might take a while, so don't allow the user to run this action again until it is done (thread safe though)
    ui->actionInstall_From_File->setEnabled(false);
    PBI->installPBIFromFile(files);
    ui->actionInstall_From_File->setEnabled(true);
  }
}

// =========================
// ===== INSTALLED TAB =====
// =========================
void MainUI::initializeInstalledTab(){
	
  //Setup the action menu for installed applications
  actionMenu = new QMenu();
    actionMenu->addAction( QIcon(":icons/view-refresh.png"), tr("Update"), this, SLOT(slotActionUpdate()) );
    actionMenu->addSeparator();
    QMenu *dmenu = actionMenu->addMenu( QIcon(":icons/xdg_desktop.png"), tr("Desktop Icons"));
      dmenu->addAction( QIcon(":icons/add.png"),tr("Add"),this,SLOT(slotActionAddDesktop()) );
      dmenu->addAction( QIcon(":icons/remove.png"),tr("Remove"),this,SLOT(slotActionRemoveDesktop()) );
    QMenu *mmenu = actionMenu->addMenu( QIcon(":icons/xdg_menu.png"), tr("Menu Icons"));
      mmenu->addAction( QIcon(":icons/add.png"),tr("Add"),this,SLOT(slotActionAddMenu()) );
      mmenu->addAction( QIcon(":icons/remove.png"),tr("Remove"),this,SLOT(slotActionRemoveMenu()) );  
      mmenu->addAction( QIcon(":icons/add-root.png"),tr("Add (All Users)"),this,SLOT(slotActionAddMenuAll()) );
    QMenu *pmenu = actionMenu->addMenu( QIcon(":icons/xdg_paths.png"), tr("Path Links"));
      pmenu->addAction( QIcon(":icons/add.png"),tr("Add"),this,SLOT(slotActionAddPath()) );
      pmenu->addAction( QIcon(":icons/remove.png"),tr("Remove"),this,SLOT(slotActionRemovePath()) );  
      pmenu->addAction( QIcon(":icons/add-root.png"),tr("Add (All Users)"),this,SLOT(slotActionAddPathAll()) );
    QMenu *fmenu = actionMenu->addMenu( QIcon(":icons/xdg_mime.png"), tr("File Associations"));
      fmenu->addAction( QIcon(":icons/add.png"),tr("Add"),this,SLOT(slotActionAddMime()) );
      fmenu->addAction( QIcon(":icons/remove.png"),tr("Remove"),this,SLOT(slotActionRemoveMime()) );  
      fmenu->addAction( QIcon(":icons/add-root.png"),tr("Add (All Users)"),this,SLOT(slotActionAddMimeAll()) );
    actionMenu->addSeparator();
    actionMenu->addAction( QIcon(":icons/remove.png"), tr("Uninstall"), this, SLOT(slotActionRemove()) );
    actionMenu->addSeparator();
    actionMenu->addAction( QIcon(":icons/dialog-cancel.png"), tr("Cancel Actions"), this, SLOT(slotActionCancel()) );
  //Setup the shortcuts menu for installed applications
  shortcutMenu = new QMenu(this);
    sDeskMenu = shortcutMenu->addMenu( QIcon(":icons/xdg_desktop.png"), tr("Desktop Icons"));
      sDeskMenu->addAction( QIcon(":icons/add.png"),tr("Add"),this,SLOT(slotActionAddDesktop()) );
      sDeskMenu->addAction( QIcon(":icons/remove.png"),tr("Remove"),this,SLOT(slotActionRemoveDesktop()) );
    sMenuMenu = shortcutMenu->addMenu( QIcon(":icons/xdg_menu.png"), tr("Menu Icons"));
      sMenuMenu->addAction( QIcon(":icons/add.png"),tr("Add"),this,SLOT(slotActionAddMenu()) );
      sMenuMenu->addAction( QIcon(":icons/remove.png"),tr("Remove"),this,SLOT(slotActionRemoveMenu()) );  
      sMenuMenu->addAction( QIcon(":icons/add-root.png"),tr("Add (All Users)"),this,SLOT(slotActionAddMenuAll()) );
    QMenu *spmenu = shortcutMenu->addMenu( QIcon(":icons/xdg_paths.png"), tr("Path Links"));
      spmenu->addAction( QIcon(":icons/add.png"),tr("Add"),this,SLOT(slotActionAddPath()) );
      spmenu->addAction( QIcon(":icons/remove.png"),tr("Remove"),this,SLOT(slotActionRemovePath()) );  
      spmenu->addAction( QIcon(":icons/add-root.png"),tr("Add (All Users)"),this,SLOT(slotActionAddPathAll()) );
    sMimeMenu = shortcutMenu->addMenu( QIcon(":icons/xdg_mime.png"), tr("File Associations"));
      sMimeMenu ->addAction( QIcon(":icons/add.png"),tr("Add"),this,SLOT(slotActionAddMime()) );
      sMimeMenu ->addAction( QIcon(":icons/remove.png"),tr("Remove"),this,SLOT(slotActionRemoveMime()) );  
      sMimeMenu ->addAction( QIcon(":icons/add-root.png"),tr("Add (All Users)"),this,SLOT(slotActionAddMimeAll()) );
  ui->tool_install_shortcuts->setMenu(shortcutMenu);
  //Setup the binary menu for installed applications
  appBinMenu = new QMenu();
  ui->tool_install_startApp->setMenu(appBinMenu);
    connect(appBinMenu, SIGNAL(triggered(QAction*)), this, SLOT(slotStartApp(QAction*)) );
  //Initialize the context menu
  contextActionMenu = new QMenu(this);
    connect(contextActionMenu, SIGNAL(aboutToHide()), this, SLOT(contextMenuFinished()) );
  //Now setup the action button
  ui->tool_install_performaction->setMenu(actionMenu);
  ui->tool_install_performaction->setPopupMode(QToolButton::InstantPopup);
  //Now setup any defaults for the installed tab
  ui->tree_install_apps->setIconSize(QSize(22,22));
  ui->tree_install_apps->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui->tree_install_apps, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(slotCheckSelectedItems()) );
  connect(ui->tree_install_apps, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT( slotInstalledAppRightClicked(const QPoint &)) );
  slotRefreshInstallTab();
}

void MainUI::formatInstalledItemDisplay(QTreeWidgetItem *item){
  //simplification function for filling the tree widget item with the appropriate information about the PBI
  QString ID = item->whatsThis(0);
  if(item->text(0).isEmpty()){  //new entry - get everything
    QStringList vals; vals <<"name"<<"version"<<"status"; //same as column order
    if(!vals.isEmpty()){
      vals = PBI->PBIInfo(ID, vals);
      for(int i=0; i<vals.length(); i++){ item->setText(i,vals[i]); }
      QString icon = PBI->PBIInfo(ID, QStringList("icon")).join("");
        //Load a default icon if none found
      if(icon.isEmpty()){ icon = defaultIcon; }
      item->setIcon(0,QIcon(icon) );
      item->setCheckState(0,Qt::Unchecked);
    }
  }else{ // Just update the necesary info
    QStringList vals; vals << "version" << "status";
    vals = PBI->PBIInfo(ID,vals);
    if(!vals.isEmpty()){ item->setText(1,vals[0]); item->setText(2,vals[1]); }
  }
}

QStringList MainUI::getCheckedItems(){
  //Return the pbiID's of all the active items
  QStringList output;
  //See if we are on the single-app details page or custom context menu- then get the current app only
  if(ui->stackedWidget->currentWidget() == ui->page_install_details){
      output << cDetails;  
	  
  //Check for whether this is the context menu on the main widget
  }else if(!cDetails.isEmpty()){
     output << cDetails;
     cDetails.clear();
	  
  //If on the main Installed page, look for checked items only
  }else{
    for(int i=0; i<ui->tree_install_apps->topLevelItemCount(); i++){
      if(ui->tree_install_apps->topLevelItem(i)->checkState(0) == Qt::Checked){
        output << ui->tree_install_apps->topLevelItem(i)->whatsThis(0);
      }
    }
  }
  //qDebug() << "Checked Items:" << output;
  return output;	
}

// === SLOTS ===
void MainUI::slotRefreshInstallTab(){
  //Update the list of installed PBI's w/o clearing the list (loses selections)
   //Get the list we need (in order)
  QStringList installList = PBI->installedList();
  installList.sort(); //sort in ascending order
   //Get the list we have now
  QStringList cList;
  for(int i=0; i<ui->tree_install_apps->topLevelItemCount(); i++){
    cList << ui->tree_install_apps->topLevelItem(i)->whatsThis(0);
  }
  //Quick finish if no items installed
  if(installList.isEmpty()){
    ui->tree_install_apps->clear();
  }else{
    //Now make adjustments as necessary
    for(int i=0; i<installList.length(); i++){
      //Detemine what action should be done with this item location
      int todo = 0; //0=insert new item, 1=update current item, 2=remove current item
      if(i < cList.length()){
        if(installList[i] == cList[i]){ todo=1; }
        else if( installList.contains(cList[i]) && !cList.contains(installList[i]) ){ todo=0; } //new item to be inserted here
        else if( !installList.contains(cList[i]) ){ todo=2; } //current item in this location should be removed
      }
      //Now perform the action on this location
      if(todo==0){ 
        //insert new item
        QTreeWidgetItem *item = new QTreeWidgetItem; //create the item
        item->setWhatsThis(0,installList[i]);
        //Now format the display
        formatInstalledItemDisplay(item);
	if(item->text(0).isEmpty()){
	  //Do not put empty items into the display
	  delete item;
	}else{
          //Now insert this item onto the list
          ui->tree_install_apps->insertTopLevelItem(i,item);
          cList.insert(i,installList[i]); //reflect this inclusion into the current list
	}
	
      }else if(todo==1){
        //Update current item
        formatInstalledItemDisplay( ui->tree_install_apps->topLevelItem(i) );
	
      }else{
        //Remove current item
        ui->tree_install_apps->takeTopLevelItem(i);
        cList.removeAt(i); //reflect the change to the current list
        i--; //Re-check the item that should be in this location
      }
    }
    //Now makesure that there are no extra items at the end
    int il = installList.length();
    while(il < cList.length()){
      ui->tree_install_apps->takeTopLevelItem(il);
      cList.removeAt(il); //reflect the change to the current list 
    }
  } //end of empty list check

  //Make sure that there is an item selected
  if(ui->tree_install_apps->topLevelItemCount() > 0 ){
    if( ui->tree_install_apps->selectedItems().isEmpty() ){
      ui->tree_install_apps->setCurrentItem( ui->tree_install_apps->topLevelItem(0) );
    }
    //Now re-size the columns to the minimum required width
    for(int i=0; i<3; i++){
      ui->tree_install_apps->resizeColumnToContents(i);
    } 
  }
  //If the installed app page is visible, be sure to update it too
  if( ui->stackedWidget->currentWidget() == ui->page_install_details){
    updateInstallDetails(cDetails);
  }
  slotUpdateSelectedPBI();; //Update the info boxes
  slotDisplayStats();
  slotCheckSelectedItems();
  //If the browser app page is currently visible for this app
  if( (ui->stacked_browser->currentWidget() == ui->page_app) && ui->page_app->isVisible() ){
    slotGoToApp(cApp);
  }
}

void MainUI::slotCheckSelectedItems(){
  bool chkd = false;
  for(int i=0; i<ui->tree_install_apps->topLevelItemCount(); i++){
    if(ui->tree_install_apps->topLevelItem(i)->checkState(0) == Qt::Checked){
      chkd = true; break;
    }
  }
  ui->tool_install_performaction->setEnabled(chkd);
  if(ui->stackedWidget->currentWidget() != ui->page_install_details){
    cDetails.clear(); //Make sure this is cleared if not on the details page
  }
}

void MainUI::slotPBIStatusUpdate(QString pbiID){
  //This will do a full update of a particlar PBI entry
  //	and just update/check the icons for all the other PBI's
  for(int i=0; i<ui->tree_install_apps->topLevelItemCount(); i++){
    QString itemID = ui->tree_install_apps->topLevelItem(i)->whatsThis(0);
    if(itemID == pbiID){
      QString stat = PBI->PBIInfo(pbiID,QStringList()<<"status").join("");
      ui->tree_install_apps->topLevelItem(i)->setText(2,stat);
      // See if we need to update the download progress bar
      QString appID = ui->tree_install_apps->currentItem()->whatsThis(0);
      if ( appID == pbiID ) {
	slotUpdateSelectedPBI();
	//If the details page is currently visible, update it too
	if(ui->stackedWidget->currentWidget() == ui->page_install_details){
	  updateInstallDetails(appID);
	}
      }
    }else{
      //Just check/update the icon if necessary	
      if(ui->tree_install_apps->topLevelItem(i)->icon(0).isNull()){
	 ui->tree_install_apps->topLevelItem(i)->setIcon(0, QIcon( PBI->PBIInfo(itemID, QStringList() << "icon").join("") ));
      }
    }
  }
 
  //If the browser app page is current for this app
  QString metaID = PBI->pbiToAppID(pbiID);
  if( (ui->stacked_browser->currentWidget() == ui->page_app) && (cApp == metaID) && ui->page_app->isVisible() ){
    slotUpdateAppDownloadButton();
  }
}

void MainUI::on_tool_install_details_clicked(){
  //Get the current item
  QString appID;
  if(ui->tree_install_apps->topLevelItemCount() > 0){
    appID = ui->tree_install_apps->currentItem()->whatsThis(0);
  }
  if(appID.isEmpty()){return;}
  //Update the info on the details page
  updateInstallDetails(appID);
  //Now show the page
  ui->stackedWidget->setCurrentWidget(ui->page_install_details);
}

void MainUI::on_tool_install_back_clicked(){
  //List page should always be current based upon backend
  ui->stackedWidget->setCurrentWidget(ui->page_install_list);
}

void MainUI::on_tool_install_gotobrowserpage_clicked(){
  //When you want to open up the browser page for an application
  QString appID = Extras::nameToID( ui->tree_install_apps->currentItem()->text(0) );
  slotGoToApp(appID);
}

void MainUI::on_tool_install_toggleall_clicked(){
  //Determine if the current item is checked or unchecked
  bool checkall = (ui->tree_install_apps->currentItem()->checkState(0) == Qt::Unchecked);
  for(int i=0; i<ui->tree_install_apps->topLevelItemCount(); i++){
    if(checkall){ ui->tree_install_apps->topLevelItem(i)->setCheckState(0,Qt::Checked); }
    else{ui->tree_install_apps->topLevelItem(i)->setCheckState(0,Qt::Unchecked); }
  }
  slotCheckSelectedItems();
}

void MainUI::on_tree_install_apps_itemSelectionChanged(){
  //When an installed PBI is clicked on
  slotUpdateSelectedPBI();
  if(ui->stackedWidget->currentWidget() != ui->page_install_list){
    ui->stackedWidget->setCurrentWidget(ui->page_install_list);
  }
  slotCheckSelectedItems();
}

void MainUI::on_tree_install_apps_itemDoubleClicked(QTreeWidgetItem *item){
 //Make sure it is a valid/installed application
 QString appID = item->whatsThis(0);
  appID = PBI->isInstalled(appID);
  if(appID.isEmpty()){ return; } //invalid item
  //Update the info on the details page
  updateInstallDetails(appID);
  //Now show the page
  ui->stackedWidget->setCurrentWidget(ui->page_install_details);
}

void MainUI::on_check_install_autoupdate_clicked(){
  //Get the current item
  QString appID;
  if(ui->tree_install_apps->topLevelItemCount() > 0){
    appID = ui->tree_install_apps->currentItem()->whatsThis(0);
  }
  if(appID.isEmpty()){return;}
  //Now determine the current state of the checkbox
  bool enabled = ui->check_install_autoupdate->isChecked();
  //Now have the backend make the change
  PBI->enableAutoUpdate(appID, enabled);
  //Now ask if the user also wants to start updating it now
  if(enabled && !PBI->upgradeAvailable(appID).isEmpty()){
    if( QMessageBox::Yes == QMessageBox::question(this, tr("Start Update?"), tr("Do you wish to start updating this application right now?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) ){
      PBI->upgradePBI(QStringList() << appID);
    }
  }
  //Now force the info on the page to be updated in a moment (need time for database to update)
  //QTimer::singleShot(500, this, SLOT(on_tool_install_details_clicked()) );
}

void MainUI::on_tool_install_update_clicked(){
  //Get the current item
  QString appID;
  if(ui->tree_install_apps->topLevelItemCount() > 0){
    appID = ui->tree_install_apps->currentItem()->whatsThis(0);
  }
  if(appID.isEmpty()){return;}
  PBI->upgradePBI(QStringList() << appID);
}

void MainUI::on_tool_install_remove_clicked(){
  //Get the current item
  QString appID;
  if(ui->tree_install_apps->topLevelItemCount() > 0){
    appID = ui->tree_install_apps->currentItem()->whatsThis(0);
  }
  if(appID.isEmpty()){return;}
  //Verify removal
  if( QMessageBox::Yes == QMessageBox::question(this,tr("Verify PBI Removal"), tr("Are you sure you wish to remove this application?")+"\n\n"+appID,QMessageBox::Yes | QMessageBox::Cancel,QMessageBox::Cancel) ){
    PBI->removePBI(QStringList() << appID);
  }
}

void MainUI::on_tool_install_cancel_clicked(){
  //Get the current item
  QString appID;
  if(ui->tree_install_apps->topLevelItemCount() > 0){
    appID = ui->tree_install_apps->currentItem()->whatsThis(0);
  }
  if(appID.isEmpty()){return;}
  PBI->cancelActions(QStringList() << appID);
  
}

void MainUI::on_tool_install_maintainer_clicked(){
  //Get the current item
  QString appID;
  if(ui->tree_install_apps->topLevelItemCount() > 0){
    appID = ui->tree_install_apps->currentItem()->whatsThis(0);
  }
  if(appID.isEmpty()){return;}
  //Get the maintainer email
  QString email = PBI->PBIInfo(appID, QStringList() << "maintainer").join("");
  if(email.isEmpty()){ return; }
  //Verify that the user wants to launch their email client
  if(QMessageBox::Yes != QMessageBox::question(this, tr("Launch Email Client?"), tr("Do you want to try launching your default email client? \n You must have this setup within your current desktop environment for this to work properly. If not, you can send an email to the address below manually.")+"\n\n"+email, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) ){
    return;
  }
  qDebug() << "Launching email to:" << email;
  //Get the command from the action
  QString cmd = "mailto:"+email;
  QStringList info = PBI->PBIInfo(appID, QStringList() << "name" << "date" << "version" << "arch" << "fbsdversion" );
  //Add a sample subject
  cmd.append("?subject="+info[0]+" port question");
  //Add the info to the body of the email
  cmd.append("&body=");
  cmd.append("-----------\nPBI Information:\nName: "+info[0] + "\nDate Created: "+info[1] +"\nVersion: "+info[2] +"\nArchitecture: "+info[3] +"\nFreeBSD Version: "+info[4] );
  //Startup the command externally
  QProcess::execute("xdg-open \""+cmd+"\"");
}

void MainUI::slotInstalledAppRightClicked(const QPoint &pt){
  //Get the item under the mouse click
  QTreeWidgetItem *it = ui->tree_install_apps->itemAt(pt);
  if(it==0){ return; } // no item selected
  QString pbiID = it->whatsThis(0);
  qDebug() << "Get context menu for:" << pbiID;
  //Now Update the context menu appropriately
  QStringList info = PBI->PBIInfo(pbiID, QStringList() << "hasdesktopicons" << "hasmenuicons" << "hasmimetypes");
  if(info.isEmpty()){ return; } //invalid application
  bool pending = PBI->isWorking(pbiID);
  contextActionMenu->clear();
  if(!PBI->upgradeAvailable(pbiID).isEmpty() &&  !pending){
    //Upgrade is only available if actions not pending
    contextActionMenu->addAction( QIcon(":icons/view-refresh.png"), tr("Update"), this, SLOT(slotActionUpdate()) );
    contextActionMenu->addSeparator();
  }
  if(info[0]=="true"){
    QMenu *dmenu = contextActionMenu->addMenu( QIcon(":icons/xdg_desktop.png"), tr("Desktop Icons"));
      dmenu->addAction( QIcon(":icons/add.png"),tr("Add"),this,SLOT(slotActionAddDesktop()) );
      dmenu->addAction( QIcon(":icons/remove.png"),tr("Remove"),this,SLOT(slotActionRemoveDesktop()) );
  }
  if(info[1]=="true"){
    QMenu *mmenu = contextActionMenu->addMenu( QIcon(":icons/xdg_menu.png"), tr("Menu Icons"));
      mmenu->addAction( QIcon(":icons/add.png"),tr("Add"),this,SLOT(slotActionAddMenu()) );
      mmenu->addAction( QIcon(":icons/remove.png"),tr("Remove"),this,SLOT(slotActionRemoveMenu()) );  
      mmenu->addAction( QIcon(":icons/add-root.png"),tr("Add (All Users)"),this,SLOT(slotActionAddMenuAll()) );
  }
  //Paths are always available if actually installed
  if( !PBI->isInstalled(pbiID).isEmpty() ){
    QMenu *pmenu = contextActionMenu->addMenu( QIcon(":icons/xdg_paths.png"), tr("Path Links"));
      pmenu->addAction( QIcon(":icons/add.png"),tr("Add"),this,SLOT(slotActionAddPath()) );
      pmenu->addAction( QIcon(":icons/remove.png"),tr("Remove"),this,SLOT(slotActionRemovePath()) );  
      pmenu->addAction( QIcon(":icons/add-root.png"),tr("Add (All Users)"),this,SLOT(slotActionAddPathAll()) );
  }
  if(info[2]=="true"){
    QMenu *fmenu = contextActionMenu->addMenu( QIcon(":icons/xdg_mime.png"), tr("File Associations"));
      fmenu->addAction( QIcon(":icons/add.png"),tr("Add"),this,SLOT(slotActionAddMime()) );
      fmenu->addAction( QIcon(":icons/remove.png"),tr("Remove"),this,SLOT(slotActionRemoveMime()) );  
      fmenu->addAction( QIcon(":icons/add-root.png"),tr("Add (All Users)"),this,SLOT(slotActionAddMimeAll()) );
  }
  if(!pending){
    //Remove option is only available if not currently pending actions
    contextActionMenu->addSeparator();
    contextActionMenu->addAction( QIcon(":icons/remove.png"), tr("Uninstall"), this, SLOT(slotActionRemove()) );
  }
  if(pending){
    //Cancel option is only available if actions are currently pending	  
    contextActionMenu->addSeparator();
    contextActionMenu->addAction( QIcon(":icons/dialog-cancel.png"), tr("Cancel Actions"), this, SLOT(slotActionCancel()) );
  }
  //Now show the menu
  cDetails = pbiID; //save this so we know which app is currently being modified
  contextActionMenu->popup(ui->tree_install_apps->mapToGlobal(pt));
}

void MainUI::contextMenuFinished(){
  QTimer::singleShot(500, this, SLOT(slotCheckSelectedItems()) );	
}

// === SELECTED PBI ACTIONS ===
void MainUI::slotActionAddDesktop(){
  QStringList items = getCheckedItems();
  PBI->addDesktopIcons(items,FALSE); //only for current user
}

void MainUI::slotActionRemoveDesktop(){
  QStringList items = getCheckedItems();
  PBI->rmDesktopIcons(items,FALSE);  //Only for current user
}

void MainUI::slotActionAddPath(){
  QStringList items = getCheckedItems();
  PBI->addPaths(items,FALSE);  //Only for current user	
}

void MainUI::slotActionRemovePath(){
  QStringList items = getCheckedItems();
  PBI->rmPaths(items,FALSE);  //Only for current user
}

void MainUI::slotActionAddPathAll(){
  QStringList items = getCheckedItems();
  PBI->addPaths(items,TRUE);  //For all users (root permissions required)
}

void MainUI::slotActionAddMenu(){
  QStringList items = getCheckedItems();
  PBI->addMenuIcons(items,FALSE);  //Only for current user
}

void MainUI::slotActionRemoveMenu(){
  QStringList items = getCheckedItems();
  PBI->rmMenuIcons(items,FALSE);  //Only for current user	
}

void MainUI::slotActionAddMenuAll(){
  QStringList items = getCheckedItems();
  PBI->addMenuIcons(items,TRUE);  //For all users (root permissions required)	
}

void MainUI::slotActionAddMime(){
  QStringList items = getCheckedItems();
  PBI->addMimeTypes(items,FALSE);  //Only for current user	
}

void MainUI::slotActionRemoveMime(){
  QStringList items = getCheckedItems();
  PBI->rmMimeTypes(items,FALSE);  //Only for current user	
}

void MainUI::slotActionAddMimeAll(){
    QStringList items = getCheckedItems();
  PBI->addMimeTypes(items,TRUE);  //For all users (root permissions required)	
}

void MainUI::slotActionUpdate(){
  QStringList checkedID = getCheckedItems();
  if(!checkedID.isEmpty()){
    PBI->upgradePBI(checkedID);  
  }
}

void MainUI::slotActionRemove(){
  QStringList checkedID = getCheckedItems();
  if(!checkedID.isEmpty()){
    //Verify that the user really wants to remove these apps
    if( QMessageBox::Yes == QMessageBox::question(this,tr("Verify PBI Removal"), tr("Are you sure you wish to remove these applications?")+"\n\n"+checkedID.join("\n"),QMessageBox::Yes | QMessageBox::Cancel,QMessageBox::Cancel) ){
      PBI->removePBI(checkedID);
    }
  }
}

void MainUI::slotActionCancel(){
  QStringList checkedID = getCheckedItems();
  if(!checkedID.isEmpty()){
    PBI->cancelActions(checkedID);  
  }
}

void MainUI::slotStartApp(QAction* act){
  qDebug() << "Starting external application:" << act->text();
  //Get the command from the action
  QString desktopfile = act->whatsThis();
  QString cmd = "xdg-open "+desktopfile;
  //Startup the command externally
  QProcess::startDetached(cmd);
}

void MainUI::slotUpdateSelectedPBI(){
  //Get the currently selected app
  QString appID;
  if(ui->tree_install_apps->topLevelItemCount() > 0){
    appID = ui->tree_install_apps->currentItem()->whatsThis(0);
  }
  //See if this one is actually installed
  appID = PBI->isInstalled(appID);
  if(appID.isEmpty()){
    ui->tool_install_details->setEnabled(false);
  }else{
    ui->tool_install_details->setEnabled(true);	  
  }
    
}

void MainUI::updateInstallDetails(QString appID){
  //Get the information to update the details page
  //Get the PBI info for that item
    cDetails = appID; //save for later
    QStringList vals; 
    vals << "name" << "icon" << "author" << "website" << "version" << "license" << "description" << "maintainer" << "date" << "arch";
    QStringList bools;
    bools << "autoupdate" << "hasdesktopicons" << "hasmenuicons" << "requiresroot" << "hasmimetypes";
    vals = PBI->PBIInfo(appID,vals);
    bools = PBI->PBIInfo(appID,bools);
    //Make sure the info lists are not empty
    if(vals.isEmpty() || bools.isEmpty()){
      ui->tool_install_details->setEnabled(false);
      return; 
    }
    //Load a default icon if none found
    if(vals[1].isEmpty()){ vals[1] = defaultIcon; }
    //Now set the info on the UI
    bool desktopSC = (bools[1] == "true"); // XDG desktop entries available
    bool menuSC= (bools[2] == "true"); 	 // XDG menu entries available
    bool autoupdate = (bools[0] == "true"); //auto-update enabled
    bool rootonly = (bools[3] == "true"); //requires root
    bool mimeSC = (bools[4] == "true"); //XDG mime entries available
    //Create the shortcuts string
    sDeskMenu->setEnabled(desktopSC);
    sMenuMenu->setEnabled(menuSC);
    sMimeMenu->setEnabled(mimeSC);
    //Now display that info on the UI
    ui->label_install_app->setText(vals[0]);
    ui->label_install_icon->setPixmap( QPixmap(vals[1]).scaled(64,64, Qt::KeepAspectRatio, Qt::SmoothTransformation) );
    if(vals[3].isEmpty()){ 
      ui->label_install_author->setText(vals[2]); 
      ui->label_install_author->setToolTip("");
    }else{ 
      ui->label_install_author->setText("<a href="+vals[3]+">"+vals[2]+"</a>"); 
      ui->label_install_author->setToolTip(vals[3]); //show website URL as tooltip
    }
    ui->label_install_version->setText(vals[4]);
    ui->label_install_license->setText(vals[5]);
    ui->text_install_description->setPlainText(vals[6]);
    ui->tool_install_maintainer->setVisible( vals[7].contains("@") );
    ui->label_install_date->setText(vals[8]);
    ui->label_install_arch->setText(vals[9]);
    //ui->label_install_shortcuts->setText(shortcuts);
    ui->check_install_autoupdate->setChecked(autoupdate);
  
    //Adjust the quick action buttons as necessary
    if( PBI->isWorking(appID) ){
      //Actions pending/working only show cancel button
      ui->tool_install_cancel->setVisible(TRUE);
      ui->tool_install_remove->setVisible(FALSE);
      ui->tool_install_update->setVisible(FALSE);
      ui->tool_install_startApp->setVisible(FALSE);
    }else{
      //Nothing pending
      ui->tool_install_cancel->setVisible(FALSE);
      if( !PBI->isInstalled(appID).isEmpty() ){ 
        //Remove Button
        ui->tool_install_remove->setVisible(TRUE);
        if(rootonly){ ui->tool_install_remove->setIcon(QIcon(":icons/remove-root.png")); }
        else{ ui->tool_install_remove->setIcon(QIcon(":icons/remove.png")); }
        //Update
        if(PBI->upgradeAvailable(appID).isEmpty()){ ui->tool_install_update->setVisible(FALSE); }
        else{
          ui->tool_install_update->setVisible(TRUE); 
          if(rootonly){ ui->tool_install_update->setIcon(QIcon(":icons/app_upgrade_small-root.png")); }
          else{ ui->tool_install_update->setIcon(QIcon(":icons/app_upgrade_small.png")); }
        }
	//Start Application binaries
	QStringList bins = PBI->pbiBinList(appID);
        appBinMenu->clear();
        for(int i=0; i<bins.length(); i++){
          QAction *act = new QAction(this);
	    act->setText(bins[i].section("::::",0,0)); //set name
	    act->setWhatsThis(bins[i].section("::::",1,10)); //set command string
          appBinMenu->addAction(act);
        }
	if(appBinMenu->isEmpty()){ ui->tool_install_startApp->setVisible(FALSE); }
	else{ ui->tool_install_startApp->setVisible(TRUE); }
      }else{ 
        ui->tool_install_remove->setVisible(FALSE); 
        ui->tool_install_update->setVisible(FALSE); 
	ui->tool_install_startApp->setVisible(FALSE);
      }   
    }

  //Update the current status indicators
  QString stat = PBI->currentAppStatus(appID,true); //get the raw status
  QString statF = PBI->currentAppStatus(appID, false); //get the non-raw status
  if( stat.isEmpty() && statF.isEmpty() ){
    //Not currently running - hide the display indicators
    ui->group_install_appStat->setVisible(false);
  }else{
    //Currently installing/removing/updating - show last message from process
    if(!statF.isEmpty()){ ui->label_install_status->setText(statF); }
    else{ ui->label_install_status->setText(stat); }
    ui->group_install_appStat->setVisible(TRUE);
  }
}

// ==========================
// ====== BROWSER TAB =======
// ==========================
void MainUI::initializeBrowserTab(){
  ui->tab_browse->setEnabled(FALSE);
  //Always make sure that the browser starts off on the "home" page
  ui->stacked_browser->setCurrentWidget(ui->page_home);
  //With invisible shortcut buttons
  ui->tool_browse_cat->setVisible(FALSE);
  ui->tool_browse_app->setVisible(FALSE);
  //Clear any items left over from the designer form
  clearScrollArea(ui->scroll_br_home_newapps);
  clearScrollArea(ui->scroll_br_home_rec);
  //Search functionality
  searchTimer = new QTimer();
    searchTimer->setSingleShot(TRUE);
    searchTimer->setInterval(300); // 0.3 sec wait before a search
    connect(searchTimer,SIGNAL(timeout()),this,SLOT(slotGoToSearch()) );
  connect(ui->tool_browse_search,SIGNAL(clicked()),this,SLOT(slotGoToSearch()) );
  connect(ui->line_browse_searchbar,SIGNAL(returnPressed()),this,SLOT(slotGoToSearch()) );
  connect(ui->tool_browse_gotocat, SIGNAL(clicked()), this, SLOT(slotGoToCatBrowser()) );
}

// === SLOTS ===
void MainUI::slotDisableBrowser(bool shownotification){
  if(shownotification){ qDebug() << "No Repo Available: De-activating the Browser"; }
  ui->tabWidget->setCurrentWidget(ui->tab_installed);
  ui->tab_browse->setEnabled(FALSE);
  slotDisplayStats();
}

void MainUI::slotEnableBrowser(){
  qDebug() << "Repo Ready: - generating browser home page";
  //Now create the browser home page
  slotUpdateBrowserHome();
  //And allow the user to go there
  ui->tab_browse->setEnabled(TRUE);
  slotDisplayStats();
}

void MainUI::slotUpdateBrowserHome(){
  //Load the Recommendations
  clearScrollArea(ui->scroll_br_home_rec);
  QVBoxLayout *reclayout = new QVBoxLayout;
  QStringList recList = PBI->getRecommendations();
  QStringList info; info << "name" << "shortdescription" << "icon" << "type";
  for(int i=0; i<recList.length(); i++){
    QStringList data = PBI->AppInfo(recList[i],info);
    if(!data.isEmpty()){
      LargeItemWidget *item = new LargeItemWidget(recList[i],data[0],data[1],data[2]);
      //Set the type icon
      item->setType(data[3].toLower());
      connect(item,SIGNAL(appClicked(QString)),this,SLOT(slotGoToApp(QString)) );
      reclayout->addWidget(item);
    }
  }
  reclayout->addStretch(); //add a spacer to the end
  ui->scroll_br_home_rec->widget()->setLayout(reclayout);
  //Load the newest applications
  clearScrollArea(ui->scroll_br_home_newapps);
  QHBoxLayout *newapplayout = new QHBoxLayout;
  QStringList newapps = PBI->getRecentApps();
  for(int i=0; i<newapps.length(); i++){
    QStringList appdata = PBI->AppInfo(newapps[i],QStringList() << "name" << "icon" << "latestversion");
    if(!appdata.isEmpty()){
      SmallItemWidget *item = new SmallItemWidget(newapps[i],appdata[0],appdata[1],appdata[2]);
      connect(item,SIGNAL(appClicked(QString)),this,SLOT(slotGoToApp(QString)) );
      newapplayout->addWidget(item);
    }
  }
  newapplayout->addStretch(); //add a spacer to the end
  newapplayout->setContentsMargins(0,0,0,0);
  newapplayout->setSpacing(0);
  ui->scroll_br_home_newapps->widget()->setLayout(newapplayout);
  //Make sure that the newapps scrollarea is the proper fit vertically (no vertical scrolling)
  ui->scroll_br_home_newapps->setMinimumHeight(ui->scroll_br_home_newapps->widget()->minimumSizeHint().height());
  
  //Make sure the new apps area is invisible if no items available
  if(newapps.isEmpty()){ ui->group_br_home_newapps->setVisible(FALSE); }
  else{ ui->group_br_home_newapps->setVisible(TRUE); }
  //make sure the home page is visible in the browser (slotGoToHome without changing tabs)
  ui->stacked_browser->setCurrentWidget(ui->page_home);	
  //Make sure the shortcut buttons are disabled
  ui->tool_browse_cat->setVisible(FALSE);
  ui->tool_browse_app->setVisible(FALSE); 
  
  //Now update the category browser page (since it only needs to be done once like the home menu)
  //Load the Categories
  QStringList cats = PBI->browserCategories();
    cats.sort();
    clearScrollArea(ui->scroll_br_cats);
    QVBoxLayout *catlayout = new QVBoxLayout;
    info.clear(); info << "name" << "description" << "icon";
    for(int i=0; i<cats.length(); i++){
      QStringList data = PBI->CatInfo(cats[i],info);
      if(!data.isEmpty()){
        LargeItemWidget *item = new LargeItemWidget(cats[i],data[0],data[1],data[2]);
        connect(item,SIGNAL(appClicked(QString)),this,SLOT(slotGoToCategory(QString)) );
        catlayout->addWidget(item);
      }
    }
    catlayout->addStretch(); //add a spacer to the end
    ui->scroll_br_cats->widget()->setLayout(catlayout);
}

void MainUI::slotGoToHome(){
  ui->tabWidget->setCurrentWidget(ui->tab_browse);
  ui->stacked_browser->setCurrentWidget(ui->page_home);	
  //Make sure the shortcut buttons are disabled
  ui->tool_browse_cat->setVisible(false);
  ui->tool_browse_app->setVisible(false);
  ui->tool_browse_gotocat->setVisible(true);
}

void MainUI::slotGoToCatBrowser(){
  ui->tabWidget->setCurrentWidget(ui->tab_browse);
  ui->stacked_browser->setCurrentWidget(ui->page_browsecats);
  //Make sure the shortcut buttons are diabled
  ui->tool_browse_cat->setVisible(false);
  ui->tool_browse_app->setVisible(false);
  ui->tool_browse_gotocat->setVisible(false);
}

void MainUI::slotGoToCategory(QString cat){
  qDebug() << "Show Category:" << cat;
  //Get the apps in this category
  QStringList apps = PBI->browserApps(cat);
  if(apps.isEmpty()){
    qDebug() << "Invalid category:" << cat;
    return;
  }
  clearScrollArea(ui->scroll_br_cat_apps);
  apps.sort();
  QVBoxLayout *applayout = new QVBoxLayout;
  QStringList info; info << "name" << "shortdescription" << "icon" << "type";
  for(int i=0; i<apps.length(); i++){
    QStringList data = PBI->AppInfo(apps[i],info);
    if(!data.isEmpty()){
      //Trim the description if necessary
      if(data[1].size()>100){ data[1] = data[1].left(100).append("..."); }
      LargeItemWidget *item = new LargeItemWidget(apps[i],data[0],data[1],data[2]);
      item->setType(data[3].toLower());
      connect(item,SIGNAL(appClicked(QString)),this,SLOT(slotGoToApp(QString)) );
      applayout->addWidget(item); 
    }
  }
  applayout->addStretch();
  ui->scroll_br_cat_apps->widget()->setLayout(applayout);
  //Now enable/disable the shortcut buttons
  ui->tool_browse_app->setVisible(false);
  ui->tool_browse_cat->setVisible(false);
  ui->tool_browse_gotocat->setVisible(true);
  //Setup the icon/name for the category display
    QStringList catinfo = PBI->CatInfo(cat,QStringList() << "name" << "icon");
    ui->tool_browse_cat->setText(catinfo[0]);
    ui->label_cat_name->setText(catinfo[0]);
    if(catinfo[1].isEmpty()){ catinfo[1] = defaultIcon; }
    ui->tool_browse_cat->setIcon(QIcon(catinfo[1]));
    ui->label_cat_icon->setPixmap( QPixmap(catinfo[1]).scaled(32,32) ); 
  //Now move to the page
  ui->tabWidget->setCurrentWidget(ui->tab_browse);
  ui->stacked_browser->setCurrentWidget(ui->page_cat);
  //Now save that this category is currently displayed
  cCat = cat;
}

void MainUI::slotGoToApp(QString appID){
  qDebug() << "Show App:" << appID;
  //Get the general application info
  QStringList data; 
  data << "name" << "icon" << "author" << "website" << "license" << "type" << "description" \
       << "category" << "requiresroot" << "latestversion" << "latestarch" <<"latestsize" \
       << "backupversion" << "backuparch" << "backupsize";
  data = PBI->AppInfo(appID,data);
  if(data.isEmpty()){
    qDebug() << "Invalid App:" << appID;
    return;
  }
  cApp = appID; //save this for later
  //Start the search for similar apps
  PBI->searchSimilar = appID;
  ui->group_bapp_similar->setVisible(FALSE);
  QTimer::singleShot(0,PBI,SLOT(startSimilarSearch()));
  //Now Check icon
  if(data[1].isEmpty()){ data[1] = defaultIcon; }
  //Now fill the UI with the data
  ui->label_bapp_name->setText(data[0]);
  ui->label_bapp_icon->setPixmap(QPixmap(data[1]));
  if(data[3].isEmpty()){ ui->label_bapp_authorweb->setText(data[2]); }
  else{ ui->label_bapp_authorweb->setText("<a href="+data[3]+">"+data[2]+"</a>"); }
  ui->label_bapp_authorweb->setToolTip(data[3]);
  ui->label_bapp_license->setText(data[4]);
  ui->label_bapp_type->setText(data[5]);
  ui->text_bapp_description->setText(data[6]);
  //Now determine the appropriate version info
  QString pbiID = PBI->isInstalled(appID); //get pbiID
  QString cVer;
  if(!pbiID.isEmpty()){ cVer = PBI->PBIInfo(pbiID,QStringList("version")).join(""); }
  bool useLatest=FALSE;
  bool nobackup = data[12].isEmpty();
  if(cVer.isEmpty()){ useLatest=TRUE; } //not currently installed
  else if(cVer != data[9]){ useLatest=TRUE;} //not the latest version
  //Now put the proper version info on the UI
  if(useLatest || nobackup){
    ui->label_bapp_version->setText(data[9]);
    ui->label_bapp_arch->setText(data[10]);
    if(data[11].isEmpty()){ ui->label_bapp_size->setText(tr("Unknown")); }
    else{ ui->label_bapp_size->setText( Extras::sizeKToDisplay(data[11]) ); }
  }else{
    ui->label_bapp_version->setText(data[12]);
    ui->label_bapp_arch->setText(data[13]);
    if(data[14].isEmpty()){ ui->label_bapp_size->setText(tr("Unknown")); }
    else{ ui->label_bapp_size->setText( Extras::sizeKToDisplay(data[14]) ); }
  }
  //Now update the download button appropriately
  slotUpdateAppDownloadButton();

  //Now enable/disable the shortcut buttons
  ui->tool_browse_app->setVisible(TRUE);
    ui->tool_browse_app->setText(data[0]);
    ui->tool_browse_app->setIcon(QIcon(data[1]));
  QStringList catinfo = PBI->CatInfo(Extras::nameToID(data[7]),QStringList() << "name" << "icon");
  if(!catinfo.isEmpty()){
    ui->tool_browse_gotocat->setVisible(false);
    ui->tool_browse_cat->setVisible(TRUE);
    ui->tool_browse_cat->setText(catinfo[0]);
    if(catinfo[1].isEmpty()){ catinfo[1] = defaultIcon; }
    ui->tool_browse_cat->setIcon(QIcon(catinfo[1]));
  }
  ui->tabWidget->setCurrentWidget(ui->tab_browse);
  ui->stacked_browser->setCurrentWidget(ui->page_app);
	
}

void MainUI::slotUpdateAppDownloadButton(){
  QString ico;
  QString working = PBI->currentAppStatus(cApp);
  QString rawstat = PBI->currentAppStatus(cApp, true);
  QStringList info = PBI->AppInfo(cApp, QStringList() << "latestversion" << "backupversion" << "requiresroot");
  QString pbiID = PBI->isInstalled(cApp);
  //qDebug() << "App Download status:" << working << rawstat;
  if(!working.isEmpty() && !rawstat.isEmpty() ){ //app currently pending or actually doing something
    if(rawstat.startsWith("DLSTAT::")){ ui->tool_bapp_download->setText(tr("Downloading..")); }
    else{ ui->tool_bapp_download->setText(working); }
    ui->tool_bapp_download->setIcon(QIcon(":icons/working.png"));
    ui->tool_bapp_download->setEnabled(FALSE);
  }else if( pbiID.isEmpty() ){ //new installation
    ui->tool_bapp_download->setText(tr("Install Now!"));
    ico = ":icons/app_download.png";
    ui->tool_bapp_download->setEnabled(TRUE);
  }else if( !PBI->upgradeAvailable(pbiID).isEmpty() ){ //Update available
    ui->tool_bapp_download->setText(tr("Update"));
    ico = ":icons/app_upgrade.png";
    ui->tool_bapp_download->setEnabled(TRUE);
  }else if(!info[1].isEmpty()){  //Downgrade available
    ui->tool_bapp_download->setText(tr("Downgrade"));
    ico = ":icons/app_downgrade.png";
    ui->tool_bapp_download->setEnabled(TRUE);
  }else{ //already installed (no downgrade available)
    ui->tool_bapp_download->setText(tr("Installed"));
    ui->tool_bapp_download->setIcon(QIcon(":icons/dialog-ok.png"));
    ui->tool_bapp_download->setEnabled(FALSE);
  }
  //Now set the icon appropriately if it requires root permissions
  if(!ico.isEmpty()){
    if(info[2]=="true"){ //requires root permissions to install
      ico.replace(".png","-root.png");
    }
    ui->tool_bapp_download->setIcon(QIcon(ico));
  }
  ui->tool_bapp_download->setWhatsThis(cApp); //set for slot
}

void MainUI::slotGoToSearch(){
  searchTimer->stop(); //just in case "return" was pressed to start the search
  QString search = ui->line_browse_searchbar->text();
  if(search.isEmpty()){ return; }
  PBI->searchTerm = search;
  QTimer::singleShot(1,PBI,SLOT(startAppSearch()));
  ui->label_bsearch_info->setText( tr("Searching the application database. Please Wait....") );
    ui->label_bsearch_info->setVisible(TRUE);
    ui->group_bsearch_best->setVisible(FALSE);
    ui->group_bsearch_other->setVisible(FALSE);
  
}
	
void MainUI::slotShowSimilarApps(QStringList apps){
  qDebug() << " - Similar applications:" << apps;
  if(apps.isEmpty()){ ui->group_bapp_similar->setVisible(FALSE); }
  else{
    clearScrollArea(ui->scroll_bapp_similar);
    QHBoxLayout *layout = new QHBoxLayout;
    for(int i=0; i<apps.length(); i++){
      QStringList appdata = PBI->AppInfo(apps[i],QStringList() << "name" << "icon");
      if(!appdata.isEmpty()){
        SmallItemWidget *item = new SmallItemWidget(apps[i],appdata[0],appdata[1],"");
        connect(item,SIGNAL(appClicked(QString)),this,SLOT(slotGoToApp(QString)) );
        layout->addWidget(item);
      }
    }
    layout->addStretch(); //add a spacer to the end
    layout->setContentsMargins(1,1,1,1);
    ui->scroll_bapp_similar->widget()->setLayout(layout);
    //Make sure that the similar scrollarea is the proper fit vertically (no vertical scrolling)
    ui->scroll_bapp_similar->setMinimumHeight(ui->scroll_bapp_similar->widget()->minimumSizeHint().height() +ui->scroll_bapp_similar->horizontalScrollBar()->height()/1.2);
    //Now make the group visible as appropriate
    ui->group_bapp_similar->setVisible(TRUE);
    if(ui->group_bapp_similar->isChecked()){ ui->scroll_bapp_similar->setVisible(TRUE); }
    else{ ui->scroll_bapp_similar->setVisible(FALSE); }
    //Now make sure the app page has the proper layout dimensions
    ui->page_app->updateGeometry();
  }
}

void MainUI::slotShowSearchResults(QStringList best, QStringList rest){
  //Now display the search results
  if(best.isEmpty()){
    ui->label_bsearch_info->setText( QString(tr("No Search Results Found for the term: %1")).arg(ui->line_browse_searchbar->text()) );
    ui->label_bsearch_info->setVisible(TRUE);
    ui->group_bsearch_best->setVisible(FALSE);
    ui->group_bsearch_other->setVisible(FALSE);
  }else{
    ui->label_bsearch_info->setVisible(FALSE);
    //Now fill the "best" section
    clearScrollArea(ui->scroll_bsearch_best);
    QVBoxLayout *layout = new QVBoxLayout;
    QStringList info; info << "name" << "shortdescription" << "icon" << "type";
    for(int i=0; i<best.length(); i++){
      QStringList data = PBI->AppInfo(best[i],info);
      if(!data.isEmpty()){
        //Trim the description if necessary
        if(data[1].size()>100){ data[1] = data[1].left(100).append("..."); }
        LargeItemWidget *item = new LargeItemWidget(best[i],data[0],data[1],data[2]);
	item->setType(data[3].toLower());
        connect(item,SIGNAL(appClicked(QString)),this,SLOT(slotGoToApp(QString)) );
        layout->addWidget(item); 
      }
    }
    layout->addStretch();
    ui->scroll_bsearch_best->widget()->setLayout(layout);
    ui->group_bsearch_best->setVisible(TRUE);
    
    //Now fill the other results
    if(rest.isEmpty()){ ui->group_bsearch_other->setVisible(FALSE); }
    else{
      ui->group_bsearch_other->setVisible(TRUE);
      clearScrollArea(ui->scroll_bsearch_other);
      QVBoxLayout *layout2 = new QVBoxLayout;
      for(int i=0; i<rest.length(); i++){
        QStringList data = PBI->AppInfo(rest[i],info);
        if(!data.isEmpty()){
          //Trim the description if necessary
          if(data[1].size()>100){ data[1] = data[1].left(100).append("..."); }
          LargeItemWidget *item = new LargeItemWidget(rest[i],data[0],data[1],data[2]);
	  item->setType(data[3].toLower());
          connect(item,SIGNAL(appClicked(QString)),this,SLOT(slotGoToApp(QString)) );
          layout2->addWidget(item); 
        }
      }
      layout2->addStretch();
      ui->scroll_bsearch_other->widget()->setLayout(layout2);
    }
  }
	
  //need to make sure the search bar still has keyboard focus (just in case)
  ui->tabWidget->setCurrentWidget(ui->tab_browse);
  ui->stacked_browser->setCurrentWidget(ui->page_search);
  
}

void MainUI::on_tabWidget_currentChanged(){
  if(ui->tabWidget->currentWidget() == ui->tab_browse){
    //Refresh the app page if that is the one currently showing
    if(ui->stacked_browser->currentWidget() == ui->page_app){ on_tool_browse_app_clicked(); }	  
  }else{
    //Always return to the installed list
    ui->stackedWidget->setCurrentWidget(ui->page_install_list);
  }
}

void MainUI::on_tool_browse_home_clicked(){
  slotGoToHome();
}

void MainUI::on_tool_browse_cat_clicked(){
  QString cat = Extras::nameToID(ui->tool_browse_cat->text());
  if(cCat == cat){ //already loaded - just show it (prevents resetting scroll positions)
    ui->stacked_browser->setCurrentWidget(ui->page_cat);
    //Now enable/disable the shortcut buttons
    ui->tool_browse_app->setVisible(false);
    ui->tool_browse_cat->setVisible(false);
    ui->tool_browse_gotocat->setVisible(true);
  }else{ //load and show the category
    slotGoToCategory(cat);
  }
}

void MainUI::on_tool_browse_app_clicked(){
  QString appID = Extras::nameToID(ui->tool_browse_app->text());
  slotGoToApp(appID);
}

void MainUI::on_line_browse_searchbar_textChanged(){
  //Connect this to a singleshot timer, so the search functionality is only
  //  run once after a short wait rather than every time a new character is typed
  
  //Live search only after 3 characters have been typed
  if(ui->line_browse_searchbar->text().length() > 2){
    searchTimer->start();
  }else{
    searchTimer->stop();	  
  }
}

void MainUI::on_tool_bapp_download_clicked(){
  QString appID = ui->tool_bapp_download->whatsThis();
  //Verify the app installation
  if( QMessageBox::Yes != QMessageBox::question(this,tr("Verify Installation"), tr("Are you sure you want to install this application?")+"\n\n"+appID,QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)){
    return;
  }
  PBI->installApp(QStringList() << appID);
  ui->tool_bapp_download->setEnabled(FALSE); //make sure it cannot be clicked more than once before page refresh
  //Now show the Installed tab
  ui->tabWidget->setCurrentWidget(ui->tab_installed);
}

void MainUI::on_group_br_home_newapps_toggled(bool show){
  ui->scroll_br_home_newapps->setVisible(show);
}

void MainUI::on_group_bapp_similar_toggled(bool show){
  ui->scroll_bapp_similar->setVisible(show);
}

// ================================
// ==========  OTHER ==============
// ================================
void MainUI::clearScrollArea(QScrollArea* area){
  QWidget *wgt = area->takeWidget();
  delete wgt; //delete the widget and all children
  area->setWidget( new QWidget() ); //create a new widget in the scroll area
  area->widget()->setContentsMargins(0,0,0,0);
}

void MainUI::slotDisplayError(QString title,QString message,QStringList log){
  QMessageBox *dlg = new QMessageBox(this);
    dlg->setWindowTitle(title);
    dlg->setText(message);
    dlg->setDetailedText(log.join("\n"));
  dlg->show();
}

void MainUI::slotDisplayStats(){
  int avail = PBI->numAvailable;
  int installed = PBI->numInstalled;
  QString text;
  if(avail == -1){
    text = QString(tr("Installed: %1")).arg(QString::number(installed));  
  }else{
    text = QString(tr("Installed: %1")+"\t"+tr("Available: %2")).arg(QString::number(installed), QString::number(avail));  
  }
  //Get the number of installed/available applications and display it 
  statusLabel->setText(text);	
}
