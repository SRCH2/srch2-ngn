#include "LocationRecordUtil.h"

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
	buffer = srch2::util::serializeFixedTypes(getShapeType() , buffer);
	switch (getShapeType()) {
		case Shape::TypeRectangle:
			Rectangle * thisRect = (Rectangle *)this;
			buffer = srch2::util::serializeFixedTypes(thisRect->min.x , buffer);
			buffer = srch2::util::serializeFixedTypes(thisRect->min.y , buffer);
			buffer = srch2::util::serializeFixedTypes(thisRect->max.x , buffer);
			buffer = srch2::util::serializeFixedTypes(thisRect->max.y , buffer);
			return buffer;
		case Shape::TypeCircle:
			Circle * thisCircle = (Circle *)this;
			buffer = srch2::util::serializeFixedTypes(thisCircle->getCenter().x , buffer);
			buffer = srch2::util::serializeFixedTypes(thisCircle->getCenter().y , buffer);
			buffer = srch2::util::serializeFixedTypes(thisCircle->getRadius() , buffer);
			return buffer;
	}

}
/*
 * Serialization Scheme:
 * | ShapeType | ... |
 * | ShapeType == Rectangle | minX | minY | maxX | maxY |
 * | ShapeType == Circle | centerX | centerY | radius |
 */
void * Shape::deserializeForNetwork(Shape * &shape, void * buffer){
	ShapeType type ;
   	buffer = srch2::util::deserializeFixedTypes(buffer, type);
   	switch (type) {
		case Shape::TypeRectangle:
			Rectangle * rect = new Rectangle();
			buffer = srch2::util::deserializeFixedTypes(buffer, rect->min.x);
			buffer = srch2::util::deserializeFixedTypes(buffer, rect->min.y);
			buffer = srch2::util::deserializeFixedTypes(buffer, rect->max.x );
			buffer = srch2::util::deserializeFixedTypes(buffer, rect->max.y );
			shape = rect;
			return buffer;
		case Shape::TypeCircle:
			Circle * circle = new Circle(Point(), 0);
			buffer = srch2::util::deserializeFixedTypes(buffer, circle->getCenter().x);
			buffer = srch2::util::deserializeFixedTypes(buffer, circle->getCenter().y);
			buffer = srch2::util::deserializeFixedTypes(buffer, circle->getRadius());
			shape = circle;
			return buffer;
	}
}
/*
 * Serialization Scheme:
 * | ShapeType | ... |
 * | ShapeType == Rectangle | minX | minY | maxX | maxY |
 * | ShapeType == Circle | centerX | centerY | radius |
 */
unsigned Shape::getNumberOfBytesForSerializationForNetwork(){
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(ShapeType);
	switch (getShapeType()) {
		case Shape::TypeRectangle:
			Rectangle * thisRect = (Rectangle *)this;
			numberOfBytes += sizeof(thisRect->min.x);
			numberOfBytes += sizeof(thisRect->min.y);
			numberOfBytes += sizeof(thisRect->max.x);
			numberOfBytes += sizeof(thisRect->max.y);
			return numberOfBytes;
		case Shape::TypeCircle:
			Circle * thisCircle = (Circle *)this;
			numberOfBytes += sizeof(thisCircle->getCenter().x);
			numberOfBytes += sizeof(thisCircle->getCenter().y);
			numberOfBytes += sizeof(thisCircle->getRadius());
			return numberOfBytes;
	}
}

}
}
