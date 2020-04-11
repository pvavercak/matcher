#ifndef MATCHER_H
#define MATCHER_H

#include "matcher_global.h"
#include "matcher_config.h"

#include "matcher_isoconverter.h"
#include "bozorth3m.h"

class MATCHERSHARED_EXPORT Matcher : public QObject
{
    Q_OBJECT

public:
    Matcher();
    ~Matcher();

    BozorthMultiThreadManager bozorth3m;

    int setMatcher(MATCHER matcher=bozorth3);
    int setDBTestParams(int numberOfSubject, int imgPerSubject);

    void identify(unsigned char* subjectISO, const QMultiMap<QString, QVector<uchar> > &dbISO);
    void identify(const QVector<MINUTIA> &subject, const QMultiMap<QString, QVector<MINUTIA> > &db, const qintptr &requester);

    void verify(unsigned char *subjectISO, const QVector<unsigned char *> &dbISO);
    void verify(const QVector<MINUTIA> &subject, const QVector<QVector<MINUTIA> > &db, const qintptr &requester);

    void testDatabase(QMap<QString, QVector<MINUTIA> > &db);
    void testDatabase(const QMap<QString, unsigned char *> &dbISO);

    DBTEST_RESULT getDbtestResult() const;

private:

    MATCHER matcher;
    MatcherISOConverter isoConverter;

    bool matcherIsRunning;
    qintptr requester;

    MODE mode;
    MATCH_TRESHOLDS thresholds;

    QMap<QString, QVector<MINUTIA>> bozorthTemplates;

    FINGERPRINT_PAIRS fingerprintPairs;
    QMap<QString, QString> alternativeNames;

    DBTEST_PARAMS dbtestParams;
    DBTEST_RESULT dbtestResult;

    void generatePairs();
    void generateGenuinePairs();
    void generateImpostorPairs();

    int findMaxScoreItem();
    double computeEERValue();
    void compureROCValues();
    void boostMinutiae(QVector<MINUTIA> &mv, int minMinutiae);

    void cleanDBTestResults();
    void matcherError(int errorCode);

private slots:
    void bozorthMatchingDone(int duration);

signals:

    void identificationDoneSignal(bool success, QString bestSubject, float bestScore, const qintptr& requester);
    void verificationDoneSignal(bool success, const qintptr& requester);
    void dbTestDoneSignal(DBTEST_RESULT result);

    void matcherProgressSignal(int progress);
    void matcherErrorSignal(int errorCode);
};

#endif // MATCHER_H
