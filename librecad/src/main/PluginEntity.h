//
// Created by hawk on 26.12.21.
//

#ifndef LIBRECAD_PLUGINENTITY_H
#define LIBRECAD_PLUGINENTITY_H

#include <qvariant.h>
#include "rs_entity.h"
#include "DocumentPluginImplementation.h"

class PluginEntity {
public:
    PluginEntity(RS_Entity *ent, DocumentPluginImplementation *d);

    PluginEntity(RS_EntityContainer *parent, enum DPI::ETYPE type);

    virtual ~PluginEntity();

    bool isValid() { if (entity) return true; else return false; }

    RS_Entity *getEnt() { return entity; }

    virtual void getData(QHash<int, QVariant> *data);

    virtual void updateData(QHash<int, QVariant> *data);

    virtual void getPolylineData(QList<Plug_VertexData> *data);

    virtual void updatePolylineData(QList<Plug_VertexData> *data);

    virtual void move(QPointF offset);

    virtual void moveRotate(QPointF const &offset, QPointF const &center, double angle);

    virtual void rotate(QPointF center, double angle);

    virtual void scale(QPointF center, QPointF factor);

    virtual QString intColor2str(int color);

private:
    RS_Entity *entity;
    bool hasContainer;
    DocumentPluginImplementation *dpi;
};


#endif //LIBRECAD_PLUGINENTITY_H
