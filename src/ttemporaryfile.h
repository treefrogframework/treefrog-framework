#ifndef TTEMPORARYFILE_H
#define TTEMPORARYFILE_H

#include <QTemporaryFile>
#include <TGlobal>


class T_CORE_EXPORT TTemporaryFile : public QTemporaryFile
{
public:
    TTemporaryFile();
    QString absoluteFilePath() const;

private:
    Q_DISABLE_COPY(TTemporaryFile) 
};

#endif // TTEMPORARYFILE_H
