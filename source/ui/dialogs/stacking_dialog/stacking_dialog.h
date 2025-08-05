#ifndef STACKING_DIALOG_H
#define STACKING_DIALOG_H

#include <QDialog>
#include "data/media_collection.h"

namespace Ui {
class StackingDialog;
}

class StackingDialog : public QDialog {
    Q_OBJECT

public:
    explicit StackingDialog(QWidget *parent = nullptr);
    ~StackingDialog();

public slots:
    void includeFile(MediaFile *file, bool flag);

private:
    Ui::StackingDialog *ui;
    MediaCollection files;

    void analyzeFiles();
    void enableStackingOptions(bool flag);

    std::vector<std::pair<int, double>> frameQualities;
};

#endif // STACKING_DIALOG_H
