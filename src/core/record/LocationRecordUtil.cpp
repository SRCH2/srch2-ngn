#include "LocationRecordUtil.h"
#include "util/SerializationHelper.h"
#include "util/Assert.h"

namespace srch2
{
namespace instantsearch
{

/*
 * Serialization Scheme:
 * | ShapeType | ... |
 * | ShapeType == Rectangle | minX | minY | maxX | maxY |
 * | ShapeType == Circle | centerX | centerY | radius |
 */
void * Shape::serializeForNetwork(void * buffer){
    buffer = srch2::util::serializeFixedTypes((uint32_t)getShapeType() , buffer);
    switch (getShapeType()) {
    case Shape::TypeRectangle: {
        Rectangle * thisRect = (Rectangle *)this;
        buffer = srch2::util::serializeFixedTypes(thisRect->min.x , buffer);
        buffer = srch2::util::serializeFixedTypes(thisRect->min.y , buffer);
        buffer = srch2::util::serializeFixedTypes(thisRect->max.x , buffer);
        buffer = srch2::util::serializeFixedTypes(thisRect->max.y , buffer);
        return buffer;
    } case Shape::TypeCircle:
    {
        Circle * thisCircle = (Circle *)this;
        buffer = srch2::util::serializeFixedTypes(thisCircle->getCenter().x , buffer);
        buffer = srch2::util::serializeFixedTypes(thisCircle->getCenter().y , buffer);
        buffer = srch2::util::serializeFixedTypes(thisCircle->getRadius() , buffer);
        return buffer;
    }
    }

    ASSERT(false);
    return NULL;
}
/*
 * Serialization Scheme:
 * | ShapeType | ... |
 * | ShapeType == Rectangle | minX | minY | maxX | maxY |
 * | ShapeType == Circle | centerX | centerY | radius |
 */
void * Shape::deserializeForNetwork(Shape * &shape, void * buffer){
    ShapeType type ;
    uint32_t intVar = 0;
    buffer = srch2::util::deserializeFixedTypes(buffer, intVar);
    type = (ShapeType)intVar;
    switch (type) {
    case Shape::TypeRectangle: {
        Rectangle * rect = new Rectangle();
        buffer = srch2::util::deserializeFixedTypes(buffer, rect->min.x);
        buffer = srch2::util::deserializeFixedTypes(buffer, rect->min.y);
        buffer = srch2::util::deserializeFixedTypes(buffer, rect->max.x );
        buffer = srch2::util::deserializeFixedTypes(buffer, rect->max.y );
        shape = rect;
        return buffer;
    } case Shape::TypeCircle: {
        Circle * circle = new Circle(Point(), 0);
        buffer = srch2::util::deserializeFixedTypes(buffer, circle->getCenter().x);
        buffer = srch2::util::deserializeFixedTypes(buffer, circle->getCenter().y);
        buffer = srch2::util::deserializeFixedTypes(buffer, circle->getRadius());
        shape = circle;
        return buffer;
    }
    }
    ASSERT(false);
    return NULL;
}
/*
 * Serialization Scheme:
 * | ShapeType | ... |
 * | ShapeType == Rectangle | minX | minY | maxX | maxY |
 * | ShapeType == Circle | centerX | centerY | radius |
 */
unsigned Shape::getNumberOfBytesForSerializationForNetwork(){
    unsigned numberOfBytes = 0;
    numberOfBytes += srch2::util::getNumberOfBytesFixedTypes((uint32_t)getShapeType());
    switch (getShapeType()) {
    case Shape::TypeRectangle: {
        Rectangle * thisRect = (Rectangle *)this;
        numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(thisRect->min.x);
        numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(thisRect->min.y);
        numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(thisRect->max.x);
        numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(thisRect->max.y);
        return numberOfBytes;
    }    case Shape::TypeCircle: {
        Circle * thisCircle = (Circle *)this;
        numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(thisCircle->getCenter().x);
        numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(thisCircle->getCenter().y);
        numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(thisCircle->getRadius());
        return numberOfBytes;
    }
    }
    ASSERT(false);
    return 0;
}

}
}
