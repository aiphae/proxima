#ifndef STACKING_DIALOG_H
#define STACKING_DIALOG_H

#include <QCheckBox>
#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QCloseEvent>

#include "data/media_collection.h"
#include "threading/analysis_thread.h"
#include "threading/stack_thread.h"
#include "stacking/alignment.h"

using ModifyingFunction = std::function<void(cv::Mat &)>;

class StackDialog : public QDialog {
    Q_OBJECT

public:
    explicit StackDialog(QWidget *parent = nullptr);
    ~StackDialog() override = default;

public slots:
    void includeFile(MediaFile *file, bool flag);

signals:
    void analysisFinished(MediaCollection *, const std::vector<int> &);
    void previewConfigChanged(ModifyingFunction);
    void singleStackCompleted(const std::string &);

private:
    void setupUI_();
    void connectUI_();

    MediaCollection files_;

    void analyzeFiles_();
    void enableStackingOptions_(bool flag) const;
    std::vector<std::pair<int, double>> frameQualities_;

    void updateOutputDimensions_();

    ModifyingFunction modifyingFunction_;
    void updateModifyingFunction_();
    void estimateAlignmentPoints_();

    StackConfig config_;
    AlignmentPointSet aps_;
    void stack_();

    // UI elements
    // 'Input' group box
    QLabel *selectedFilesEdit_;
    QLabel *totalFramesEdit_;

    // 'Analysis' group box
    QGroupBox *analysisGroupBox_;
    QPushButton *analyzeFramesPushButton_;
    QLabel *analysisProgressEdit_;

    // 'Alignment' group box
    QGroupBox *alignmentGroupBox_;
    QCheckBox *localAlignmentCheckBox_;
    QFrame *localAlignmentOptionsFrame_;
    QSpinBox *apSizeSpinBox_;
    QCheckBox *featureBasedAlignmentCheckBox_;
    QCheckBox *showApsCheckBox_;

    // 'Output options' group box
    QGroupBox *outputOptionsGroupBox_;
    QSpinBox *cropWidthSpinBox_;
    QSpinBox *cropHeightSpinBox_;
    QCheckBox *upsampleCheckBox_;
    QDoubleSpinBox *upsampleFactorDoubleSpinBox_;

    // 'Stacking options' group box
    QGroupBox *stackingOptionsGroupBox_;
    std::array<QSpinBox *, 4> percentageSpinBoxes_;

    // 'Stack' group box
    QGroupBox *stackGroupBox_;
    QCheckBox *addToWorkspaceCheckBox_;
    QPushButton *stackPushButton_;
    QLabel *stackProgressEdit_;
};

#endif // STACKING_DIALOG_H
