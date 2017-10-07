//#include <map>
//#include <vector>
//#include "../../../mec_api.h"
//#include "Rectangle.h"
//#include "Map2D.h"
//
//namespace mec {
//namespace morph {
//
//
//Map2D::Map2D(const unsigned int &cellWidth, const unsigned int &cellHeight) : cellWidth_(cellWidth), cellHeight_(cellHeight) {
//}
//
//void Map2D::insert(std::shared_ptr<Rectangle>& boundary) {
//    unsigned int leftX = boundary->x / cellWidth_;
//    unsigned int rightX = (boundary->x + boundary->width) / cellWidth_;
//    unsigned int upperY = boundary->y / cellHeight_;
//    unsigned int lowerY = (boundary->y + boundary->height) / cellHeight_;
//    unsigned long upperLeftKey = leftX * 17 + upperY * 27;
//    unsigned long upperRightKey = rightX * 17 + upperY * 27;
//    unsigned long lowerLeftKey = leftX * 17 + lowerY * 27;
//    unsigned long lowerRightKey = rightX * 17 + lowerY * 27;
//    add(upperLeftKey, boundary);
//    if(upperRightKey != upperLeftKey) {
//        add(upperRightKey, boundary);
//    }
//    if(lowerLeftKey != upperLeftKey && lowerLeftKey != upperRightKey) {
//        add(lowerLeftKey, boundary);
//    }
//    if(lowerRightKey != upperLeftKey && lowerRightKey != upperRightKey && lowerRightKey != lowerLeftKey) {
//        add(lowerLeftKey, boundary);
//    }
//}
//
//const std::shared_ptr<Rectangle> Map2D::getSurroundingRectangle(const Touch &touch) const {
//    unsigned long key = touch.x_ * 17 + touch.y_ * 27;
//    auto vectorIter = boundariesStorage_.find(key);
//    if(vectorIter != boundariesStorage_.end()) {
//        for (auto boundariesPtrIter = vectorIter->second->begin();
//             boundariesPtrIter != vectorIter->second->end(); ++boundariesPtrIter) {
//            auto boundary = *boundariesPtrIter;
//            if (touch.x_ > boundary->x && touch.x_ < boundary->x + boundary->width
//                && touch.y_ > boundary->y && touch.y_ < boundary->y + boundary->height) {
//                return boundary;
//            }
//        }
//    }
//    return std::shared_ptr<Rectangle>();
//}
//
//void Map2D::add(const unsigned long &key, const std::shared_ptr<Rectangle> &boundary) {
//    std::shared_ptr<std::vector<std::shared_ptr<Rectangle>>> vectorPtr;
//    auto vectorIter = boundariesStorage_.find(key);
//    if(vectorIter == boundariesStorage_.end()) {
//        vectorPtr.reset(new std::vector<std::shared_ptr<Rectangle>>());
//        boundariesStorage_[key] = vectorPtr;
//    } else {
//        vectorPtr = vectorIter->second;
//    }
//    vectorPtr->push_back(boundary);
//}
//
//}
//}
