#pragma once
#include <QString>
#include <TGlobal>

class TFileAioWriterData;


class T_CORE_EXPORT TFileAioWriter {
public:
    TFileAioWriter(const QString &name = QString());
    ~TFileAioWriter();

    bool open();
    void close();
    bool isOpen() const;
    int write(const char *data, int length);
    void flush();
    void setFileName(const QString &name);
    QString fileName() const;

private:
    TFileAioWriterData *d {nullptr};

    T_DISABLE_COPY(TFileAioWriter)
    T_DISABLE_MOVE(TFileAioWriter)
};

