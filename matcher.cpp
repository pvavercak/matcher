#include "matcher.h"

Matcher::Matcher()
{
    this->matcherIsRunning = false;
    this->matcher = bozorth3;
    this->thresholds = {50, static_cast<float>(0.15)};

    this->dbtestParams.numberOfSubject = 0;
    this->dbtestParams.imgPerSubject = 0;
    this->dbtestParams.genuineTestDone = false;

    this->dbtestResult.plotParams = {0, 500, 1};

    connect(&this->bozorth3m, SIGNAL(bozorthThreadsFinished(int)), this, SLOT(bozorthMatchingDone(int)));
}

Matcher::~Matcher() {}

void Matcher::cleanDBTestResults()
{
    this->dbtestParams.genuineTestDone = false;
    this->dbtestResult.fnmrX.clear(); this->dbtestResult.fnmrY.clear();
    this->dbtestResult.fmrX.clear(); this->dbtestResult.fmrY.clear();
    this->dbtestResult.rocX.clear(); this->dbtestResult.rocY.clear();
    this->dbtestResult.eer = -1;
    this->dbtestParams.keys.clear();
}

// SETTERS

int Matcher::setMatcher(MATCHER matcher)
{
    if (this->matcherIsRunning) {
        this->matcherError(10);
        return -1;
    }

    this->matcher = matcher;


    this->dbtestResult.plotParams = {0, 500, 1};
    return 1;
}

int Matcher::setDBTestParams(int numberOfSubject, int imgPerSubject)
{
    if (this->matcherIsRunning) {
        this->matcherError(10);
        return -1;
    }

    this->dbtestParams.numberOfSubject = numberOfSubject;
    this->dbtestParams.imgPerSubject = imgPerSubject;

    return 1;
}


// PAIR GENERATORS

void Matcher::generatePairs()
{
    this->fingerprintPairs.clear();

    for (auto i = this->alternativeNames.begin(); i != this->alternativeNames.end(); ++i) {
        this->fingerprintPairs.push_back(FINGERPRINT_PAIR{"SUBJECT", i.key(), 0});
    }
}

void Matcher::generateGenuinePairs()
{
    this->dbtestParams.genuinePairs.clear();

    for (int subject = 0; subject < this->dbtestParams.numberOfSubject; subject++) {
        for(int image1 = subject * this->dbtestParams.imgPerSubject; image1 < subject * this->dbtestParams.imgPerSubject + this->dbtestParams.imgPerSubject; image1++) {
            for(int image2 = image1+1; image2 < subject * this->dbtestParams.imgPerSubject + this->dbtestParams.imgPerSubject; image2++) {

                this->dbtestParams.genuinePairs.push_back(FINGERPRINT_PAIR{this->dbtestParams.keys[image1], this->dbtestParams.keys[image2], 0});

            }
        }
    }
}

void Matcher::generateImpostorPairs()
{   
    this->dbtestParams.impostorPairs.clear();

    for (int image1 = 0; image1 < (this->dbtestParams.numberOfSubject-1) * this->dbtestParams.imgPerSubject; image1 += this->dbtestParams.imgPerSubject) {
        for (int image2 = image1 + this->dbtestParams.imgPerSubject; image2 < this->dbtestParams.numberOfSubject * this->dbtestParams.imgPerSubject; image2 += this->dbtestParams.imgPerSubject) {

            this->dbtestParams.impostorPairs.push_back(FINGERPRINT_PAIR{this->dbtestParams.keys[image1], this->dbtestParams.keys[image2], 0});

        }
    }
}





//IDENTIFICATION, VERIFICATION, DBTEST

void Matcher::identify(unsigned char* subjectISO, const QMultiMap<QString, unsigned char*> &dbISO)
{
    if (this->matcherIsRunning) {
        this->matcherError(10);
        return;
    }
    else this->matcherIsRunning = true;

    this->mode = identification;

    //ALTERNATIVE_NAMES, BOZORTH TEMPLATES
    this->alternativeNames.clear();
    this->bozorthTemplates.clear();

    this->bozorthTemplates.insert("SUBJECT", this->isoConverter.convertFromISO(subjectISO));

    for (auto i = dbISO.begin(); i != dbISO.end(); ++i) {
        if (this->alternativeNames.contains(i.key())) {
            int cnt = 0;
            while (this->alternativeNames.contains(i.key() + "_" + QString::number(cnt))) {
                cnt++;
            }
            this->alternativeNames.insert(i.key() + "_" + QString::number(cnt), i.key());
            this->bozorthTemplates.insert(i.key() + "_" + QString::number(cnt), this->isoConverter.convertFromISO(i.value()));
        }
        else {
            this->alternativeNames.insert(i.key(), i.key());
            this->bozorthTemplates.insert(i.key(), this->isoConverter.convertFromISO(i.value()));
        }
    }
    this->generatePairs();
    this->bozorth3m.setParameters(QThread::idealThreadCount(), this->bozorthTemplates, this->fingerprintPairs);
    this->bozorth3m.matchAll();
}

void Matcher::identify(const QVector<MINUTIA> &subject, const QMultiMap<QString, QVector<MINUTIA>> &db)
{
    if (this->matcherIsRunning) {
        this->matcherError(10);
        return;
    }
    else this->matcherIsRunning = true;

    this->mode = identification;

    //ALTERNATIVE_NAMES, BOZORTH TEMPLATES
    this->alternativeNames.clear();
    this->bozorthTemplates.clear();

    this->bozorthTemplates.insert("SUBJECT", subject);

    for (auto i = db.begin(); i != db.end(); ++i) {
        if (this->alternativeNames.contains(i.key())) {
            int cnt = 0;
            while (this->alternativeNames.contains(i.key() + "_" + QString::number(cnt))) {
                cnt++;
            }
            this->alternativeNames.insert(i.key() + "_" + QString::number(cnt), i.key());
            this->bozorthTemplates.insert(i.key() + "_" + QString::number(cnt), i.value());
        }
        else {
            this->alternativeNames.insert(i.key(), i.key());
            this->bozorthTemplates.insert(i.key(), i.value());
        }
    }
    this->generatePairs();
    this->bozorth3m.setParameters(QThread::idealThreadCount(), this->bozorthTemplates, this->fingerprintPairs);
    this->bozorth3m.matchAll();
}

void Matcher::verify(unsigned char* subjectISO, const QVector<unsigned char *> &dbISO)
{
    if (this->matcherIsRunning) {
        this->matcherError(10);
        return;
    }
    else this->matcherIsRunning = true;

    this->mode = verification;
    // ALTERNATIVE NAMES
    this->alternativeNames.clear();
    this->bozorthTemplates.clear();

    this->bozorthTemplates.insert("SUBJECT", this->isoConverter.convertFromISO(subjectISO));

    for (int i = 0; i < dbISO.size(); i++) {
        this->alternativeNames.insert(QString::number(i), QString::number(i));
        this->bozorthTemplates.insert(QString::number(i), this->isoConverter.convertFromISO(dbISO[i]));
    }
    this->generatePairs();
    this->bozorth3m.setParameters(QThread::idealThreadCount(), this->bozorthTemplates, this->fingerprintPairs);
    this->bozorth3m.matchAll();
}

void Matcher::verify(const QVector<MINUTIA> &subject, const QVector<QVector<MINUTIA> > &db)
{
    if (this->matcherIsRunning) {
        this->matcherError(10);
        return;
    }
    else this->matcherIsRunning = true;

    this->mode = verification;
    // ALTERNATIVE NAMES
    this->alternativeNames.clear();
    this->bozorthTemplates.clear();

    this->bozorthTemplates.insert("SUBJECT", subject);

    for (int i = 0; i < db.size(); i++) {
        this->alternativeNames.insert(QString::number(i), QString::number(i));
        this->bozorthTemplates.insert(QString::number(i), db[i]);
    }
    this->generatePairs();
    this->bozorth3m.setParameters(QThread::idealThreadCount(), this->bozorthTemplates, this->fingerprintPairs);
    this->bozorth3m.matchAll();
}

void Matcher::testDatabase(QMap<QString, QVector<MINUTIA>> &db)
{
    if (this->matcherIsRunning) {
        this->matcherError(10);
        return;
    }
    else this->matcherIsRunning = true;

    this->mode = dbtest;
    // GENUINES
    this->bozorthTemplates.clear();

    for (auto i = db.begin(); i != db.end(); ++i) {
        this->dbtestParams.keys.push_back(i.key());
        //this->boostMinutiae(i.value(), 60);
        this->bozorthTemplates.insert(i.key(), i.value());
    }
    this->generateGenuinePairs();
    this->generateImpostorPairs();

    this->bozorth3m.setParameters(QThread::idealThreadCount(), this->bozorthTemplates, this->dbtestParams.genuinePairs);
    this->bozorth3m.matchAll();
}

void Matcher::testDatabase(const QMap<QString, unsigned char *> &dbISO)
{
    if (this->matcherIsRunning) {
        this->matcherError(10);
        return;
    }
    else this->matcherIsRunning = true;

    this->mode = dbtest;
    this->dbtestParams.db.clear();

    for (auto i = dbISO.begin(); i != dbISO.end(); ++i) {
        this->dbtestParams.db.insert(i.key(), this->isoConverter.convertFromISO(i.value()));
    }

    this->matcherIsRunning = false;
    this->testDatabase(this->dbtestParams.db);
}

DBTEST_RESULT Matcher::getDbtestResult() const
{
    return dbtestResult;
}

//SLOTS
void Matcher::bozorthMatchingDone(int duration)
{
    if (this->mode == identification) {
        this->fingerprintPairs = this->bozorth3m.getOutputFingerprintPairs();

        int bestMatch = this->findMaxScoreItem();
        if (this->fingerprintPairs[bestMatch].score >= this->thresholds.bozorthThr)
            emit identificationDoneSignal(true, this->alternativeNames.value(this->fingerprintPairs[bestMatch].rightFingerprint), this->fingerprintPairs[bestMatch].score);
        else emit identificationDoneSignal(false, this->alternativeNames.value(this->fingerprintPairs[bestMatch].rightFingerprint), this->fingerprintPairs[bestMatch].score);

        this->matcherIsRunning = false;
    }
    else if (this->mode == verification) {
        this->fingerprintPairs = this->bozorth3m.getOutputFingerprintPairs();

        int bestMatch = this->findMaxScoreItem();
        if (this->fingerprintPairs[bestMatch].score >= this->thresholds.bozorthThr)
            emit verificationDoneSignal(true);
        else emit verificationDoneSignal(false);

        this->matcherIsRunning = false;
    }
    else if (this->mode == dbtest) {

        double error;

        if (!this->dbtestParams.genuineTestDone) {
            this->dbtestParams.genuinePairs = this->bozorth3m.getOutputFingerprintPairs();

            for(int threshold = this->dbtestResult.plotParams.min; threshold < this->dbtestResult.plotParams.max; threshold += this->dbtestResult.plotParams.sensitivity) {
                this->dbtestResult.fnmrX.push_back(threshold);
                error = std::count_if(this->dbtestParams.genuinePairs.begin(), this->dbtestParams.genuinePairs.end(), [=](FINGERPRINT_PAIR fp) {return fp.score < threshold;});
                this->dbtestResult.fnmrY.push_back(error/this->dbtestParams.genuinePairs.size()*100);
            }
            this->dbtestParams.genuineTestDone = true;

            emit matcherProgressSignal(50);

            // IMPOSTORS
            this->bozorth3m.setParameters(QThread::idealThreadCount(), this->bozorthTemplates, this->dbtestParams.impostorPairs);
            this->bozorth3m.matchAll();
        }
        else {

            this->dbtestParams.impostorPairs = this->bozorth3m.getOutputFingerprintPairs();
            for(int threshold = this->dbtestResult.plotParams.min; threshold < this->dbtestResult.plotParams.max; threshold += this->dbtestResult.plotParams.sensitivity) {
                this->dbtestResult.fmrX.push_back(threshold);
                error = std::count_if(this->dbtestParams.impostorPairs.begin(), this->dbtestParams.impostorPairs.end(), [=](FINGERPRINT_PAIR fp) {return fp.score >= threshold;});
                this->dbtestResult.fmrY.push_back(error/this->dbtestParams.impostorPairs.size()*100);
            }
            this->compureROCValues();
            this->dbtestResult.eer = this->computeEERValue();

            emit matcherProgressSignal(100);
            emit dbTestDoneSignal(dbtestResult);

            this->matcherIsRunning = false;
            this->cleanDBTestResults();
        }
    }
}

// OTHER
int Matcher::findMaxScoreItem()
{
    int maxItemNum = 0;
    float max = this->fingerprintPairs[0].score;

    for (int i = 1; i < this->fingerprintPairs.size(); i++) {
        if (this->fingerprintPairs[i].score > max) maxItemNum = i;
    }

    return maxItemNum;
}

double Matcher::computeEERValue()
{
    QVector<double> absDiff;
    for(int i=0; i < this->dbtestResult.fmrY.size(); i++){
        absDiff.push_back(qAbs(this->dbtestResult.fmrY[i] - this->dbtestResult.fnmrY[i]));
    }
    double smallestDiff = *std::min_element(absDiff.begin(), absDiff.end());

    for(int i=0; i< absDiff.size(); i++){
        if(absDiff[i] == smallestDiff) {
            return (this->dbtestResult.fmrY[i] + this->dbtestResult.fnmrY[i])/2.0;
        }
    }

    return 0;
}

void Matcher::compureROCValues()
{
    foreach (double  i, this->dbtestResult.fnmrY) {
        this->dbtestResult.rocY.push_back(1-i/100.0);
    }

    foreach (double  i, this->dbtestResult.fmrY) {
        this->dbtestResult.rocX.push_back(i/100.0);
    }
}

void Matcher::boostMinutiae(QVector<MINUTIA> &mv, int minMinutiae)
{
    qSort(mv.begin(), mv.end(),[=](const MINUTIA& left, const MINUTIA& right){return left.quality > right.quality;});

    int cnt = 0;
    int origSize = mv.size();
    while (mv[cnt].quality > 79 && mv.size() != 2*origSize) {
        mv.push_back(mv[cnt++]);
    }
}

// ERROR
void Matcher::matcherError(int errorCode)
{
    /* Error codes:
     *
     *
     *
    */

    emit this->matcherErrorSignal(errorCode);
}
