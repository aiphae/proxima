#ifndef STACKING_DIALOG_H
#define STACKING_DIALOG_H

#include <QDialog>
#include "data/media_collection.h"
#include "threading/analyze_thread.h"
#include "threading/stack_thread.h"
#include "stacking/alignment.h"

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
    void closed(const std::vector<std::string> &);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::StackingDialog *ui;
    MediaCollection _files;

    AnalyzeThread _analyzingThread;
    void _analyzeFiles();
    void _enableStackingOptions(bool flag);
    std::vector<std::pair<int, double>> _frameQualities;

    void _updateOutputDimensions();

    AlignmentPointSet _aps;
    ModifyingFunction _modifyingFunction;
    void _updateModifyingFunction();
    void _estimateAlignmentPoints();

    _StackThread _stackThread;
    _StackConfig _config;
    std::array<int, 4> _percentages;
    std::string _outputDir;
    void _stack();
    void _collectConfig();

    std::vector<std::string> _output;
};

#endif // STACKING_DIALOG_H
