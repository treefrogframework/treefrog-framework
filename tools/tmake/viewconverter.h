#pragma once
#include <QDir>
#include <QFileInfo>
#include <QString>


class ViewConverter {
public:
    ViewConverter(const QDir &view, const QDir &output, bool projectFile = true);
    ~ViewConverter() { }

    int convertView(const QString &templateSystem) const;
    void setCodec(const QString &name) { codecName = name; }
    static QString getViewClassName(const QString &filePath);
    static QString getViewClassName(const QFileInfo &fileInfo);
    static QString changeFileExtension(const QString &filePath, const QString &ext);

protected:
    bool createProjectFile() const;
    bool createSourceList(const QStringList &classNameList, const QStringList &viewFileList) const;
    bool write(const QString &filePath, const QString &data) const;

private:
    QString codecName;
    QDir viewDir;
    QDir outputDir;
    bool createProFile;
};

