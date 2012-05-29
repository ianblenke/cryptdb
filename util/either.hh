#pragma once

#include <cassert>

template <typename Left, typename Right>
class either {
public:
  /** Left constructor */
  either(const Left& leftItem) :
    _left(true), leftItem(leftItem) {}
  /** Right constructor */
  either(const Right& rightItem) :
    _left(false), rightItem(rightItem) {}

  inline bool isLeft() const { return _left; }
  inline bool isRight() const { return !_left; }

  inline Left& left() { assert(isLeft()); return leftItem; }
  inline const Left& left() const { assert(isLeft()); return leftItem; }

  inline Right& right() { assert(isRight()); return rightItem; }
  inline const Right& right() const { assert(isRight()); return rightItem; }

protected:
  bool _left;
  Left leftItem;
  Right rightItem;
};
