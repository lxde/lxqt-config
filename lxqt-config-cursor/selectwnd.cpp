/* coded by Ketmar // Vampire Avalon (psyc://ketmar.no-ip.org/~Ketmar)
 * (c)DWTFYW
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */

// 2014-04-10 modified by Hong Jen Yee (PCMan) for integration with lxqt-config-input

#include <QDebug>

#include "selectwnd.h"

#include <QKeyEvent>
#include <QMessageBox>
#include <QTimer>
#include <QWidget>
#include <QPushButton>
#include <QToolTip>

#include "cfgfile.h"
#include "crtheme.h"
#include "thememodel.h"
#include "itemdelegate.h"

#include "xcrimg.h"
#include "xcrxcur.h"
#include "xcrtheme.h"

#include <lxqt/lxqtsettings.h>
#include <qtxdg/xdgicon.h>

#define HOME_ICON_DIR QDir::homePath() + "/.icons"

SelectWnd::SelectWnd(LxQt::Settings* settings, QWidget *parent) : QWidget(parent), mSettings(settings)
{
    setupUi(this);

    warningLabel->hide();

    mModel = new XCursorThemeModel(this);

    int size = style()->pixelMetric(QStyle::PM_LargeIconSize);
    lbThemes->setModel(mModel);
    lbThemes->setItemDelegate(new ItemDelegate(this));
    lbThemes->setIconSize(QSize(size, size));
    lbThemes->setSelectionMode(QAbstractItemView::SingleSelection);

    // Make sure we find out about selection changes
    connect(lbThemes->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
            SLOT(currentChanged(const QModelIndex &, const QModelIndex &)));
    // display/hide warning label
    connect(mModel, SIGNAL(modelReset()),
                    this, SLOT(handleWarning()));
    connect(mModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)),
                    this, SLOT(handleWarning()));
    connect(mModel, SIGNAL(rowsRemoved(const QModelIndex&, int, int)),
                    this, SLOT(handleWarning()));

    connect(warningLabel, SIGNAL(showDirInfo()),
                    this, SLOT(showDirInfo()));

    // Disable the install button if we can't install new themes to ~/.icons,
    // or Xcursor isn't set up to look for cursor themes there
    btInstall->setEnabled(mModel->searchPaths().contains(HOME_ICON_DIR) && iconsIsWritable());
    // TODO/FIXME: btInstall functionality
    btInstall->hide();
    btRemove->hide();

    //QTimer::singleShot(0, this, SLOT(setCurrent()));

    handleWarning();
}


SelectWnd::~SelectWnd()
{
}

void SelectWnd::setCurrent()
{
    lbThemes->selectionModel()->clear();

    QString ct = getCurrentTheme();
    if(ct.isEmpty())
    {
        // if we fail to get the default icon theme name, read from lxqt config
        mSettings->beginGroup("Environment");
        ct = mSettings->value("XCURSOR_THEME").toString();
        mSettings->endGroup();
    }

    mAppliedIndex = mModel->defaultIndex();

    if (!ct.isEmpty()) mAppliedIndex = mModel->findIndex(ct);
    else mAppliedIndex = mModel->defaultIndex();

    if (mAppliedIndex.isValid())
    {
        const XCursorThemeData *theme = mModel->theme(mAppliedIndex);
        // Select the current theme
        selectRow(mAppliedIndex);
        lbThemes->scrollTo(mAppliedIndex, QListView::PositionAtCenter);
        // Update the preview widget as well
        if (theme) preview->setTheme(*theme);// else preview->clearTheme();
    }
}

bool SelectWnd::iconsIsWritable() const
{
    const QFileInfo icons = QFileInfo(HOME_ICON_DIR);
    const QFileInfo home = QFileInfo(QDir::homePath());
    return ((icons.exists() && icons.isDir() && icons.isWritable()) || (!icons.exists() && home.isWritable()));
}

/*
void SelectWnd::keyPressEvent(QKeyEvent *e)
{
  if (e->key() == Qt::Key_Escape) close();
}
*/

void SelectWnd::selectRow(int row) const
{
    // Create a selection that stretches across all columns
    QModelIndex from = mModel->index(row, 0);
    QModelIndex to = mModel->index(row, mModel->columnCount()-1);
    QItemSelection selection(from, to);
    lbThemes->selectionModel()->select(selection, QItemSelectionModel::Select);
    lbThemes->selectionModel()->setCurrentIndex(mAppliedIndex, QItemSelectionModel::NoUpdate);
}

void SelectWnd::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous)
    if (current.isValid())
    {
        const XCursorThemeData *theme = mModel->theme(current);
        if (theme) preview->setTheme(*theme); else preview->clearTheme();
        btRemove->setEnabled(theme->isWritable());
        //qDebug() << theme->path() << theme->name();
        
        // 2014-04-10 added by pcman for lxqt-config-input
        // directly apply the current settings
        applyCurrent();
    }else preview->clearTheme();
   //emit changed(mAppliedIndex != current);
}

void SelectWnd::on_btInstall_clicked()
{
    qDebug() << "'install' clicked";
}

void SelectWnd::applyCurrent()
{
    //qDebug() << "'set' clicked";
    const XCursorThemeData *theme = mModel->theme(lbThemes->currentIndex());
    if (!theme) return;
    applyTheme(*theme);
    fixXDefaults(theme->name());

    // LXQT: LxQt settings - session requires restart!
    mSettings->beginGroup("Environment");
    mSettings->setValue("XCURSOR_THEME", theme->name());
    mSettings->endGroup();
}

void SelectWnd::on_btRemove_clicked()
{
    qDebug() << "'remove' clicked";
    const XCursorThemeData *theme = mModel->theme(lbThemes->currentIndex());
    if (!theme) return;
    QString ct = getCurrentTheme();
    if (ct == theme->name())
    {
        QMessageBox::warning(this, tr("XCurTheme error"),
                             tr("You can't remove active theme!"), QMessageBox::Ok, QMessageBox::Ok);
        return;
    }
    QDir d(theme->path());
    preview->clearTheme();
    mModel->removeTheme(lbThemes->currentIndex());
    removeXCursorTheme(d);
}

void SelectWnd::handleWarning()
{
        bool empty = mModel->rowCount();
        warningLabel->setVisible(!empty);
        preview->setVisible(empty);
        infoLabel->setVisible(empty);
}

void SelectWnd::showDirInfo()
{
        QToolTip::showText(mapToGlobal(warningLabel->buttonPos()), mModel->searchPaths().join("\n"));
}
