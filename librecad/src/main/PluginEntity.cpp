//
// Created by hawk on 26.12.21.
//
#include "rs_graphicview.h"
#include "rs_actioninterface.h"
#include "rs_eventhandler.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_line.h"
#include "rs_point.h"
#include "rs_mtext.h"
#include "rs_text.h"
#include "rs_layer.h"
#include "rs_image.h"
#include "rs_block.h"
#include "rs_insert.h"
#include "rs_polyline.h"
#include "rs_ellipse.h"
#include "rs_math.h"
#include "ConvertLine.h"

#include "PluginEntity.h"

PluginEntity::PluginEntity(RS_Entity *ent, DocumentPluginImplementation *d) :
        entity(ent), hasContainer(true), dpi(d) {
}

PluginEntity::PluginEntity(RS_EntityContainer *parent, enum DPI::ETYPE type) {
    hasContainer = false;
    dpi = nullptr;
    entity = nullptr;
    switch (type) {
        case DPI::POINT:
            entity = new RS_Point(parent, RS_PointData(RS_Vector(0, 0)));
            break;
        case DPI::LINE:
            entity = new RS_Line{parent, {}, {}};
            break;
        case DPI::CIRCLE:
            entity = new RS_Circle(parent, RS_CircleData());
            break;
        case DPI::ARC:
            entity = new RS_Arc(parent, RS_ArcData());
            break;
        case DPI::ELLIPSE:
            entity = new RS_Ellipse{parent,
                                    {{0., 0.}, {0., 0.}, 0., 0., 0., false}};
            break;
        case DPI::IMAGE:
            entity = new RS_Image(parent, RS_ImageData());
            break;
        case DPI::MTEXT:
            entity = new RS_MText(parent, RS_MTextData());
            break;
        case DPI::TEXT:
            entity = new RS_Text(parent, RS_TextData());
            break;
        case DPI::POLYLINE:
            entity = new RS_Polyline(parent, RS_PolylineData());
            break;
        default:
            break;
    }
}

PluginEntity::~PluginEntity() {
    if (!hasContainer)
        delete entity;
}

void PluginEntity::getData(QHash<int, QVariant> *data) {
    if (!entity) return;
    RS2::EntityType et = entity->rtti();
    data->insert(DPI::EID, (qulonglong) entity->getId());
    data->insert(DPI::LAYER, entity->getLayer()->getName());
    data->insert(DPI::LTYPE, getConverter().lt2str(entity->getPen(false).getLineType()));
    data->insert(DPI::LWIDTH, getConverter().lw2str(entity->getPen(false).getWidth()));
    data->insert(DPI::COLOR, entity->getPen(false).getColor().toIntColor());
    data->insert(DPI::VISIBLE, (entity->isVisible()) ? 1 : 0);
    switch (et) {
        //atomicEntity
        case RS2::EntityLine: {
            data->insert(DPI::ETYPE, DPI::LINE);
            RS_LineData d = static_cast<RS_Line *>(entity)->getData();
            data->insert(DPI::STARTX, d._startpoint.x);
            data->insert(DPI::STARTY, d._startpoint.y);
            data->insert(DPI::ENDX, d._endpoint.x);
            data->insert(DPI::ENDY, d._endpoint.y);
            break;
        }
        case RS2::EntityPoint: {
            data->insert(DPI::ETYPE, DPI::POINT);
            RS_PointData d = static_cast<RS_Point *>(entity)->getData();
            data->insert(DPI::STARTX, d.pos.x);
            data->insert(DPI::STARTY, d.pos.y);
            break;
        }
        case RS2::EntityArc: {
            data->insert(DPI::ETYPE, DPI::ARC);
            RS_ArcData d = static_cast<RS_Arc *>(entity)->getData();
            data->insert(DPI::STARTX, d.center.x);
            data->insert(DPI::STARTY, d.center.y);
            data->insert(DPI::RADIUS, d.radius);
            data->insert(DPI::STARTANGLE, d.angle1);
            data->insert(DPI::ENDANGLE, d.angle2);
            data->insert(DPI::REVERSED, d.reversed);
            break;
        }
        case RS2::EntityCircle: {
            data->insert(DPI::ETYPE, DPI::CIRCLE);
            RS_CircleData d = static_cast<RS_Circle *>(entity)->getData();
            data->insert(DPI::STARTX, d.center.x);
            data->insert(DPI::STARTY, d.center.y);
            data->insert(DPI::RADIUS, d.radius);
            break;
        }
        case RS2::EntityEllipse: {
            data->insert(DPI::ETYPE, DPI::ELLIPSE);
//        RS_EllipseData d = static_cast<RS_Ellipse*>(entity)->getData();
            RS_Ellipse *dd = static_cast<RS_Ellipse *>(entity);
            data->insert(DPI::STARTX, dd->getCenter().x);//10
            data->insert(DPI::STARTY, dd->getCenter().y);//20
            data->insert(DPI::ENDX, dd->getMajorP().x);//11 endpoint major axis x
            data->insert(DPI::ENDY, dd->getMajorP().y);//21 endpoint major axis y
            data->insert(DPI::HEIGHT, dd->getRatio());//40 major/minor axis ratio
            data->insert(DPI::STARTANGLE, dd->getAngle1());
            data->insert(DPI::ENDANGLE, dd->getAngle2());
            data->insert(DPI::REVERSED, dd->isReversed());
            break;
        }
        case RS2::EntitySolid: //TODO
            //Only used in dimensions ?
            data->insert(DPI::ETYPE, DPI::SOLID);
            break;
        case RS2::EntityConstructionLine:
            //Unused ?
            data->insert(DPI::ETYPE, DPI::CONSTRUCTIONLINE);
            break;
        case RS2::EntityImage: {
            data->insert(DPI::ETYPE, DPI::IMAGE);
            RS_ImageData d = static_cast<RS_Image *>(entity)->getData();
            data->insert(DPI::STARTX, d.insertionPoint.x);
            data->insert(DPI::STARTY, d.insertionPoint.y);
            data->insert(DPI::ENDX, d.uVector.x);
            data->insert(DPI::ENDY, d.uVector.y);
            data->insert(DPI::VVECTORX, d.vVector.x);
            data->insert(DPI::VVECTORY, d.vVector.y);
            data->insert(DPI::SIZEU, d.size.x);
            data->insert(DPI::SIZEV, d.size.y);
            data->insert(DPI::BLKNAME, d.file);
            break;
        }
        case RS2::EntityOverlayBox:
            //Unused ?
            data->insert(DPI::ETYPE, DPI::OVERLAYBOX);
            break;
//EntityContainer
        case RS2::EntityInsert: {
            data->insert(DPI::ETYPE, DPI::INSERT);
            RS_InsertData d = static_cast<RS_Insert *>(entity)->getData();
            data->insert(DPI::STARTX, d.insertionPoint.x);
            data->insert(DPI::STARTY, d.insertionPoint.y);
            data->insert(DPI::BLKNAME, d.name);
            data->insert(DPI::STARTANGLE, d.angle);
            data->insert(DPI::XSCALE, d.scaleFactor.x);
            data->insert(DPI::YSCALE, d.scaleFactor.y);
            break;
        }
        case RS2::EntityMText: {
            data->insert(DPI::ETYPE, DPI::MTEXT);
            RS_MTextData d = static_cast<RS_MText *>(entity)->getData();
            data->insert(DPI::STARTX, d.insertionPoint.x);
            data->insert(DPI::STARTY, d.insertionPoint.y);
            data->insert(DPI::STARTANGLE, d.angle);
            data->insert(DPI::HEIGHT, d.height);
            data->insert(DPI::TEXTCONTENT, d.text);
            break;
        }
        case RS2::EntityText: {
            data->insert(DPI::ETYPE, DPI::TEXT);
            RS_TextData d = static_cast<RS_Text *>(entity)->getData();
            data->insert(DPI::STARTX, d.insertionPoint.x);
            data->insert(DPI::STARTY, d.insertionPoint.y);
            data->insert(DPI::STARTANGLE, d.angle);
            data->insert(DPI::HEIGHT, d.height);
            data->insert(DPI::TEXTCONTENT, d.text);
            break;
        }
        case RS2::EntityHatch:
            data->insert(DPI::ETYPE, DPI::HATCH);
            break;
        case RS2::EntitySpline:
            data->insert(DPI::ETYPE, DPI::SPLINE);
            break;
        case RS2::EntitySplinePoints:
            data->insert(DPI::ETYPE, DPI::SPLINEPOINTS);
            break;
        case RS2::EntityPolyline:
            data->insert(DPI::ETYPE, DPI::POLYLINE);
            data->insert(DPI::CLOSEPOLY, static_cast<RS_Polyline *>(entity)->isClosed());
            break;
        case RS2::EntityVertex:
            data->insert(DPI::ETYPE, DPI::UNKNOWN);
            break;
        case RS2::EntityDimAligned:
            data->insert(DPI::ETYPE, DPI::DIMALIGNED);
            break;
        case RS2::EntityDimLinear:
            data->insert(DPI::ETYPE, DPI::DIMLINEAR);
            break;
        case RS2::EntityDimRadial:
            data->insert(DPI::ETYPE, DPI::DIMRADIAL);
            break;
        case RS2::EntityDimDiametric:
            data->insert(DPI::ETYPE, DPI::DIMDIAMETRIC);
            break;
        case RS2::EntityDimAngular:
            data->insert(DPI::ETYPE, DPI::DIMANGULAR);
            break;
        case RS2::EntityDimLeader:
            data->insert(DPI::ETYPE, DPI::DIMLEADER);
            break;
        case RS2::EntityUnknown:
        default:
            data->insert(DPI::ETYPE, DPI::UNKNOWN);
            break;
    }
}

void PluginEntity::updateData(QHash<int, QVariant> *data) {
    if (!entity) return;
    RS_Entity *ec = entity;
    if (hasContainer && dpi) {
        ec = entity->clone();
    }
    QHash<int, QVariant> hash = *data;
    QString str;
    RS_Vector vec;
    RS_Pen epen = ec->getPen();
//    double num;
    if (hash.contains(DPI::LAYER)) {
        str = (hash.take(DPI::LAYER)).toString();
        ec->setLayer(str);
    }
    if (hash.contains(DPI::LTYPE)) {
        str = (hash.take(DPI::LTYPE)).toString();
        epen.setLineType(getConverter().str2lt(str));
    }
    if (hash.contains(DPI::LWIDTH)) {
        str = (hash.take(DPI::LWIDTH)).toString();
        epen.setWidth(getConverter().str2lw(str));
    }
    if (hash.contains(DPI::COLOR)) {
        int co = hash.take(DPI::COLOR).toInt();
        RS_Color color;// = hash.take(DPI::COLOR).value<QColor>();
        color.fromIntColor(co);
        epen.setColor(color);
    }
    ec->setPen(epen);

    RS2::EntityType et = ec->rtti();
    switch (et) {
        //atomicEntity
        case RS2::EntityLine: {
            vec = static_cast<RS_Line *>(ec)->getStartpoint();
            if (hash.contains(DPI::STARTX)) {
                vec.x = (hash.take(DPI::STARTX)).toDouble();
            }
            if (hash.contains(DPI::STARTY)) {
                vec.y = (hash.take(DPI::STARTY)).toDouble();
            }
            static_cast<RS_Line *>(ec)->setStartpoint(vec);
            vec = static_cast<RS_Line *>(ec)->getEndpoint();
            if (hash.contains(DPI::ENDX)) {
                vec.x = (hash.take(DPI::ENDX)).toDouble();
            }
            if (hash.contains(DPI::ENDY)) {
                vec.y = (hash.take(DPI::ENDY)).toDouble();
            }
            static_cast<RS_Line *>(ec)->setEndpoint(vec);
            break;
        }
        case RS2::EntityPoint: {
            vec = static_cast<RS_Point *>(ec)->getPos();
            if (hash.contains(DPI::STARTX)) {
                vec.x = (hash.take(DPI::STARTX)).toDouble();
            }
            if (hash.contains(DPI::STARTY)) {
                vec.y = (hash.take(DPI::STARTY)).toDouble();
            }
            static_cast<RS_Point *>(ec)->setPos(vec);
            break;
        }
        case RS2::EntityArc: {
            RS_Arc *arc = static_cast<RS_Arc *>(ec);
            vec = arc->getCenter();
            if (hash.contains(DPI::STARTX)) {
                vec.x = (hash.take(DPI::STARTX)).toDouble();
            }
            if (hash.contains(DPI::STARTY)) {
                vec.y = (hash.take(DPI::STARTY)).toDouble();
            }
            arc->setCenter(vec);
            if (hash.contains(DPI::RADIUS)) {
                arc->setRadius((hash.take(DPI::RADIUS)).toDouble());
            }
            if (hash.contains(DPI::STARTANGLE)) {
                arc->setAngle1((hash.take(DPI::STARTANGLE)).toDouble());
                vec.y = (hash.take(DPI::STARTANGLE)).toDouble();
            }
            if (hash.contains(DPI::ENDANGLE)) {
                arc->setAngle2((hash.take(DPI::ENDANGLE)).toDouble());
            }
            break;
        }
        case RS2::EntityCircle: {
            RS_Circle *cir = static_cast<RS_Circle *>(ec);
            vec = cir->getCenter();
            if (hash.contains(DPI::STARTX)) {
                vec.x = (hash.take(DPI::STARTX)).toDouble();
            }
            if (hash.contains(DPI::STARTY)) {
                vec.y = (hash.take(DPI::STARTY)).toDouble();
            }
            cir->setCenter(vec);
            if (hash.contains(DPI::RADIUS)) {
                cir->setRadius((hash.take(DPI::RADIUS)).toDouble());
            }
            break;
        }
        case RS2::EntityEllipse: {
            RS_Ellipse *ellipse = static_cast<RS_Ellipse *>(ec);
            vec = ellipse->getCenter();
            if (hash.contains(DPI::STARTX)) {
                vec.x = (hash.take(DPI::STARTX)).toDouble();
            }
            if (hash.contains(DPI::STARTY)) {
                vec.y = (hash.take(DPI::STARTY)).toDouble();
            }
            ellipse->setCenter(vec);

            vec = ellipse->getMajorP();
            if (hash.contains(DPI::ENDX)) {
                vec.x = (hash.take(DPI::ENDX)).toDouble();
            }
            if (hash.contains(DPI::ENDY)) {
                vec.y = (hash.take(DPI::ENDY)).toDouble();
            }
            ellipse->setMajorP(vec);

            if (hash.contains(DPI::STARTANGLE)) {
                ellipse->setAngle1((hash.take(DPI::STARTANGLE)).toDouble());
            }
            if (hash.contains(DPI::ENDANGLE)) {
                ellipse->setAngle2((hash.take(DPI::ENDANGLE)).toDouble());
            }
            if (hash.contains(DPI::HEIGHT)) {
                ellipse->setRatio((hash.take(DPI::HEIGHT)).toDouble());
            }
            if (hash.contains(DPI::REVERSED)) {
                ellipse->setReversed((hash.take(DPI::REVERSED)).toBool());
            }
            break;
        }
        case RS2::EntitySolid: //TODO
            //Only used in dimensions ?
            break;
        case RS2::EntityConstructionLine:
            //Unused ?
            break;
        case RS2::EntityImage: {
            RS_Image *img = static_cast<RS_Image *>(ec);
            vec = img->getInsertionPoint();
            if (hash.contains(DPI::STARTX)) {
                vec.x = (hash.take(DPI::STARTX)).toDouble();
            }
            if (hash.contains(DPI::STARTY)) {
                vec.y = (hash.take(DPI::STARTY)).toDouble();
            }
            img->setInsertionPoint(vec);
            if (hash.contains(DPI::BLKNAME)) {
                img->setFile((hash.take(DPI::BLKNAME)).toString());
            }
            vec = img->getUVector();
            RS_Vector vec2 = img->getVVector();
            RS_Vector vec3(img->getWidth(), img->getHeight());
            if (hash.contains(DPI::ENDX)) {
                vec.x = (hash.take(DPI::ENDX)).toDouble();
            }
            if (hash.contains(DPI::ENDY)) {
                vec.y = (hash.take(DPI::ENDY)).toDouble();
            }
            if (hash.contains(DPI::VVECTORX)) {
                vec2.x = (hash.take(DPI::VVECTORX)).toDouble();
            }
            if (hash.contains(DPI::VVECTORY)) {
                vec2.y = (hash.take(DPI::VVECTORY)).toDouble();
            }
            if (hash.contains(DPI::SIZEU)) {
                vec3.x = (hash.take(DPI::SIZEU)).toDouble();
            }
            if (hash.contains(DPI::SIZEV)) {
                vec3.y = (hash.take(DPI::SIZEV)).toDouble();
            }
            img->updateData(vec3, vec, vec2);
            break;
        }
        case RS2::EntityOverlayBox:
            //Unused ?
            break;
//EntityContainer
        case RS2::EntityInsert: {
            break;
        }
        case RS2::EntityMText: {
            RS_MText *txt = static_cast<RS_MText *>(ec);
            bool move = false;
            vec = txt->getInsertionPoint();
            if (hash.contains(DPI::STARTX)) {
                vec.x = (hash.take(DPI::STARTX)).toDouble() - vec.x;
                move = true;
            } else vec.x = 0;
            if (hash.contains(DPI::STARTY)) {
                vec.y = (hash.take(DPI::STARTY)).toDouble() - vec.y;
                move = true;
            } else vec.y = 0;
            if (move)
                txt->move(vec);
            if (hash.contains(DPI::TEXTCONTENT)) {
                txt->setText((hash.take(DPI::TEXTCONTENT)).toString());
            }
            if (hash.contains(DPI::STARTANGLE)) {
                txt->setAngle((hash.take(DPI::STARTANGLE)).toDouble());
            }
            if (hash.contains(DPI::HEIGHT)) {
                txt->setHeight((hash.take(DPI::HEIGHT)).toDouble());
            }
            break;
        }
        case RS2::EntityText: {
            RS_Text *txt = static_cast<RS_Text *>(ec);
            bool move = false;
            vec = txt->getInsertionPoint();
            if (hash.contains(DPI::STARTX)) {
                vec.x = (hash.take(DPI::STARTX)).toDouble() - vec.x;
                move = true;
            } else vec.x = 0;
            if (hash.contains(DPI::STARTY)) {
                vec.y = (hash.take(DPI::STARTY)).toDouble() - vec.y;
                move = true;
            } else vec.y = 0;
            if (move)
                txt->move(vec);
            if (hash.contains(DPI::TEXTCONTENT)) {
                txt->setText((hash.take(DPI::TEXTCONTENT)).toString());
            }
            if (hash.contains(DPI::STARTANGLE)) {
                txt->setAngle((hash.take(DPI::STARTANGLE)).toDouble());
            }
            if (hash.contains(DPI::HEIGHT)) {
                txt->setHeight((hash.take(DPI::HEIGHT)).toDouble());
            }
            break;
        }
        case RS2::EntityHatch:
            break;
        case RS2::EntitySpline:
            break;
        case RS2::EntityPolyline: {
            RS_Polyline *pl = static_cast<RS_Polyline *>(ec);
            if (hash.take(DPI::CLOSEPOLY).toBool()) {
                pl->setClosedFlag(true);
            } else {
                pl->setClosedFlag(false);
            }
            break;
        }
        case RS2::EntityVertex:
            break;
        case RS2::EntityDimAligned:
            break;
        case RS2::EntityDimLinear:
            break;
        case RS2::EntityDimRadial:
            break;
        case RS2::EntityDimDiametric:
            break;
        case RS2::EntityDimAngular:
            break;
        case RS2::EntityDimLeader:
            break;
        case RS2::EntityUnknown:
        default:
            break;
    }
    ec->update();
    if (hasContainer && dpi)
        this->dpi->updateEntity(entity, ec);
}

void PluginEntity::getPolylineData(QList<Plug_VertexData> *data) {
    if (!entity) return;
    RS2::EntityType et = entity->rtti();
    if (et != RS2::EntityPolyline) return;
    RS_Polyline *l = static_cast<RS_Polyline *>(entity);

    RS_Entity *nextEntity = 0;
    RS_AtomicEntity *ae = nullptr;
    RS_Entity *v = l->firstEntity(RS2::ResolveNone);
    double bulge = 0.0;
//bad polyline without vertex
    if (!v) return;

//First polyline vertex
    if (v->rtti() == RS2::EntityArc) {
        bulge = ((RS_Arc *) v)->getBulge();
    }
    ae = (RS_AtomicEntity *) v;
    data->append(Plug_VertexData(QPointF(ae->getStartpoint().x,
                                         ae->getStartpoint().y), bulge));

    for (v = l->firstEntity(RS2::ResolveNone); v; v = nextEntity) {
        nextEntity = l->nextEntity(RS2::ResolveNone);
        bulge = 0.0;
        if (!v->isAtomic()) {
            continue;
        }
        ae = (RS_AtomicEntity *) v;

        if (nextEntity) {
            if (nextEntity->rtti() == RS2::EntityArc) {
                bulge = ((RS_Arc *) nextEntity)->getBulge();
            }
        }

        if (!l->isClosed() || nextEntity) {
            data->append(Plug_VertexData(QPointF(ae->getEndpoint().x,
                                                 ae->getEndpoint().y), bulge));
        }
    }
}

void PluginEntity::updatePolylineData(QList<Plug_VertexData> *data) {
    if (!entity) return;
    RS2::EntityType et = entity->rtti();
    if (et != RS2::EntityPolyline) return;
    if (data->size() < 2) return; //At least two vertex
    RS_Vector vec(false);
    RS_Polyline *pl = static_cast<RS_Polyline *>(entity);
//    vec.x = data->at(0).point.x();
//    vec.y = data->at(0).point.y();
    pl->clear();
    pl->setEndpoint(vec);
    pl->setStartpoint(vec);
    vec._valid = true;
    for (int i = 0; i < data->size(); ++i) {
        vec.x = data->at(i).point.x();
        vec.y = data->at(i).point.y();
        pl->addVertex(vec, data->at(i).bulge);
    }


}

void PluginEntity::move(QPointF offset) {
    RS_Entity *ne = entity->clone();
    ne->move(RS_Vector(offset.x(), offset.y()));
    bool ok = dpi->addToUndo(entity, ne);
    //if doc interface fails to handle for undo only modify original entity
    if (!ok) {
        entity->move(RS_Vector(offset.x(), offset.y()));
        delete ne;
    } else
        this->entity = ne;
}

void PluginEntity::moveRotate(QPointF const &offset, QPointF const &center, double angle) {
    RS_Entity *ne = entity->clone();
    ne->move(RS_Vector(offset.x(), offset.y()));
    ne->rotate(RS_Vector(center.x(), center.y()), angle);
    bool ok = dpi->addToUndo(entity, ne);
    //if doc interface fails to handle for undo only modify original entity
    if (!ok) {
        entity->move(RS_Vector(offset.x(), offset.y()));
        entity->rotate(RS_Vector(center.x(), center.y()), angle);
        delete ne;
    } else
        this->entity = ne;
}

void PluginEntity::rotate(QPointF center, double angle) {
    RS_Entity *ne = entity->clone();
    ne->rotate(RS_Vector(center.x(), center.y()), angle);
    bool ok = dpi->addToUndo(entity, ne);
    //if doc interface fails to handle for undo only modify original entity
    if (!ok) {
        entity->rotate(RS_Vector(center.x(), center.y()), angle);
        delete ne;
    } else
        this->entity = ne;
}

void PluginEntity::scale(QPointF center, QPointF factor) {
    RS_Entity *ne = entity->clone();
    ne->scale(RS_Vector(center.x(), center.y()),
              RS_Vector(factor.x(), factor.y()));
    bool ok = dpi->addToUndo(entity, ne);
    //if doc interface fails to handle for undo only modify original entity
    if (!ok) {
        entity->scale(RS_Vector(center.x(), center.y()),
                      RS_Vector(factor.x(), factor.y()));
        delete ne;
    } else
        this->entity = ne;
}

QString PluginEntity::intColor2str(int color) {
    return getConverter().intColor2str(color);
}
