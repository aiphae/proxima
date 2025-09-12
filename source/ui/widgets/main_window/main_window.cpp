#include "main_window.h"
#include <QFileDialog>
#include <QGroupBox>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUI_();
    connectUI_();
}

void MainWindow::connectUI_() {
    // 'Add items' push button
    connect(addItemsPushButton_, &QPushButton::clicked, this, [this]() {
        const auto files = QFileDialog::getOpenFileNames(this, "Select Files", QDir::homePath());
        if (files.isEmpty()) {
            return;
        }
        for (const auto &file : files) {
            workspace_->addItem(file.toStdString());
        }
        totalFilesEdit_->setText(QString::number(workspace_->itemCount()));
    });

    // 'Clear workspace' push button
    connect(clearWorkspacePushButton_, &QPushButton::clicked, this, [this]() {
        workspace_->clear();
        totalFilesEdit_->setText("");
        mediaViewer_->clear();
    });

    // Clicking on items in the workspace
    connect(workspace_, &Workspace::itemClicked, this, [this](MediaFile *file) {
        if (file == currentFile_) {
            return;
        }
        currentFile_ = file;
        showFile_();
    });

    // 'Stacking' dialog push button
    connect(stackingPushButton_, &QPushButton::clicked, this, [this]() {
        auto StackDialog = initializeStackDialog_();
        StackDialog->show();
    });
}

void MainWindow::showFile_() {
    mediaViewer_->show(currentFile_);
}

StackDialog *MainWindow::initializeStackDialog_() {
    auto stackDialog = new StackDialog(this);

    workspace_->enableMultipleSelection(true);

    connect(stackDialog, &StackDialog::analysisFinished, this, [this](MediaCollection *collection, const std::vector<int> &map) {
        // Show sorted frames
        mediaViewer_->show(*collection, map);
        // Block the workspace
        workspaceFrame_->setEnabled(false);
    });

    connect(stackDialog, &StackDialog::previewConfigChanged, this, [this](ModifyingFunction func) {
        mediaViewer_->setModifyingFunction(func);
    });

    // When the dialog is closed
    connect(stackDialog, &QDialog::finished, this, [this]() {
        // Disable multiple selection and reset check boxes
        workspace_->enableMultipleSelection(false);
        workspace_->resetMultipleSelection();
        workspaceFrame_->setEnabled(true);
        // Reset the preview
        mediaViewer_->setModifyingFunction({});
        if (currentFile_) {
            mediaViewer_->show(currentFile_);
        }
    });

    connect(workspace_, &Workspace::itemChecked, this, [stackDialog](MediaFile *file, bool flag) {
        stackDialog->includeFile(file, flag);
    });

    connect(stackDialog, &StackDialog::singleStackCompleted, this, [this](const std::string &filePath) {
        if (!filePath.empty()) {
            workspace_->addItem(filePath);
        }
    });

    return stackDialog;
}

void MainWindow::setupUI_() {
    auto centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    // Set the main window font size to 10
    QFont mainFont;
    mainFont.setPointSize(10);
    centralWidget->setFont(mainFont);

    auto mainLayout = new QHBoxLayout(centralWidget);

    // WORKSPACE FRAME
    workspaceFrame_ = new QFrame(centralWidget);
    workspaceFrame_->setFrameShape(QFrame::Box);
    auto workspaceLayout = new QGridLayout(workspaceFrame_);

    auto workspaceLabel = new QLabel("Workspace", workspaceFrame_);
    // Set the frame title font size to 11
    QFont labelFont;
    labelFont.setPointSize(11);
    workspaceLabel->setFont(labelFont);
    workspaceLayout->addWidget(workspaceLabel, 0, 0);

    workspace_ = new Workspace(workspaceFrame_);
    // Set the workspace font size to 9
    QFont workspaceFont;
    workspaceFont.setPointSize(9);
    workspace_->setFont(workspaceFont);
    workspace_->setMinimumWidth(300);
    workspaceLayout->addWidget(workspace_, 1, 0, 1, 2);

    workspaceLayout->addWidget(new QLabel("Total files:", workspaceFrame_), 4, 0);

    totalFilesEdit_ = new QLabel("", workspaceFrame_);
    totalFilesEdit_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    workspaceLayout->addWidget(totalFilesEdit_, 4, 1);

    addItemsPushButton_ = new QPushButton("Add items...", workspaceFrame_);
    addItemsPushButton_->setMinimumHeight(35);
    workspaceLayout->addWidget(addItemsPushButton_, 5, 0, 1, 2);

    clearWorkspacePushButton_ = new QPushButton("Clear workspace", workspaceFrame_);
    clearWorkspacePushButton_->setMinimumHeight(35);
    workspaceLayout->addWidget(clearWorkspacePushButton_, 6, 0, 1, 2);

    // Add to the main layout
    mainLayout->addWidget(workspaceFrame_);

    // DISPLAY FRAME
    displayFrame_ = new QFrame(centralWidget);
    displayFrame_->setFrameShape(QFrame::Box);
    auto displayLayout = new QVBoxLayout(displayFrame_);
    displayLayout->setContentsMargins(0, 9, 0, 0);

    mediaViewer_ = new MediaViewer(displayFrame_);
    mediaViewer_->setMinimumSize(300, 300);
    displayLayout->addWidget(mediaViewer_);

    // Add to the main layout
    mainLayout->addWidget(displayFrame_);

    // TOOLS FRAME
    toolsFrame_ = new QFrame(centralWidget);
    toolsFrame_->setFrameShape(QFrame::Box);
    toolsFrame_->setMinimumWidth(300);
    auto toolsLayout = new QVBoxLayout(toolsFrame_);

    auto toolsLabel = new QLabel("Tools", toolsFrame_);
    // Set the frame title font size to 11
    toolsLabel->setFont(labelFont);
    toolsLayout->addWidget(toolsLabel);

    // Video processing group
    auto videoGroup = new QGroupBox("Video processing", toolsFrame_);
    videoGroup->setFlat(true);
    auto videoLayout = new QVBoxLayout(videoGroup);
    videoLayout->setContentsMargins(0, 9, 0, 9);
    stackingPushButton_ = new QPushButton("Stacking", videoGroup);
    stackingPushButton_->setMinimumHeight(40);
    videoLayout->addWidget(stackingPushButton_);
    toolsLayout->addWidget(videoGroup);

    // Image processing group
    auto imageGroup = new QGroupBox("Image processing", toolsFrame_);
    imageGroup->setFlat(true);
    auto imageLayout = new QVBoxLayout(imageGroup);
    imageLayout->setContentsMargins(0, 9, 0, 9);

    QPushButton *colorCorrection = new QPushButton("Color correction", imageGroup);
    colorCorrection->setMinimumHeight(40);
    imageLayout->addWidget(colorCorrection);

    QPushButton *deconvolution = new QPushButton("Deconvolution", imageGroup);
    deconvolution->setMinimumHeight(40);
    imageLayout->addWidget(deconvolution);

    QPushButton *sharpening = new QPushButton("Multilayer sharpening", imageGroup);
    sharpening->setMinimumHeight(40);
    imageLayout->addWidget(sharpening);

    QPushButton *rgbAligning = new QPushButton("RGB aligning", imageGroup);
    rgbAligning->setMinimumHeight(40);
    imageLayout->addWidget(rgbAligning);

    toolsLayout->addWidget(imageGroup);

    // Spacer
    toolsLayout->addStretch();

    saveButton_ = new QPushButton("Save as...", toolsFrame_);
    saveButton_->setMinimumHeight(35);
    toolsLayout->addWidget(saveButton_);

    // Add to the main layout
    mainLayout->addWidget(toolsFrame_);
}
