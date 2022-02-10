/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/


#include <QStringList>
#include "rs_filtercxf.h"

#include <iostream>
#include <fstream>

#include "rs_arc.h"
#include "rs_line.h"
#include "rs_font.h"
#include "rs_utility.h"
#include "rs_system.h"
#include "rs_block.h"
#include "rs_math.h"
#include "rs_debug.h"


/**
 * Default constructor.
 */
RS_FilterCXF::RS_FilterCXF() : RS_FilterInterface() {

    RS_DEBUG->print("Setting up CXF filter...");
}

/**
 * Implementation of the method used for RS_Import to communicate
 * with this filter.
 *
 * @param graphic The graphic in which the entities from the file
 * will be created or the graphics from which the entities are
 * taken to be stored in a file.
 *
 * TODO RS_FilterLFF::fileImport and RS_FilterCXF::fileImport look very similar
 */
bool RS_FilterCXF::fileImport(RS_Graphic& graphic, const QString& file, RS2::FormatType /*type*/) {
    RS_DEBUG->print("CXF Filter: importing file '%s'...", file.toLatin1().data());

    // Load font file as we normally do, but the font doesn't own the
    //  letters (we'll add them to the graphic instead. Hence 'false').
    RS_Font font(file, false);
    bool success = font.loadFont();

    if (!success) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "Cannot open CXF file '%s'.", file.toLatin1().data());
		return false;
    }

    graphic.getVariables()->add("Names",
                        font.getNames().join(","), 0);
    graphic.getVariables()->add("LetterSpacing", font.getLetterSpacing(), 0);
    graphic.getVariables()->add("WordSpacing", font.getWordSpacing(), 0);
    graphic.getVariables()->add("LineSpacingFactor", font.getLineSpacingFactor(), 0);
    graphic.getVariables()->add("Authors", font.getAuthors().join(","), 0);
    graphic.getVariables()->add("License", font.getFileLicense(), 0);
    graphic.getVariables()->add("Created", font.getFileCreate(), 0);
    if (!font.getEncoding().isEmpty()) {
        graphic.getVariables()->add("Encoding", font.getEncoding(), 0);
    }

    RS_BlockList* letterList = font.getLetterList();
    for (auto & ch : *font.getLetterList()) {
        QString uCode;
        uCode.setNum(letter->getName().at(0).unicode(), 16);
        while (uCode.length()<4) {
            uCode="0"+uCode;
        }
        letterList->rename(letter,
                           QString("[%1] %2").arg(uCode).arg(letter->getName().at(0)));

        g.getBlockList()->add(ch, false);
        ch->reparent(&g);
    }

    g.getBlockList()->addNotification();

	return true;
}



/**
 * Implementation of the method used for RS_Export to communicate
 * with this filter.
 *
 * @param file Full path to the CXF file that will be written.
 */
bool RS_FilterCXF::fileExport(RS_Graphic& g, const QString& file, RS2::FormatType /*type*/) {

    RS_DEBUG->print("CXF Filter: exporting file '%s'...", file.toLatin1().data());

    // crashes under windoze xp:
    //std::ofstream fout;

    RS_DEBUG->print("RS_FilterCXF::fileExport: open");
    //fout.open((const char*)file.toLocal8Bit());
    FILE* fp;

    if ((fp = fopen(file.toLocal8Bit(), "wt")) != NULL) {

        RS_DEBUG->print("RS_FilterCXF::fileExport: open: OK");

        RS_DEBUG->print("RS_FilterCXF::fileExport: header");

        // header:
        fprintf(fp, "# Format:            QCad II Font\n");

        fprintf(fp, "# Creator:           %s\n",
                (const char*)RS_SYSTEM->getAppName().toLocal8Bit());
        fprintf(fp, "# Version:           %s\n",
                (const char*)RS_SYSTEM->getAppVersion().toLocal8Bit());

        RS_DEBUG->print("001");
        QString ns = g.getVariables()->getString("Names", "");
        if (!ns.isEmpty()) {
            QStringList names = ns.split(',');
            RS_DEBUG->print("002");
            for (int i = 0; i < names.size(); ++i) {
                fprintf(fp, "# Name:              %s\n",
                        names.at(i).toLocal8Bit().data() );
             }
        }

        RS_DEBUG->print("003");

        QString es = g.getVariables()->getString("Encoding", "");
        if (!es.isEmpty()) {
            fprintf(fp, "# Encoding:          %s\n",
                    es.toLocal8Bit().data());
        }

        RS_DEBUG->print("004a");

        fprintf(fp, "# LetterSpacing:     %f\n",
                g.getVariables()->getDouble("LetterSpacing", 3.0));
        fprintf(fp, "# WordSpacing:       %f\n",
                g.getVariables()->getDouble("WordSpacing", 6.75));
        fprintf(fp, "# LineSpacingFactor: %f\n",
                g.getVariables()->getDouble("LineSpacingFactor", 1.0));

        QString sa = g.getVariables()->getString("Authors", "");
        RS_DEBUG->print("authors: %s", sa.toLocal8Bit().data());
        if (!sa.isEmpty()) {
            QStringList authors = sa.split(',');
            RS_DEBUG->print("006");
            RS_DEBUG->print("count: %d", authors.count());

            QString a;
            for (QStringList::Iterator it2 = authors.begin();
                    it2!=authors.end(); ++it2) {

                RS_DEBUG->print("006a");
                a = QString(*it2);
                RS_DEBUG->print("006b");
                RS_DEBUG->print("string is: %s", a.toLatin1().data());
                RS_DEBUG->print("006b0");
                fprintf(fp, "# Author:            ");
                RS_DEBUG->print("006b1");
                fprintf(fp, "%s\n", a.toLatin1().data());
                //fout << "# Author:            " << a.ascii() << "\n";
            }
            RS_DEBUG->print("007");
        }

        RS_DEBUG->print("RS_FilterCXF::fileExport: header: OK");

        RS_DEBUG->print("008");
        // iterate through blocks (=letters of font)
        int i = -1;
        for (auto & blk : *g.getBlockList()) {
            i++;
            RS_DEBUG->print("block: %d", i);
            RS_DEBUG->print("001");

            if (blk && !blk->isUndone()) {
                RS_DEBUG->print("002");
                RS_DEBUG->print("002a: %s",
                                (blk->getName().toLocal8Bit().data()));

                fprintf(fp, "\n%s\n",
                        (blk->getName().toLocal8Bit().data()));


                // iterate through entities of this letter:
                for (RS_Entity* e=blk->firstEntity(RS2::ResolveAll);
                        e;
                        e=blk->nextEntity(RS2::ResolveAll)) {

                    if (!e->isUndone()) {

                        RS_DEBUG->print("004");

                        // lines:
                        if (e->rtti()==RS2::EntityLine) {
                            RS_Line* l = (RS_Line*)e;

                            fprintf(fp, "L %f,%f,%f,%f\n",
                                    l->getStartpoint().x,
                                    l->getStartpoint().y,
                                    l->getEndpoint().x,
                                    l->getEndpoint().y);
                        }

                        // arcs:
                        else if (e->rtti()==RS2::EntityArc) {
                            RS_Arc* a = (RS_Arc*)e;

                            if (!a->isReversed()) {
                                fprintf(fp, "A ");
                            } else {
                                fprintf(fp, "AR ");
                            }

                            fprintf(fp, "%f,%f,%f,%f,%f\n",
                                    a->getCenter().x,
                                    a->getCenter().y,
                                    a->getRadius(),
									RS_Math::rad2deg(a->getAngle1()),
									RS_Math::rad2deg(a->getAngle2())
													 );
                        }
                        // Ignore entities other than arcs / lines
                        else {}
                    }

                    RS_DEBUG->print("005");
                }
                RS_DEBUG->print("006");
            }
            RS_DEBUG->print("007");
        }
        //fout.close();
        fclose(fp);
    	RS_DEBUG->print("CXF Filter: exporting file: OK");
		return true;
    }
	else {
    	RS_DEBUG->print("CXF Filter: exporting file failed");
	}

	return false;
}



/**
 * Streams a double value to the gien stream cutting away trailing 0's.
 *
 * @param value A double value. e.g. 2.700000
 */
void RS_FilterCXF::stream(std::ofstream& fs, double value) {
    fs << RS_Utility::doubleToString(value).toLatin1().data();
}

