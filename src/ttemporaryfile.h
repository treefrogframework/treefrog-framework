#pragma once
#include <QTemporaryFile>
#include <TGlobal>


class T_CORE_EXPORT TTemporaryFile : public QTemporaryFile {
public:
    TTemporaryFile();
    bool open();
    QString absoluteFilePath() const;

protected:
    bool open(OpenMode flags);

private:
    T_DISABLE_COPY(TTemporaryFile)
    T_DISABLE_MOVE(TTemporaryFile)
};

