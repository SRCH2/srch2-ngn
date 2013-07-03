//$Id: LocationRecordUtil.h 3181 2013-02-06 02:10:06Z oliverax $

/*
 * The Software is made available solely for use according to the License Agreement. Any reproduction
 * or redistribution of the Software not in accordance with the License Agreement is expressly prohibited
 * by law, and may result in severe civil and criminal penalties. Violators will be prosecuted to the
 * maximum extent possible.
 *
 * THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE AGREEMENT. EXCEPT
 * AS WARRANTED IN THE LICENSE AGREEMENT, SRCH2 INC. HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS WITH
 * REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT.  IN NO EVENT SHALL SRCH2 INC. BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF SOFTWARE.

 * Copyright © 2010 SRCH2 Inc. All rights reserved
 */

#ifndef LOCATIONRECORDUTIL_H_
#define LOCATIONRECORDUTIL_H_

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <math.h>
#include <vector>

namespace bimaple
{
namespace instantsearch
{


class Point
{
public:
    double x;
    double y;

    Point()
    {
        x = 0;
        y = 0;
    }

    bool operator==(const Point &point) const
    {
        return x == point.x && y == point.y;
    };

    double distSquare(const Point &point) const
    {
        return (this->x - point.x) * (this->x - point.x)
            + (this->y - point.y) * (this->y - point.y);
    }

private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & this->x;
        ar & this->y;
    }
};

class Rectangle;

class Shape
{
public:
    Shape() {};
    virtual ~Shape() {};

    virtual bool contains(const Point &point) const = 0;
    virtual bool intersects(const Rectangle &) const = 0;
    virtual double getMinDist2FromLatLong(double resultLat, double resultLng) const = 0;
    virtual double getSearchRadius2() const = 0;
    virtual void getValues(std::vector<double> &values) const = 0;
};

class Rectangle : public Shape
{
public:
    // the mbr's lower value
    Point min;
    // the mbr's upper value
    Point max;

    Rectangle()
    {
        this->min.x = 0;
        this->min.y = 0;
        this->max.x = 0;
        this->max.y = 0;
    }

    Rectangle(const std::pair<std::pair<double,double>, std::pair<double,double> >& rect)
    {
        this->min.x = rect.first.first;
        this->min.y = rect.first.second;
        this->max.x = rect.second.first;
        this->max.y = rect.second.second;
    }

    virtual ~Rectangle() {}

    // check whether two rectangles intersect or not
    virtual bool intersects(const Rectangle &rect) const
    {
        return min.x <= rect.max.x && rect.min.x <= max.x
            && min.y <= rect.max.y && rect.min.y <= max.y;
    };

    // check whether the rectangle contains the point
    virtual bool contains(const Point &point) const
    {
        return min.x <= point.x && max.x >= point.x
            && min.y <= point.y && max.y >= point.y;
    };

    // check whether this rectangle is contained by another
    bool containedBy(const Rectangle &rect) const
    {
        return min.x >= rect.min.x && min.y >= rect.min.y
            && max.x <= rect.max.x && max.y <= rect.max.y;
    };

    bool operator==(const Rectangle &r) const
    {
            return min == r.min && max == r.max;
    };

    virtual double getMinDist2FromLatLong(double resultLat, double resultLng) const
    {
        double rangeMidLat = (this->max.x + this->min.x) / 2.0;
        double rangeMidLng = (this->max.y + this->min.y) / 2.0;
        return pow((resultLat - rangeMidLat), 2) + pow((resultLng - rangeMidLng), 2);
    }

    virtual double getSearchRadius2() const
    {
        return pow((this->max.x - this->min.x)/2.0, 2) + pow((this->max.y - this->min.y)/2.0, 2);
    }

    virtual void getValues(std::vector<double> &values) const
    {
        values.push_back(this->min.x);
        values.push_back(this->min.y);
        values.push_back(this->max.x);
        values.push_back(this->max.y);
    }

private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & this->min;
        ar & this->max;
    }
};

class Circle : public Shape
{
private:
    Point center;
    double radius;
public:
    Circle(Point c, double r) : center(c), radius(r) {}

    virtual ~Circle() {}

    virtual bool contains(const Point &point) const
    {
        return center.distSquare(point) <= radius*radius;
    }

    // how to detect the intersection between a circle and a rectangle
    // http://stackoverflow.com/a/402010/1407156
    virtual bool intersects(const Rectangle &rect) const
    {
        double rectX = (rect.min.x + rect.max.x) / 2;
        double rectY = (rect.min.y + rect.max.y) / 2;
        double width = rect.max.x - rect.min.x;
        double height = rect.max.y - rect.min.y;

        double distanceX = abs(center.x - rectX);
        double distanceY = abs(center.y - rectY);

        if (distanceX > (width/2 + radius)) { return false; }
        if (distanceY > (height/2 + radius)) { return false; }

        if (distanceX <= (width/2)) { return true; } 
        if (distanceY <= (height/2)) { return true; }

        double cornerDistance_sq = (distanceX - width/2) * (distanceX - width/2) +
                                   (distanceY - height/2) * (distanceY - height/2);

        return (cornerDistance_sq <= (radius*radius));
    }

    virtual double getMinDist2FromLatLong(double resultLat, double resultLng) const
    {
        return pow((resultLat - center.x), 2) + pow((resultLng - center.y), 2);
    }

    virtual double getSearchRadius2() const
    {
        return pow(radius, 2);
    }

    virtual void getValues(std::vector<double> &values) const
    {
        values.push_back(center.x);
        values.push_back(center.y);
        values.push_back(radius);
    }

};

}}

#endif /* LOCATIONRECORDUTIL_H_ */
