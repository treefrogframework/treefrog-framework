#ifndef TTEMPORARYFILE_H
#define TTEMPORARYFILE_H

#include <QTemporaryFile>
#include <TGlobal>


class T_CORE_EXPORT TTemporaryFile : public QTemporaryFile
{
public:
    TTemporaryFile();
    bool open();
    QString absoluteFilePath() const;

protected:
    bool open(OpenMode flags);

private:
    Q_DISABLE_COPY(TTemporaryFile)
};

#endif // TTEMPORARYFILE_H
