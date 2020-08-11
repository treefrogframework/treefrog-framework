#pragma once
#include <QString>


class FileWriter {
public:
    FileWriter(const QString &filePath = QString());
    bool write(const QString &data, bool overwrite) const;
    QString fileName() const;
    void setFilePath(const QString &path) { filepath = path; }
    QString filePath() const { return filepath; }

private:
    bool write(const QString &data) const;
    QString filepath;
};

