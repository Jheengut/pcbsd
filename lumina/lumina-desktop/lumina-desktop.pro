
QT       += core gui network

TARGET = Lumina-DE
target.path = /usr/local/bin

TEMPLATE = app


SOURCES += main.cpp \
	WMProcess.cpp \
	LSession.cpp \
	LDesktop.cpp \
	LPanel.cpp \
	AppMenu.cpp \
	SettingsMenu.cpp \
	SystemWindow.cpp \
	panel-plugins/userbutton/LUserButton.cpp \
	panel-plugins/desktopbar/LDeskBar.cpp \
	panel-plugins/taskmanager/LTaskManagerPlugin.cpp \
	panel-plugins/taskmanager/LTaskButton.cpp \
	panel-plugins/systemtray/LSysTray.cpp \
	panel-plugins/clock/LClock.cpp \
	desktop-plugins/desktopview/DesktopViewPlugin.cpp \
	desktop-plugins/desktopview/DeskItem.cpp



HEADERS  += Globals.h \
	WMProcess.h \
	LSession.h \
	LDesktop.h \
	LPanel.h \
	AppMenu.h \
	SettingsMenu.h \
	SystemWindow.h \
	panel-plugins/LPPlugin.h \
	panel-plugins/NewPP.h \
	panel-plugins/LTBWidget.h \
	desktop-plugins/LDPlugin.h \
	desktop-plugins/NewDP.h \
	panel-plugins/userbutton/LUserButton.h \
	panel-plugins/desktopbar/LDeskBar.h \
	panel-plugins/systemtray/LSysTray.h \
	panel-plugins/spacer/LSpacer.h \
	panel-plugins/clock/LClock.h \
	panel-plugins/taskmanager/LWinInfo.h \
	panel-plugins/taskmanager/LTaskManagerPlugin.h \
	panel-plugins/taskmanager/LTaskButton.h \
	desktop-plugins/desktopview/DesktopViewPlugin.h \
	desktop-plugins/desktopview/DeskItem.h

FORMS    += SystemWindow.ui

RESOURCES+= Lumina-DE.qrc

INCLUDEPATH += ../libLumina /usr/local/include

desktop.files = Lumina-DE.desktop
desktop.path = /usr/local/share/xsessions

icons.files = Lumina-DE.png
icons.path = /usr/local/share/pixmaps

defaultbackground.files = images/desktop-background.jpg
defaultbackground.path = /usr/local/share/Lumina-DE/

LIBS     += -L../libLumina -lLuminaUtils

TRANSLATIONS =  i18n/lumina-desktop_af.ts \
                i18n/lumina-desktop_ar.ts \
                i18n/lumina-desktop_az.ts \
                i18n/lumina-desktop_bg.ts \
                i18n/lumina-desktop_bn.ts \
                i18n/lumina-desktop_bs.ts \
                i18n/lumina-desktop_ca.ts \
                i18n/lumina-desktop_cs.ts \
                i18n/lumina-desktop_cy.ts \
                i18n/lumina-desktop_da.ts \
                i18n/lumina-desktop_de.ts \
                i18n/lumina-desktop_el.ts \
                i18n/lumina-desktop_en_GB.ts \
                i18n/lumina-desktop_en_ZA.ts \
                i18n/lumina-desktop_es.ts \
                i18n/lumina-desktop_et.ts \
                i18n/lumina-desktop_eu.ts \
                i18n/lumina-desktop_fa.ts \
                i18n/lumina-desktop_fi.ts \
                i18n/lumina-desktop_fr.ts \
                i18n/lumina-desktop_fr_CA.ts \
                i18n/lumina-desktop_gl.ts \
                i18n/lumina-desktop_he.ts \
                i18n/lumina-desktop_hi.ts \
                i18n/lumina-desktop_hr.ts \
                i18n/lumina-desktop_hu.ts \
                i18n/lumina-desktop_id.ts \
                i18n/lumina-desktop_is.ts \
                i18n/lumina-desktop_it.ts \
                i18n/lumina-desktop_ja.ts \
                i18n/lumina-desktop_ka.ts \
                i18n/lumina-desktop_ko.ts \
                i18n/lumina-desktop_lt.ts \
                i18n/lumina-desktop_lv.ts \
                i18n/lumina-desktop_mk.ts \
                i18n/lumina-desktop_mn.ts \
                i18n/lumina-desktop_ms.ts \
                i18n/lumina-desktop_mt.ts \
                i18n/lumina-desktop_nb.ts \
                i18n/lumina-desktop_nl.ts \
                i18n/lumina-desktop_pa.ts \
                i18n/lumina-desktop_pl.ts \
                i18n/lumina-desktop_pt.ts \
                i18n/lumina-desktop_pt_BR.ts \
                i18n/lumina-desktop_ro.ts \
                i18n/lumina-desktop_ru.ts \
                i18n/lumina-desktop_sk.ts \
                i18n/lumina-desktop_sl.ts \
                i18n/lumina-desktop_sr.ts \
                i18n/lumina-desktop_sv.ts \
                i18n/lumina-desktop_sw.ts \
                i18n/lumina-desktop_ta.ts \
                i18n/lumina-desktop_tg.ts \
                i18n/lumina-desktop_th.ts \
                i18n/lumina-desktop_tr.ts \
                i18n/lumina-desktop_uk.ts \
                i18n/lumina-desktop_uz.ts \
                i18n/lumina-desktop_vi.ts \
                i18n/lumina-desktop_zh_CN.ts \
                i18n/lumina-desktop_zh_HK.ts \
                i18n/lumina-desktop_zh_TW.ts \
                i18n/lumina-desktop_zu.ts

dotrans.path=/usr/local/share/Lumina-DE/i18n/
dotrans.extra=cd i18n && lrelease-qt4 -nounfinished *.ts && cp *.qm $(INSTALL_ROOT)/usr/local/share/Lumina-DE/i18n/

INSTALLS += target desktop icons defaultbackground dotrans
