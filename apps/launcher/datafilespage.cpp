#include <QtGui>

#include <QDebug> // TODO: Remove

#include <components/esm/esm_reader.hpp>
#include <components/files/path.hpp>
#include <components/files/collections.hpp>
#include <components/files/multidircollection.hpp>

#include "datafilespage.hpp"
#include "lineedit.hpp"
#include "naturalsort.hpp"

using namespace ESM;
using namespace std;

DataFilesPage::DataFilesPage(QWidget *parent) : QWidget(parent)
{
    mDataFilesModel = new QStandardItemModel(); // Contains all plugins with masters
    mPluginsModel = new QStandardItemModel(); // Contains selectable plugins

    mPluginsProxyModel = new QSortFilterProxyModel();
    mPluginsProxyModel->setDynamicSortFilter(true);
    mPluginsProxyModel->setSourceModel(mPluginsModel);

    QLabel *filterLabel = new QLabel(tr("Filter:"), this);
    LineEdit *filterLineEdit = new LineEdit(this);

    QHBoxLayout *topLayout = new QHBoxLayout();
    QSpacerItem *hSpacer1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    topLayout->addItem(hSpacer1);
    topLayout->addWidget(filterLabel);
    topLayout->addWidget(filterLineEdit);

    mMastersWidget = new QTableWidget(this); // Contains the available masters
    mPluginsTable = new QTableView(this);

    mMastersWidget->setObjectName("MastersWidget");
    mMastersWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    mMastersWidget->setSelectionMode(QAbstractItemView::MultiSelection);
    mMastersWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mMastersWidget->setAlternatingRowColors(true);
    mMastersWidget->horizontalHeader()->setStretchLastSection(true);
    mMastersWidget->horizontalHeader()->hide();
    mMastersWidget->verticalHeader()->hide();
    mMastersWidget->insertColumn(0);

    mPluginsTable->setModel(mPluginsProxyModel);
    mPluginsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mPluginsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mPluginsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mPluginsTable->setAlternatingRowColors(true);
    mPluginsTable->horizontalHeader()->setStretchLastSection(true);
    mPluginsTable->horizontalHeader()->hide();

    // Set the row height to the size of the checkboxes
    QCheckBox checkBox;
    unsigned int height = checkBox.sizeHint().height() + 2;

    mPluginsTable->verticalHeader()->setDefaultSectionSize(height);

    mPluginsTable->setDragEnabled(true);
    mPluginsTable->setDragDropMode(QAbstractItemView::InternalMove);
    mPluginsTable->setDropIndicatorShown(true);
    mPluginsTable->setDragDropOverwriteMode(false);
    mPluginsTable->viewport()->setAcceptDrops(true);

    mPluginsTable->setContextMenuPolicy(Qt::CustomContextMenu);

    // Add both tables to a splitter
    QSplitter *splitter = new QSplitter(this);
    splitter->setOrientation(Qt::Horizontal);

    splitter->addWidget(mMastersWidget);
    splitter->addWidget(mPluginsTable);

    // Adjust the default widget widths inside the splitter
    QList<int> sizeList;
    sizeList << 100 << 300;
    splitter->setSizes(sizeList);

    // Bottom part with profile options
    QLabel *profileLabel = new QLabel(tr("Current Profile:"), this);

    mProfilesComboBox = new ComboBox(this);
    mProfilesComboBox->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
    mProfilesComboBox->setInsertPolicy(QComboBox::InsertAtBottom);

    /*mNewProfileButton = new QPushButton(this);
    mNewProfileButton->setIcon(QIcon::fromTheme("document-new"));
    //mNewProfileButton->setToolTip(tr("New Profile"));
    //mNewProfileButton->setShortcut(QKeySequence(tr("Ctrl+N")));

    mCopyProfileButton = new QPushButton(this);
    mCopyProfileButton->setIcon(QIcon::fromTheme("edit-copy"));
    //mCopyProfileButton->setToolTip(tr("Copy Profile"));

    mDeleteProfileButton = new QPushButton(this);
    mDeleteProfileButton->setIcon(QIcon::fromTheme("edit-delete"));
    //mDeleteProfileButton->setToolTip(tr("Delete Profile"));*/
    //mDeleteProfileButton->setShortcut(QKeySequence(tr("Delete")));

    //QHBoxLayout *bottomLayout = new QHBoxLayout();
    //bottomLayout = new QHBoxLayout();

    mProfileToolBar = new QToolBar(this);
    mProfileToolBar->setMovable(false);
    mProfileToolBar->setIconSize(QSize(16, 16));

    mProfileToolBar->addWidget(profileLabel);
    mProfileToolBar->addWidget(mProfilesComboBox);

    //splitter->addWidget(profileToolBar);
    //bottomLayout->addWidget(profileLabel);
    //bottomLayout->addWidget(mProfilesComboBox);
    /*bottomLayout->addWidget(mNewProfileButton);
    bottomLayout->addWidget(mCopyProfileButton);
    bottomLayout->addWidget(mDeleteProfileButton);*/

    QVBoxLayout *pageLayout = new QVBoxLayout(this);
    // Add some space above and below the page items
    QSpacerItem *vSpacer2 = new QSpacerItem(5, 5, QSizePolicy::Minimum, QSizePolicy::Minimum);


    pageLayout->addLayout(topLayout);
    pageLayout->addItem(vSpacer2);
    pageLayout->addWidget(splitter);
    pageLayout->addWidget(mProfileToolBar);

    connect(mMastersWidget->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this, SLOT(masterSelectionChanged(const QItemSelection&, const QItemSelection&)));

    connect(filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(const QString)));

    connect(mPluginsTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(setCheckState(QModelIndex)));
    connect(mPluginsTable, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));



    setupConfig();
    createActions();
}

void DataFilesPage::setupDataFiles(const QStringList &paths, bool strict)
{
    qDebug() << "setupDataFiles called!";

    // Put the paths in a boost::filesystem vector to use with Files::Collections
    std::vector<boost::filesystem::path> dataDirs;

    foreach (const QString &currentPath, paths) {
        dataDirs.push_back(boost::filesystem::path(currentPath.toStdString()));
    }

    // Create a file collection for the dataDirs
    Files::Collections mFileCollections(dataDirs, strict);

     // First we add all the master files
    const Files::MultiDirCollection &esm = mFileCollections.getCollection(".esm");
    unsigned int i = 0; // Row number

    for (Files::MultiDirCollection::TIter iter(esm.begin()); iter!=esm.end(); ++iter)
    {
        qDebug() << "Master: " << QString::fromStdString(iter->second.filename().string());

        QString currentMaster = QString::fromStdString(iter->second.filename().string());
        const QList<QTableWidgetItem*> itemList = mMastersWidget->findItems(currentMaster, Qt::MatchExactly);

        if (itemList.isEmpty()) // Master is not yet in the widget
        {
            qDebug() << "Master not yet in the widget, rowcount is " << i;

            mMastersWidget->insertRow(i);
            QTableWidgetItem *item = new QTableWidgetItem(currentMaster);
            mMastersWidget->setItem(i, 0, item);
            ++i;
        }
    }

    // Now on to the plugins
    const Files::MultiDirCollection &esp = mFileCollections.getCollection(".esp");

    for (Files::MultiDirCollection::TIter iter(esp.begin()); iter!=esp.end(); ++iter)
    {
        ESMReader fileReader;
        QStringList availableMasters; // Will contain all found masters

        fileReader.open(iter->second.string());

        // First we fill the availableMasters and the mMastersWidget
        ESMReader::MasterList mlist = fileReader.getMasters();

        for (unsigned int i = 0; i < mlist.size(); ++i) {
            const QString currentMaster = QString::fromStdString(mlist[i].name);
            availableMasters.append(currentMaster);

            const QList<QTableWidgetItem*> itemList = mMastersWidget->findItems(currentMaster, Qt::MatchExactly);

            if (itemList.isEmpty()) // Master is not yet in the widget
            {
                // TODO: Show warning, missing master
                mMastersWidget->insertRow(i);
                QTableWidgetItem *item = new QTableWidgetItem(currentMaster);
                mMastersWidget->setItem(i, 0, item);
            }
        }

        availableMasters.sort(); // Sort the masters alphabetically

        // Now we put the current plugin in the mDataFilesModel under its masters
        QStandardItem *parent = new QStandardItem(availableMasters.join(","));
        QStandardItem *child = new QStandardItem(QString::fromStdString(iter->second.filename().string()));

        const QList<QStandardItem*> masterList = mDataFilesModel->findItems(availableMasters.join(","));

        if (masterList.isEmpty()) { // Masters node not yet in the mDataFilesModel
            parent->appendRow(child);
            mDataFilesModel->appendRow(parent);
        } else {
            // Masters node exists, append current plugin
            foreach (QStandardItem *currentItem, masterList) {
                currentItem->appendRow(child);
            }
        }
    }

    readConfig();
}

void DataFilesPage::setupConfig()
{
    qDebug() << "setupConfig called";
    QString config = "./launcher.cfg";
    QFile file(config);

    if (!file.exists()) {
        config = QString::fromStdString(Files::getPath(Files::Path_ConfigUser,
                                                       "openmw", "launcher.cfg"));
    }

    file.setFileName(config); // Just for displaying information
    qDebug() << "Using config file from " << file.fileName();
    file.open(QIODevice::ReadOnly);
    qDebug() << "File contents:" << file.readAll();
    file.close();

    // Open our config file
    mLauncherConfig = new QSettings(config, QSettings::IniFormat);
    mLauncherConfig->sync();


    // Make sure we have no groups open
    while (!mLauncherConfig->group().isEmpty()) {
        mLauncherConfig->endGroup();
    }

    mLauncherConfig->beginGroup("Profiles");
    QStringList profiles = mLauncherConfig->childGroups();

    if (profiles.isEmpty()) {
        // Add a default profile
        profiles.append("Default");
    }

    mProfilesComboBox->addItems(profiles);

    QString currentProfile = mLauncherConfig->value("CurrentProfile").toString();

    qDebug() << mLauncherConfig->value("CurrentProfile").toString();
    qDebug() << mLauncherConfig->childGroups();

    if (currentProfile.isEmpty()) {
        qDebug() << "No current profile selected";
        currentProfile = "Default";
    }
    mProfilesComboBox->setCurrentIndex(mProfilesComboBox->findText(currentProfile));

    mLauncherConfig->endGroup();

    // Now we connect the combobox to do something if the profile changes
    // This prevents strange behaviour while reading and appending the profiles
    connect(mProfilesComboBox, SIGNAL(textChanged(const QString&, const QString&)), this, SLOT(profileChanged(const QString&, const QString&)));
}

void DataFilesPage::createActions()
{

    // Profile actions
    mNewProfileAction = new QAction(QIcon::fromTheme("document-new"), tr("&New Profile"), this);
    mNewProfileAction->setToolTip(tr("New Profile"));
    mNewProfileAction->setShortcut(QKeySequence(tr("Ctrl+N")));
    connect(mNewProfileAction, SIGNAL(triggered()), this, SLOT(newProfile()));


    mCopyProfileAction = new QAction(QIcon::fromTheme("edit-copy"), tr("&Copy Profile"), this);
    mCopyProfileAction->setToolTip(tr("Copy Profile"));
    mCopyProfileAction->setShortcut(QKeySequence(tr("Ctrl+C")));
    connect(mCopyProfileAction, SIGNAL(triggered()), this, SLOT(copyProfile()));

    mDeleteProfileAction = new QAction(QIcon::fromTheme("edit-delete"), tr("Delete Profile"), this);
    mDeleteProfileAction->setToolTip(tr("Delete Profile"));
    mDeleteProfileAction->setShortcut(QKeySequence(tr("Delete")));
    connect(mDeleteProfileAction, SIGNAL(triggered()), this, SLOT(deleteProfile()));

    // Add the newly created actions to the toolbar
    mProfileToolBar->addSeparator();
    mProfileToolBar->addAction(mNewProfileAction);
    mProfileToolBar->addAction(mCopyProfileAction);
    mProfileToolBar->addAction(mDeleteProfileAction);

    // Context menu actions
    mMoveUpAction = new QAction(QIcon::fromTheme("go-up"), tr("Move &Up"), this);
    mMoveUpAction->setShortcut(QKeySequence(tr("Ctrl+U")));
    connect(mMoveUpAction, SIGNAL(triggered()), this, SLOT(moveUp()));

    mMoveDownAction = new QAction(QIcon::fromTheme("go-down"), tr("&Move Down"), this);
    mMoveDownAction->setShortcut(QKeySequence(tr("Ctrl+M")));
    connect(mMoveDownAction, SIGNAL(triggered()), this, SLOT(moveDown()));

    mMoveTopAction = new QAction(QIcon::fromTheme("go-top"), tr("Move to Top"), this);
    mMoveTopAction->setShortcut(QKeySequence(tr("Ctrl+Shift+U")));
    connect(mMoveTopAction, SIGNAL(triggered()), this, SLOT(moveTop()));

    mMoveBottomAction = new QAction(QIcon::fromTheme("go-bottom"), tr("Move to Bottom"), this);
    mMoveBottomAction->setShortcut(QKeySequence(tr("Ctrl+Shift+M")));
    connect(mMoveBottomAction, SIGNAL(triggered()), this, SLOT(moveBottom()));

    mCheckAction = new QAction(tr("Check selected"), this);
    connect(mCheckAction, SIGNAL(triggered()), this, SLOT(check()));

    mUncheckAction = new QAction(tr("Uncheck selected"), this);
    connect(mUncheckAction, SIGNAL(triggered()), this, SLOT(uncheck()));

    // Makes shortcuts work even if the context menu is hidden
    this->addAction(mMoveUpAction);
    this->addAction(mMoveDownAction);
    this->addAction(mMoveTopAction);
    this->addAction(mMoveBottomAction);

    // Context menu for the plugins table
    mContextMenu = new QMenu(this);

    mContextMenu->addAction(mMoveUpAction);
    mContextMenu->addAction(mMoveDownAction);
    mContextMenu->addSeparator();
    mContextMenu->addAction(mMoveTopAction);
    mContextMenu->addAction(mMoveBottomAction);
    mContextMenu->addSeparator();
    mContextMenu->addAction(mCheckAction);
    mContextMenu->addAction(mUncheckAction);

}

void DataFilesPage::newProfile()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("New Profile"),
                                         tr("Profile Name:"), QLineEdit::Normal,
                                         tr("New Profile"), &ok);
    if (ok && !text.isEmpty()) {
        if (mProfilesComboBox->findText(text) != -1) {
            QMessageBox::warning(this, tr("Profile already exists"),
                                 tr("the profile <b>%0</b> already exists.").arg(text),
                                 QMessageBox::Ok);
        } else {
            // Add the new profile to the combobox
            mProfilesComboBox->addItem(text);
            mProfilesComboBox->setCurrentIndex(mProfilesComboBox->findText(text));

        }

    }

}

void DataFilesPage::copyProfile()
{
    QString profile = mProfilesComboBox->currentText();
    bool ok;

    QString text = QInputDialog::getText(this, tr("Copy Profile"),
                                         tr("Profile Name:"), QLineEdit::Normal,
                                         tr("%0 Copy").arg(profile), &ok);
    if (ok && !text.isEmpty()) {
        if (mProfilesComboBox->findText(text) != -1) {
            QMessageBox::warning(this, tr("Profile already exists"),
                                 tr("the profile <b>%0</b> already exists.").arg(text),
                                 QMessageBox::Ok);
        } else {
            // Add the new profile to the combobox
            mProfilesComboBox->addItem(text);

            // First write the current profile as the new profile
            writeConfig(text);
            mProfilesComboBox->setCurrentIndex(mProfilesComboBox->findText(text));

        }

    }

}

void DataFilesPage::deleteProfile()
{
    QString profile = mProfilesComboBox->currentText();


    if (profile.isEmpty()) {
        return;
    }

    QMessageBox deleteMessageBox(this);
    deleteMessageBox.setWindowTitle(tr("Delete Profile"));
    deleteMessageBox.setText(tr("Are you sure you want to delete <b>%0</b>?").arg(profile));
    deleteMessageBox.setIcon(QMessageBox::Warning);
    QAbstractButton *deleteButton =
    deleteMessageBox.addButton(tr("Delete"), QMessageBox::ActionRole);

    deleteMessageBox.addButton(QMessageBox::Cancel);

    deleteMessageBox.exec();

    if (deleteMessageBox.clickedButton() == deleteButton) {

        qDebug() << "Delete profile " << profile;

        // Make sure we have no groups open
        while (!mLauncherConfig->group().isEmpty()) {
            mLauncherConfig->endGroup();
        }

        mLauncherConfig->beginGroup("Profiles");

        // Open the profile-name subgroup
        mLauncherConfig->beginGroup(profile);
        mLauncherConfig->remove(""); // Clear the subgroup
        mLauncherConfig->endGroup();
        mLauncherConfig->endGroup();

        // Remove the profile from the combobox
        mProfilesComboBox->removeItem(mProfilesComboBox->findText(profile));
    }
}

void DataFilesPage::moveUp()
{
    // Shift the selected plugins up one row

    if (!mPluginsTable->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndexList selectedIndexes = mPluginsTable->selectionModel()->selectedIndexes();
    QModelIndex firstIndex = mPluginsProxyModel->mapToSource(selectedIndexes.first());

    // Check if the first selected plugin is the top one
    if (firstIndex.row() == 0) {
        return;
    }

    foreach (const QModelIndex &currentIndex, selectedIndexes) {
        QModelIndex sourceModelIndex = mPluginsProxyModel->mapToSource(currentIndex);
        int currentRow = sourceModelIndex.row();

        if (sourceModelIndex.isValid() && currentRow > 0) {
            mPluginsModel->insertRow((currentRow - 1), mPluginsModel->takeRow(currentRow));
        }
    }
}

void DataFilesPage::moveDown()
{
    // Shift the selected plugins down one row

    if (!mPluginsTable->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndexList selectedIndexes = mPluginsTable->selectionModel()->selectedIndexes();
    QModelIndex lastIndex = mPluginsProxyModel->mapToSource(selectedIndexes.last());

    // Check if last selected plugin is bottom one
    if ((lastIndex.row() + 1) == mPluginsModel->rowCount()) {
        return;
    }

    foreach (const QModelIndex &currentIndex, selectedIndexes) {
        QModelIndex sourceModelIndex = mPluginsProxyModel->mapToSource(currentIndex);
        int currentRow = sourceModelIndex.row();

        if (sourceModelIndex.isValid() && (currentRow + 1) < mPluginsModel->rowCount()) {
            mPluginsModel->insertRow((currentRow + 1), mPluginsModel->takeRow(currentRow));
        }
    }
}

void DataFilesPage::moveTop()
{
    // Move the selection to the top of the table

    if (!mPluginsTable->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndexList selectedIndexes = mPluginsTable->selectionModel()->selectedIndexes();
    QModelIndex firstIndex = mPluginsProxyModel->mapToSource(selectedIndexes.first());

    // Check if the first selected plugin is the top one
    if (firstIndex.row() == 0) {
        return;
    }

    for (int i=0; i < selectedIndexes.count(); ++i) {

        QModelIndex sourceModelIndex = mPluginsProxyModel->mapToSource(selectedIndexes.at(i));

        int currentRow = sourceModelIndex.row();

        if (sourceModelIndex.isValid() && currentRow > 0) {
            mPluginsModel->insertRow(i, mPluginsModel->takeRow(currentRow));
        }
    }
}

void DataFilesPage::moveBottom()
{
    // Move the selection to the bottom of the table

    if (!mPluginsTable->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndexList selectedIndexes = mPluginsTable->selectionModel()->selectedIndexes();
    QModelIndex lastIndex = mPluginsProxyModel->mapToSource(selectedIndexes.last());

    // Check if last selected plugin is bottom one
    if ((lastIndex.row() + 1) == mPluginsModel->rowCount()) {
        return;
    }

    for (int i=0; i < selectedIndexes.count(); ++i) {

        QModelIndex sourceModelIndex = mPluginsProxyModel->mapToSource(selectedIndexes.at(i));

        // Subtract iterations because takeRow shifts the rows below the taken row up
        int currentRow = sourceModelIndex.row() - i;

        if (sourceModelIndex.isValid() && (currentRow + 1) < mPluginsModel->rowCount()) {
            mPluginsModel->appendRow(mPluginsModel->takeRow(currentRow));
        }
    }
}

void DataFilesPage::check()
{
    // Check the current selection

    if (!mPluginsTable->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndexList selectedIndexes = mPluginsTable->selectionModel()->selectedIndexes();

    foreach (const QModelIndex &currentIndex, selectedIndexes) {
        QModelIndex sourceModelIndex = mPluginsProxyModel->mapToSource(currentIndex);

        if (sourceModelIndex.isValid()) {
            mPluginsModel->setData(sourceModelIndex, Qt::Checked, Qt::CheckStateRole);
        }
    }
}

void DataFilesPage::uncheck()
{
    // Uncheck the current selection

    if (!mPluginsTable->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndexList selectedIndexes = mPluginsTable->selectionModel()->selectedIndexes();

    foreach (const QModelIndex &currentIndex, selectedIndexes) {
        QModelIndex sourceModelIndex = mPluginsProxyModel->mapToSource(currentIndex);

        if (sourceModelIndex.isValid()) {
            mPluginsModel->setData(sourceModelIndex, Qt::Unchecked, Qt::CheckStateRole);
        }
    }
}


void DataFilesPage::showContextMenu(const QPoint &point)
{

    QPoint globalPos = mPluginsTable->mapToGlobal(point);

    QModelIndexList selectedIndexes = mPluginsTable->selectionModel()->selectedIndexes();

    // Show the check/uncheck actions depending on the state of the selected items
    mUncheckAction->setEnabled(false);
    mCheckAction->setEnabled(false);

    foreach (const QModelIndex &currentIndex, selectedIndexes) {
        if (currentIndex.isValid()) {

            if (isChecked(currentIndex)) {
                mUncheckAction->setEnabled(true);
            } else {
                mCheckAction->setEnabled(true);
            }
        }

    }

    QModelIndex firstIndex = mPluginsProxyModel->mapToSource(selectedIndexes.first());
    QModelIndex lastIndex = mPluginsProxyModel->mapToSource(selectedIndexes.last());

    // Check if selected first item is top row in model
    if (firstIndex.row() == 0) {
        mMoveUpAction->setEnabled(false);
        mMoveTopAction->setEnabled(false);
    }

    // Check if last row is bottom row in model
    if ((lastIndex.row() + 1) == mPluginsModel->rowCount()) {
        mMoveDownAction->setEnabled(false);
        mMoveBottomAction->setEnabled(false);
    }

    // Show menu
    mContextMenu->exec(globalPos);
}

void DataFilesPage::masterSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    if (mMastersWidget->selectionModel()->hasSelection()) {

        QStringList masters;
        QString masterstr;

        // Create a QStringList containing all the masters
        const QStringList masterList = selectedMasters();

        foreach (const QString &currentMaster, masterList) {
            masters.append(currentMaster);
        }

        masters.sort();
        masterstr = masters.join(","); // Make a comma-separated QString

        qDebug() << "Masters" << masterstr;

        // Iterate over all masters in the datafilesmodel to see if they are selected
        for (int r=0; r<mDataFilesModel->rowCount(); ++r) {
            QModelIndex currentIndex = mDataFilesModel->index(r, 0);
            QString master = currentIndex.data().toString();

            if (currentIndex.isValid()) {
                // See if the current master is in the string with selected masters
                if (masterstr.contains(master))
                {
                    // Append the plugins from the current master to pluginsmodel
                    addPlugins(currentIndex);
                }
            }
        }
    }

   // See what plugins to remove
   QModelIndexList deselectedIndexes = deselected.indexes();

   if (!deselectedIndexes.isEmpty()) {
        foreach (const QModelIndex &currentIndex, deselectedIndexes) {

            QString master = currentIndex.data().toString();
            master.prepend("*");
            master.append("*");
            const QList<QStandardItem *> itemList = mDataFilesModel->findItems(master, Qt::MatchWildcard);

            if (itemList.isEmpty())
                qDebug() << "Empty as shit";

            foreach (const QStandardItem *currentItem, itemList) {

                QModelIndex index = currentItem->index();
                qDebug() << "Master to remove plugins of:" << index.data().toString();

                removePlugins(index);
            }
        }
   }

}

void DataFilesPage::addPlugins(const QModelIndex &index)
{
    // Find the plugins in the datafilesmodel and append them to the pluginsmodel
    if (!index.isValid())
        return;

    for (int r=0; r<mDataFilesModel->rowCount(index); ++r ) {
        QModelIndex childIndex = index.child(r, 0);

        if (childIndex.isValid()) {
            // Now we see if the pluginsmodel already contains this plugin
            const QString childIndexData = QVariant(mDataFilesModel->data(childIndex)).toString();

            const QList<QStandardItem *> itemList = mPluginsModel->findItems(childIndexData);

            if (itemList.isEmpty())
            {
                // Plugin not yet in the pluginsmodel, add it
                QStandardItem *item = new QStandardItem(childIndexData);
                item->setFlags(item->flags() & ~(Qt::ItemIsDropEnabled));
                item->setCheckable(true);

                mPluginsModel->appendRow(item);
            }
        }

    }

}

void DataFilesPage::removePlugins(const QModelIndex &index)
{

    if (!index.isValid())
        return;

    for (int r=0; r<mDataFilesModel->rowCount(index); ++r) {
        QModelIndex childIndex = index.child(r, 0);

        const QList<QStandardItem *> itemList = mPluginsModel->findItems(QVariant(childIndex.data()).toString());

        if (!itemList.isEmpty()) {
            foreach (const QStandardItem *currentItem, itemList) {
                qDebug() << "Remove plugin:" << currentItem->data(Qt::DisplayRole).toString();

                mPluginsModel->removeRow(currentItem->row());
            }
        }
    }

}

void DataFilesPage::setCheckState(QModelIndex index)
{
    if (!index.isValid())
        return;

    QModelIndex sourceModelIndex = mPluginsProxyModel->mapToSource(index);

    if (mPluginsModel->data(sourceModelIndex, Qt::CheckStateRole) == Qt::Checked) {
        // Selected row is checked, uncheck it
        mPluginsModel->setData(sourceModelIndex, Qt::Unchecked, Qt::CheckStateRole);
    } else {
        mPluginsModel->setData(sourceModelIndex, Qt::Checked, Qt::CheckStateRole);
    }
}

bool DataFilesPage::isChecked(const QModelIndex &index)
{

    QModelIndex sourceModelIndex = mPluginsProxyModel->mapToSource(index);

    if (mPluginsModel->data(sourceModelIndex, Qt::CheckStateRole) == Qt::Checked) {
        return true;
    } else {
        return false;
    }

}

const QStringList DataFilesPage::selectedMasters()
{
    QStringList masters;
    const QList<QTableWidgetItem *> selectedMasters = mMastersWidget->selectedItems();

    foreach (const QTableWidgetItem *item, selectedMasters) {
        masters.append(item->data(Qt::DisplayRole).toString());
    }

    return masters;
}

const QStringList DataFilesPage::checkedPlugins()
{
    QStringList checkedItems;

    for (int r=0; r<mPluginsModel->rowCount(); ++r ) {
        QModelIndex index = mPluginsModel->index(r, 0);

        if (index.isValid()) {
            // See if the current item is checked
            if (mPluginsModel->data(index, Qt::CheckStateRole) == Qt::Checked) {
                checkedItems.append(index.data().toString());
            }
        }
    }
    return checkedItems;
}

void DataFilesPage::uncheckPlugins()
{
    for (int r=0; r<mPluginsModel->rowCount(); ++r ) {
        QModelIndex index = mPluginsModel->index(r, 0);

        if (index.isValid()) {
            // See if the current item is checked
            if (mPluginsModel->data(index, Qt::CheckStateRole) == Qt::Checked) {
                mPluginsModel->setData(index, Qt::Unchecked, Qt::CheckStateRole);
            }
        }
    }
}

void DataFilesPage::filterChanged(const QString filter)
{
    QRegExp regExp(filter, Qt::CaseInsensitive, QRegExp::FixedString);
    mPluginsProxyModel->setFilterRegExp(regExp);
}

void DataFilesPage::profileChanged(const QString &previous, const QString &current)
{
    qDebug() << "Profile changed " << current << previous;

    if (!previous.isEmpty()) {
        writeConfig(previous);
        mLauncherConfig->sync();
    }

    uncheckPlugins();
    // Deselect the masters
    mMastersWidget->selectionModel()->clearSelection();
    readConfig();
}

void DataFilesPage::readConfig()
{
    QString profile = mProfilesComboBox->currentText();
    qDebug() << "read from: " << profile;

    // Make sure we have no groups open
    while (!mLauncherConfig->group().isEmpty()) {
        mLauncherConfig->endGroup();
    }

    mLauncherConfig->beginGroup("Profiles");
    mLauncherConfig->beginGroup(profile);

    QStringList childKeys = mLauncherConfig->childKeys();
    QStringList plugins;

    // Sort the child keys numerical instead of alphabetically
    // i.e. Plugin1, Plugin2 instead of Plugin1, Plugin10
    qSort(childKeys.begin(), childKeys.end(), naturalSortLessThanCI);

    foreach (const QString &key, childKeys) {
        const QString keyValue = mLauncherConfig->value(key).toString();

        if (key.startsWith("Plugin")) {
            plugins.append(keyValue);
            continue;
        }

        if (key.startsWith("Master")) {
            qDebug() << "Read master: " << keyValue;
            const QList<QTableWidgetItem*> masterList = mMastersWidget->findItems(keyValue, Qt::MatchFixedString);

            if (!masterList.isEmpty()) {
                foreach (QTableWidgetItem *currentMaster, masterList) {
                    mMastersWidget->selectionModel()->select(mMastersWidget->model()->index(currentMaster->row(), 0), QItemSelectionModel::Select);
                }
            }
        }
    }

    // Iterate over the plugins to set their checkstate and position
    for (int i = 0; i < plugins.size(); ++i) {
        const QString plugin = plugins.at(i);

        const QList<QStandardItem *> pluginList = mPluginsModel->findItems(plugin);

        if (!pluginList.isEmpty())
        {
            foreach (const QStandardItem *currentPlugin, pluginList) {
                mPluginsModel->setData(currentPlugin->index(), Qt::Checked, Qt::CheckStateRole);

                // Move the plugin to the position specified in the config file
                mPluginsModel->insertRow(i, mPluginsModel->takeRow(currentPlugin->row()));
            }
        }
    }

}

void DataFilesPage::writeConfig(QString profile)
{
    // TODO: Testing the config here
    if (profile.isEmpty()) {
        profile = mProfilesComboBox->currentText();
    }

    if (profile.isEmpty()) {
        return;
    }

    qDebug() << "Writing: " << profile;

    // Make sure we have no groups open
    while (!mLauncherConfig->group().isEmpty()) {
        mLauncherConfig->endGroup();
    }

    mLauncherConfig->beginGroup("Profiles");
    mLauncherConfig->setValue("CurrentProfile", profile);

    // Open the profile-name subgroup
    qDebug() << "beginning group: " << profile;
    mLauncherConfig->beginGroup(profile);
    mLauncherConfig->remove(""); // Clear the subgroup

    // First write the masters to the config
    const QStringList masterList = selectedMasters();

    // We don't use foreach because we need i
    for (int i = 0; i < masterList.size(); ++i) {
        const QString master = masterList.at(i);
        mLauncherConfig->setValue(QString("Master%0").arg(i), master);
    }

    // Now write all checked plugins
    const QStringList plugins = checkedPlugins();

    for (int i = 0; i < plugins.size(); ++i)
    {
        mLauncherConfig->setValue(QString("Plugin%1").arg(i), plugins.at(i));
    }

    mLauncherConfig->endGroup();
    mLauncherConfig->endGroup();

}
