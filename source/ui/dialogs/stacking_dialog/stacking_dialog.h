#ifndef STACKING_DIALOG_H
#define STACKING_DIALOG_H

#include <QDialog>
#include "data/media_collection.h"

namespace Ui {
class StackingDialog;
}

using ModifyingFunction = std::function<void(cv::Mat &)>;

class StackingDialog : public QDialog {
    Q_OBJECT

public:
    explicit StackingDialog(QWidget *parent = nullptr);
    ~StackingDialog();

public slots:
    void includeFile(MediaFile *file, bool flag);

signals:
    void analyzeFinished(MediaCollection *, const std::vector<int> &);
    void previewConfigChanged(ModifyingFunction);

private:
    Ui::StackingDialog *ui;
    MediaCollection _files;

    void _analyzeFiles();
    void _enableStackingOptions(bool flag);

    std::vector<std::pair<int, double>> _frameQualities;
};

#endif // STACKING_DIALOG_H
