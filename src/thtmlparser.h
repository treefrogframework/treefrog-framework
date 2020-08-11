#pragma once
#include <QList>
#include <QPair>
#include <QString>
#include <QVector>
#include <TGlobal>


class T_CORE_EXPORT THtmlElement {
public:
    THtmlElement();
    bool isEmpty() const;
    bool isEndElement() const;
    bool hasAttribute(const QString &name) const;
    QString attribute(const QString &name, const QString &defaultValue = QString()) const;
    void setAttribute(const QString &name, const QString &value);
    void removeAttribute(const QString &name);
    void clear();
    QString attributesString() const;
    QString toString() const;

    QString tag;
    QList<QPair<QString, QString>> attributes;
    QString selfCloseMark;
    QString text;
    bool tagClosed {false};
    int parent {0};
    QVector<int> children;
};


class T_CORE_EXPORT THtmlParser {
public:
    enum TrimMode {
        TrimOff = 0,
        NormalTrim,  // Removes whitespaces if the start is "<%" and the end is "%>"
        StrongTrim,  // Removes whitespaces from the start and the end
    };

    THtmlParser(TrimMode trimMode = NormalTrim);
    void parse(const QString &text);
    int elementCount() const { return elements.count(); }
    int lastIndex() const { return elements.count() - 1; }
    const THtmlElement &at(int i) const { return elements.at(i); }
    THtmlElement &at(int i) { return elements[i]; }
    THtmlElement &last() { return elements.last(); }
    const THtmlElement &last() const { return elements.last(); }
    bool isElementClosed(int i) const;
    THtmlElement &appendNewElement(int parent);
    THtmlElement &insertNewElement(int parent, int index);
    THtmlElement &appendElementTree(int parent, const THtmlElement &element);
    void removeElementTree(int index, bool removeNewline = false);
    void removeChildElements(int index);
    void removeTag(int index);
    THtmlParser mid(int index) const;
    void append(int parent, const THtmlParser &parser);
    void prepend(int parent, const THtmlParser &parser);
    void merge(const THtmlParser &other);
    int depth(int i) const;
    QString toString() const;
    QString elementsToString(int index) const;
    QString childElementsToString(int index) const;
    bool parentExists(int i, const QString &tag) const;
    static THtmlParser mergeElements(const QString &s1, const QString &s2);
    static bool isTag(const QString &tag);
    static QString trim(const QString &str);

protected:
    void parse();
    void parseTag();
    bool isTag(int position) const;
    QList<QPair<QString, QString>> parseAttributes();
    void parseCloseTag();
    QString parseWord();
    void skipWhiteSpace(int *crCount = 0, int *lfCount = 0);
    void skipUpTo(const QString &str);
    bool hasPrefix(const QString &str, int offset = 0) const;
    void changeParent(int index, int newParent, int newIndex = -1);
    void appendTextToLastElement(const QString &upto);
    int nextElementInSameParent(int index) const;
    //void dumpHtml() const;

private:
    int trimMode;
    QVector<THtmlElement> elements;
    QString txt;
    int pos;
};

