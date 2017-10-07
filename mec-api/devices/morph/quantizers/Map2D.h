// found easier approach to calculate to get the boundaries of a diatonic piano key without iterating through all keys:
// calculate the closest octave and then only iterate through the keys of that octave (so it's 11 iterations at worst case)
// This should be (almost?) as fast as the 2dmap - retry this if the other variant is too slow

//#ifndef MEC_QUANTIZER_MAP_H
//#define MEC_QUANTIZER_MAP_H
//
//#include <map>
//#include <vector>
//#include "Rectangle.h"
//#include "Quantizer.h"
//
//namespace mec {
//namespace morph {
//
//// Retrieves the quantizer that surrounds the provided touch.
//// Holds a vector of quantizers for each cellWidth x cellHeight sector of the entire area, so the list of quantizer
//// candidates the touch might be in is very short when chosing a matching cell size
//class Map2D {
//public:
//    Map2D(const unsigned int &cellWidth, const unsigned int &cellHeight);
//    void insert(std::shared_ptr<Rectangle>& boundary);
//    const std::shared_ptr<Rectangle> getSurroundingRectangle(const Touch &touch) const;
//private:
//    unsigned int cellWidth_;
//    unsigned int cellHeight_;
//    std::map<unsigned long, std::shared_ptr<std::vector<std::shared_ptr<Rectangle>>>> boundariesStorage_;
//
//    void add(const unsigned long &key, const std::shared_ptr<Rectangle> &boundary);
//};
//
//}
//}
//
//
//#endif //MEC_QUANTIZER_MAP_H
