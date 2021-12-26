//
// Created by hawk on 25.12.21.
//

#ifndef LIBRECAD_CONVERTLINE_H
#define LIBRECAD_CONVERTLINE_H
#include <qhash.h>

class ConvertLine {
public:
    ConvertLine();

    QString lt2str(enum RS2::LineType lt) const;

    QString lw2str(enum RS2::LineWidth lw) const;

    static QString intColor2str(int col);

    enum RS2::LineType str2lt(const QString& s) const;

    enum RS2::LineWidth str2lw(const QString& w) const;

private:
    QHash<RS2::LineType, QString> lType;
    QHash<RS2::LineWidth, QString> lWidth;
};

ConvertLine getConverter();
#endif //LIBRECAD_CONVERTLINE_H
