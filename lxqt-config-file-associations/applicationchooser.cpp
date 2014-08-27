/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * http://razor-qt.org
 *
 * Copyright: 2013 Christian Surlykke
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

#include <QDialogButtonBox>
#include <QPushButton>
#include <QSettings>
#include <QString>
#include <QDebug>
#include <QMimeDatabase>

#include <XdgDesktopFile>
#include "applicationchooser.h"

Q_DECLARE_METATYPE(XdgDesktopFile*)

ApplicationChooser::ApplicationChooser(const XdgMimeType& mimeInfo, bool showUseAlwaysCheckBox)
{
    m_MimeInfo = mimeInfo;
    m_CurrentDefaultApplication = XdgDesktopFileCache::getDefaultApp(m_MimeInfo.name());
    widget.setupUi(this);

    widget.mimetypeIconLabel->setPixmap(m_MimeInfo.icon().pixmap(widget.mimetypeIconLabel->size()));
    widget.mimetypeLabel->setText(m_MimeInfo.comment());
    widget.alwaysUseCheckBox->setVisible(showUseAlwaysCheckBox);
    widget.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

ApplicationChooser::~ApplicationChooser()
{
}

int ApplicationChooser::exec()
{
    show();
    fillApplicationListWidget();

    return QDialog::exec();
}



bool lessThan(XdgDesktopFile* a, XdgDesktopFile* b)
{
    return a && b && a->name().toLower() < b->name().toLower();
}

void ApplicationChooser::fillApplicationListWidget()
{
    widget.applicationTreeWidget->clear();

    QSet<XdgDesktopFile*> addedApps;
    QList<XdgDesktopFile*> applicationsThatHandleThisMimetype = XdgDesktopFileCache::getApps(m_MimeInfo.name());
    QList<XdgDesktopFile*> otherApplications;

    QStringList mimetypes;
    mimetypes << m_MimeInfo.name() << m_MimeInfo.allAncestors();

    QMimeDatabase db;
    foreach(const QString& mts, mimetypes) {
        QMimeType mt = db.mimeTypeForName(mts);
        QString heading;
        heading = mt.name() == QLatin1String("application/octet-stream") ?
            tr("Other applications") :
            tr("Applications that handle %1").arg(mt.comment());

        QList<XdgDesktopFile*> applications;
        applications = mt.name() == QLatin1String("application/octet-stream") ?
            XdgDesktopFileCache::getAllFiles() :
            XdgDesktopFileCache::getApps(mt.name());

        qSort(applications.begin(), applications.end(), lessThan);

        QTreeWidgetItem* headingItem = new QTreeWidgetItem(widget.applicationTreeWidget);
        headingItem->setExpanded(true);
        headingItem->setFlags(Qt::ItemIsEnabled);
        headingItem->setText(0, heading);
        headingItem->setSizeHint(0, QSize(0, 25));

        addApplicationsToApplicationListWidget(headingItem, applications, addedApps);
    }
    connect(widget.applicationTreeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(selectionChanged()));
    widget.applicationTreeWidget->setFocus();
}

void ApplicationChooser::addApplicationsToApplicationListWidget(QTreeWidgetItem* parent,
                                                                QList<XdgDesktopFile*> applications,
                                                                QSet<XdgDesktopFile*>& alreadyAdded)
{
        if (applications.isEmpty())
        {
            QTreeWidgetItem* noAppsFoundItem = new QTreeWidgetItem(parent);
            noAppsFoundItem->setText(0, tr("No applications found"));
            noAppsFoundItem->setFlags(0);
            QFont font = noAppsFoundItem->font(0);
            font.setStyle(QFont::StyleItalic);
            noAppsFoundItem->setFont(0, font);
        }
        else
        {
            // Insert applications in the listwidget, skipping already added applications
            foreach (XdgDesktopFile* desktopFile, applications)
            {
                if (alreadyAdded.contains(desktopFile))
                    continue;

                // Only applications
                if (desktopFile->type() != XdgDesktopFile::ApplicationType)
                    continue;

                QTreeWidgetItem *item = new QTreeWidgetItem(parent);
                item->setIcon(0, desktopFile->icon());
                item->setText(0, desktopFile->name());
                item->setData(0, 32, QVariant::fromValue<XdgDesktopFile*>(desktopFile));

                if (desktopFile == m_CurrentDefaultApplication)
                {
                    widget.applicationTreeWidget->setCurrentItem(item);
                }

                alreadyAdded.insert(desktopFile);
                QCoreApplication::processEvents();
            }
        }
}

void ApplicationChooser::selectionChanged()
{
    widget.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    QTreeWidgetItem* newItem = widget.applicationTreeWidget->currentItem();
    if (newItem && newItem->data(0, 32).value<XdgDesktopFile*>())
    {
        widget.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        m_CurrentDefaultApplication = newItem->data(0, 32).value<XdgDesktopFile*>();
    }
}
