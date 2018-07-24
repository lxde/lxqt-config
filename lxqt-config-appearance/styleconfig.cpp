/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org/
 *
 * Copyright: 2014 LXQt team
 * Authors:
 *   Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "styleconfig.h"
#include "ui_styleconfig.h"
#include <QTreeWidget>
#include <QDebug>
#include <QStyleFactory>
#include <QToolBar>
#include <QSettings>
#include <QMetaObject>
#include <QMetaProperty>
#include <QMetaEnum>
#include <QToolBar>

#ifdef Q_WS_X11
extern void qt_x11_apply_settings_in_all_apps();
#endif

StyleConfig::StyleConfig(LXQt::Settings* settings, QSettings* qtSettings, LXQt::Settings *configAppearanceSettings, ConfigOtherToolKits *configOtherToolKits, QWidget* parent) :
    QWidget(parent),
    ui(new Ui::StyleConfig),
    mQtSettings(qtSettings),
    mSettings(settings)
{
    mConfigAppearanceSettings = configAppearanceSettings;
    mConfigOtherToolKits = configOtherToolKits;
    ui->setupUi(this);

    initControls();
    
    connect(ui->gtk2ComboBox, SIGNAL(activated(const QString &)), this, SLOT(gtk2StyleSelected(const QString &)));
    connect(ui->gtk3ComboBox, SIGNAL(activated(const QString &)), this, SLOT(gtk3StyleSelected(const QString &)));
    connect(ui->qtComboBox, SIGNAL(activated(const QString &)), this, SLOT(qtStyleSelected(const QString &)));
    
    connect(ui->advancedOptionsGroupBox, SIGNAL(toggled(bool)), this, SLOT(showAdvancedOptions(bool)));
    
    connect(ui->toolButtonStyle, SIGNAL(currentIndexChanged(int)), SLOT(toolButtonStyleSelected(int)));
    connect(ui->singleClickActivate, SIGNAL(toggled(bool)), SLOT(singleClickActivateToggled(bool)));
}


StyleConfig::~StyleConfig()
{
    delete ui;
}


void StyleConfig::initControls()
{

    // Fill global themes
    QStringList qtThemes = QStyleFactory::keys();
    QStringList gtk2Themes = mConfigOtherToolKits->getGTKThemes("2.0");
    QStringList gtk3Themes = mConfigOtherToolKits->getGTKThemes("3.0");
    
    if(!mConfigAppearanceSettings->contains("ControlGTKThemeEnabled"))
        mConfigAppearanceSettings->setValue("ControlGTKThemeEnabled", false);
    bool controlGTKThemeEnabled = mConfigAppearanceSettings->value("ControlGTKThemeEnabled").toBool();
    
    showAdvancedOptions(controlGTKThemeEnabled);
    ui->advancedOptionsGroupBox->setChecked(controlGTKThemeEnabled);

    // read other widget related settings from LXQt settings.
    QByteArray tb_style = mSettings->value("tool_button_style").toByteArray();
    // convert toolbar style name to value
    QMetaEnum me = QToolBar::staticMetaObject.property(QToolBar::staticMetaObject.indexOfProperty("toolButtonStyle")).enumerator();
    int val = me.keyToValue(tb_style.constData());
    if(val == -1)
      val = Qt::ToolButtonTextBesideIcon;
    ui->toolButtonStyle->setCurrentIndex(val);

    // activate item views with single click
    ui->singleClickActivate->setChecked( mSettings->value("single_click_activate", false).toBool());
    
    
    // Fill Qt themes
    ui->qtComboBox->addItems(qtThemes);
    
    // Fill GTK themes
    ui->gtk2ComboBox->addItems(gtk2Themes);
    ui->gtk3ComboBox->addItems(gtk3Themes);
    
    ui->gtk2ComboBox->setCurrentText(mConfigOtherToolKits->getGTKThemeFromRCFile("2.0"));
    ui->gtk3ComboBox->setCurrentText(mConfigOtherToolKits->getGTKThemeFromRCFile("3.0"));
    mSettings->beginGroup(QLatin1String("Qt"));
    ui->qtComboBox->setCurrentText(mSettings->value("style").toString());
    mSettings->endGroup();

    update();
}

void StyleConfig::toolButtonStyleSelected(int index)
{
    // convert style value to string
    QMetaEnum me = QToolBar::staticMetaObject.property(QToolBar::staticMetaObject.indexOfProperty("toolButtonStyle")).enumerator();
    if(index == -1)
        index = Qt::ToolButtonTextBesideIcon;
    const char* str = me.valueToKey(index);
    if(str)
    {
        mSettings->setValue("tool_button_style", str);
        mSettings->sync();
        emit updateSettings();
    }
}

void StyleConfig::singleClickActivateToggled(bool toggled)
{
    mSettings->setValue("single_click_activate", toggled);
    mSettings->sync();
}

void StyleConfig::qtStyleSelected(const QString &themeName)
{
    mQtSettings->beginGroup(QLatin1String("Qt"));
    mQtSettings->setValue("style", themeName);
    mQtSettings->endGroup();
    mQtSettings->sync();
}

void StyleConfig::gtk3StyleSelected(const QString &themeName)
{
    mConfigOtherToolKits->setGTKConfig("3.0", themeName);
    mConfigOtherToolKits->setXSettingsConfig();
}

void StyleConfig::gtk2StyleSelected(const QString &themeName)
{
    mConfigOtherToolKits->setGTKConfig("2.0", themeName);
    mConfigOtherToolKits->setXSettingsConfig();
}

void StyleConfig::showAdvancedOptions(bool on)
{
    ui->uniformThemeLabel->setVisible(on);
    mConfigAppearanceSettings->setValue("ControlGTKThemeEnabled", on);
}
