/* -*- Mode: C++; c-default-style: "k&r"; indent-tabs-mode: nil; tab-width: 2; c-basic-offset: 2 -*- */

/* libstaroffice
* Version: MPL 2.0 / LGPLv2+
*
* The contents of this file are subject to the Mozilla Public License Version
* 2.0 (the "License"); you may not use this file except in compliance with
* the License or as specified alternatively below. You may obtain a copy of
* the License at http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
* for the specific language governing rights and limitations under the
* License.
*
* Major Contributor(s):
* Copyright (C) 2002 William Lachance (wrlach@gmail.com)
* Copyright (C) 2002,2004 Marc Maurer (uwog@uwog.net)
* Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
* Copyright (C) 2006, 2007 Andrew Ziem
* Copyright (C) 2011, 2012 Alonso Laurent (alonso@loria.fr)
*
*
* All Rights Reserved.
*
* For minor contributions see the git repository.
*
* Alternatively, the contents of this file may be used under the terms of
* the GNU Lesser General Public License Version 2 or later (the "LGPLv2+"),
* in which case the provisions of the LGPLv2+ are applicable
* instead of those above.
*/

#ifndef LIBSTAROFFICE_INTERNAL_H
#define LIBSTAROFFICE_INTERNAL_H

#include <assert.h>
#include <math.h>
#ifdef DEBUG
#include <stdio.h>
#endif

#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <librevenge-stream/librevenge-stream.h>
#include <librevenge/librevenge.h>

#if defined(_MSC_VER) || defined(__DJGPP__)

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;
typedef __int64 int64_t;

#else /* !_MSC_VER && !__DJGPP__*/

#  ifdef HAVE_CONFIG_H

#    include <config.h>
#    ifdef HAVE_STDINT_H
#      include <stdint.h>
#    endif
#    ifdef HAVE_INTTYPES_H
#      include <inttypes.h>
#    endif

#  else

// assume that the headers are there inside LibreOffice build when no HAVE_CONFIG_H is defined
#    include <stdint.h>
#    include <inttypes.h>

#  endif

#endif /* _MSC_VER || __DJGPP__ */

// define gmtime_r and localtime_r on Windows, so that can use
// thread-safe functions on other environments
#ifdef _WIN32
#  define gmtime_r(tp,tmp) (gmtime(tp)?(*(tmp)=*gmtime(tp),(tmp)):0)
#  define localtime_r(tp,tmp) (localtime(tp)?(*(tmp)=*localtime(tp),(tmp)):0)
#endif

/** an noop deleter used to transform a libwpd pointer in a false std::shared_ptr */
template <class T>
struct STOFF_shared_ptr_noop_deleter {
  void operator()(T *) {}
};

/** fall through attributes */
#if defined(HAVE_CLANG_ATTRIBUTE_FALLTHROUGH)
#  define STOFF_FALLTHROUGH [[clang::fallthrough]]
#elif defined(HAVE_GCC_ATTRIBUTE_FALLTHROUGH)
#  define STOFF_FALLTHROUGH __attribute__((fallthrough))
#else
#  define STOFF_FALLTHROUGH ((void) 0)
#endif

#if defined(HAVE_FUNC_ATTRIBUTE_FORMAT)
#  define LIBSTOFF_ATTRIBUTE_PRINTF(fmt, arg) __attribute__((format(printf, fmt, arg)))
#else
#  define LIBSTOFF_ATTRIBUTE_PRINTF(fmt, arg)
#endif

#define STOFF_N_ELEMENTS(m) sizeof(m)/sizeof(m[0])

/* ---------- debug  --------------- */
#ifdef DEBUG
namespace libstoff
{
void printDebugMsg(const char *format, ...) LIBSTOFF_ATTRIBUTE_PRINTF(1,2);
}
#define STOFF_DEBUG_MSG(M) libstoff::printDebugMsg M
#else
#define STOFF_DEBUG_MSG(M)
#endif

namespace libstoff
{
// Various exceptions:
class VersionException
{
};

class FileException
{
};

class ParseException
{
};

class GenericException
{
};

class WrongPasswordException
{
};
}

/* ---------- input ----------------- */
namespace libstoff
{
uint8_t readU8(librevenge::RVNGInputStream *input);
//! adds an unicode character to a string
void appendUnicode(uint32_t val, librevenge::RVNGString &buffer);
//! transform a unicode string in a RNVGString
librevenge::RVNGString getString(std::vector<uint32_t> const &unicode);

//! checks whether addition of \c x and \c y would overflow
template<typename T>
bool checkAddOverflow(T x, T y)
{
  return (x < 0 && y < std::numeric_limits<T>::lowest() - x)
         || (x > 0 && y > std::numeric_limits<T>::max() - x);
}
}

/* ---------- small enum/class ------------- */
namespace libstoff
{
//! basic position enum
enum Position { Left = 0, Right = 1, Top = 2, Bottom = 3, HMiddle = 4, VMiddle = 5 };
//! basic position enum bits
enum { LeftBit = 0x01,  RightBit = 0x02, TopBit=0x4, BottomBit = 0x08, HMiddleBit = 0x10, VMiddleBit = 0x20 };

enum NumberingType { NONE, BULLET, ARABIC, LOWERCASE, UPPERCASE, LOWERCASE_ROMAN, UPPERCASE_ROMAN };
std::string numberingTypeToString(NumberingType type);
std::string numberingValueToString(NumberingType type, int value);
enum SubDocumentType { DOC_NONE, DOC_CHART, DOC_CHART_ZONE, DOC_COMMENT_ANNOTATION, DOC_GRAPHIC_GROUP, DOC_HEADER_FOOTER_REGION, DOC_NOTE, DOC_SHEET, DOC_TABLE, DOC_TEXT_BOX };
}

//! the class to store a color
struct STOFFColor {
  //! constructor
  explicit STOFFColor(uint32_t argb=0) : m_value(argb)
  {
  }
  //! constructor from color
  STOFFColor(unsigned char r, unsigned char g,  unsigned char b, unsigned char a=255) :
    m_value(uint32_t((a<<24)+(r<<16)+(g<<8)+b))
  {
  }
  //! operator=
  STOFFColor &operator=(uint32_t argb)
  {
    m_value = argb;
    return *this;
  }
  //! return a color from a cmyk color ( basic)
  static STOFFColor colorFromCMYK(unsigned char c, unsigned char m,  unsigned char y, unsigned char k)
  {
    double w=1.-double(k)/255.;
    return STOFFColor
           (static_cast<unsigned char>(255 * (1-double(c)/255) * w),
            static_cast<unsigned char>(255 * (1-double(m)/255) * w),
            static_cast<unsigned char>(255 * (1-double(y)/255) * w)
           );
  }
  //! return a color from a hsl color (basic)
  static STOFFColor colorFromHSL(unsigned char H, unsigned char S,  unsigned char L)
  {
    double c=(1-((L>=128) ? (2*double(L)-255) : (255-2*double(L)))/255)*
             double(S)/255;
    double tmp=std::fmod((double(H)*6/255),2)-1;
    double x=c*(1-(tmp>0 ? tmp : -tmp));
    auto C=static_cast<unsigned char>(255*c);
    auto M=static_cast<unsigned char>(double(L)-255*c/2);
    auto X=static_cast<unsigned char>(255*x);
    if (H<=42) return STOFFColor(static_cast<unsigned char>(M+C),static_cast<unsigned char>(M+X),static_cast<unsigned char>(M));
    if (H<=85) return STOFFColor(static_cast<unsigned char>(M+X),static_cast<unsigned char>(M+C),static_cast<unsigned char>(M));
    if (H<=127) return STOFFColor(static_cast<unsigned char>(M),static_cast<unsigned char>(M+C),static_cast<unsigned char>(M+X));
    if (H<=170) return STOFFColor(static_cast<unsigned char>(M),static_cast<unsigned char>(M+X),static_cast<unsigned char>(M+C));
    if (H<=212) return STOFFColor(static_cast<unsigned char>(M+X),static_cast<unsigned char>(M),static_cast<unsigned char>(M+C));
    return STOFFColor(static_cast<unsigned char>(M+C),static_cast<unsigned char>(M),static_cast<unsigned char>(M+X));
  }
  //! return the back color
  static STOFFColor black()
  {
    return STOFFColor(0,0,0);
  }
  //! return the white color
  static STOFFColor white()
  {
    return STOFFColor(255,255,255);
  }

  //! return alpha*colA+beta*colB
  static STOFFColor barycenter(float alpha, STOFFColor const &colA,
                               float beta, STOFFColor const &colB);
  //! return the rgba value
  uint32_t value() const
  {
    return m_value;
  }
  //! returns the alpha value
  unsigned char getAlpha() const
  {
    return static_cast<unsigned char>((m_value>>24)&0xFF);
  }
  //! reset the alpha value
  void setAlpha(unsigned char alpha)
  {
    m_value=(m_value&0xFFFFFF)|uint32_t(alpha<<24);
  }
  //! returns the green value
  unsigned char getBlue() const
  {
    return static_cast<unsigned char>(m_value&0xFF);
  }
  //! returns the red value
  unsigned char getRed() const
  {
    return static_cast<unsigned char>((m_value>>16)&0xFF);
  }
  //! returns the green value
  unsigned char getGreen() const
  {
    return static_cast<unsigned char>((m_value>>8)&0xFF);
  }
  //! return true if the color is black
  bool isBlack() const
  {
    return (m_value&0xFFFFFF)==0;
  }
  //! return true if the color is white
  bool isWhite() const
  {
    return (m_value&0xFFFFFF)==0xFFFFFF;
  }
  //! operator==
  bool operator==(STOFFColor const &c) const
  {
    return (c.m_value&0xFFFFFF)==(m_value&0xFFFFFF);
  }
  //! operator!=
  bool operator!=(STOFFColor const &c) const
  {
    return !operator==(c);
  }
  //! operator<
  bool operator<(STOFFColor const &c) const
  {
    return (c.m_value&0xFFFFFF)<(m_value&0xFFFFFF);
  }
  //! operator<=
  bool operator<=(STOFFColor const &c) const
  {
    return (c.m_value&0xFFFFFF)<=(m_value&0xFFFFFF);
  }
  //! operator>
  bool operator>(STOFFColor const &c) const
  {
    return !operator<=(c);
  }
  //! operator>=
  bool operator>=(STOFFColor const &c) const
  {
    return !operator<(c);
  }
  //! operator<< in the form \#rrggbb
  friend std::ostream &operator<< (std::ostream &o, STOFFColor const &c);
  //! print the color in the form \#rrggbb
  std::string str() const;
protected:
  //! the argb color
  uint32_t m_value;
};

//! a border line
struct STOFFBorderLine {
  //! constructor
  STOFFBorderLine() : m_outWidth(0), m_inWidth(0), m_color(STOFFColor::black()), m_distance(0) { }
  /** add the border property to proplist (if needed )

  \note if set which must be equal to "left", "top", ... */
  bool addTo(librevenge::RVNGPropertyList &propList, std::string which="") const;
  //! returns true if the border is empty
  bool isEmpty() const
  {
    return m_outWidth==0 && m_inWidth==0;
  }
  //! operator==
  bool operator==(STOFFBorderLine const &orig) const
  {
    return !operator!=(orig);
  }
  //! operator!=
  bool operator!=(STOFFBorderLine const &orig) const
  {
    return m_outWidth != orig.m_outWidth || m_inWidth != orig.m_inWidth ||
           m_distance != orig.m_distance || m_color != orig.m_color;
  }

  //! operator<<
  friend std::ostream &operator<< (std::ostream &o, STOFFBorderLine const &border);
  //! the outline width in TWIP
  int m_outWidth;
  //! the inline width in TWIP
  int m_inWidth;
  //! the border color
  STOFFColor m_color;
  //! the border distance
  int m_distance;
};

//! a field
struct STOFFField {
  /** basic constructor */
  STOFFField() : m_propertyList()
  {
  }
  /** add the link property to proplist (if possible) */
  void addTo(librevenge::RVNGPropertyList &propList) const;
  //! the property list
  librevenge::RVNGPropertyList m_propertyList;
};

//! a link
struct STOFFLink {
  /** basic constructor */
  STOFFLink() : m_HRef("")
  {
  }

  /** add the link property to proplist (if needed ) */
  bool addTo(librevenge::RVNGPropertyList &propList) const;

  //! the href field
  std::string m_HRef;
};

//! a note
struct STOFFNote {
  //! enum to define note type
  enum Type { FootNote, EndNote };
  //! constructor
  explicit STOFFNote(Type type) : m_type(type), m_label(""), m_number(-1)
  {
  }
  //! the note type
  Type m_type;
  //! the note label
  librevenge::RVNGString m_label;
  //! the note number if defined
  int m_number;
};

/** small class use to define a embedded object

    \note mainly used to store picture
 */
struct STOFFEmbeddedObject {
  //! empty constructor
  STOFFEmbeddedObject() : m_dataList(), m_typeList(),  m_filenameLink("")
  {
  }
  //! constructor
  STOFFEmbeddedObject(librevenge::RVNGBinaryData const &binaryData,
                      std::string const &type="image/pict") : m_dataList(), m_typeList(), m_filenameLink("")
  {
    add(binaryData, type);
  }
  STOFFEmbeddedObject(STOFFEmbeddedObject const &)=default;
  STOFFEmbeddedObject(STOFFEmbeddedObject &&)=default;
  STOFFEmbeddedObject &operator=(STOFFEmbeddedObject const &)=default;
  STOFFEmbeddedObject &operator=(STOFFEmbeddedObject &&)=default;
  //! destructor
  ~STOFFEmbeddedObject();
  //! return true if the picture contains no data
  bool isEmpty() const
  {
    if (!m_filenameLink.empty())
      return false;
    for (const auto &i : m_dataList) {
      if (!i.empty())
        return false;
    }
    return true;
  }
  //! add a picture
  void add(librevenge::RVNGBinaryData const &binaryData, std::string const &type="image/pict")
  {
    size_t pos=m_dataList.size();
    if (pos<m_typeList.size()) pos=m_typeList.size();
    m_dataList.resize(pos+1);
    m_dataList[pos]=binaryData;
    m_typeList.resize(pos+1);
    m_typeList[pos]=type;
  }
  /** add the link property to proplist */
  bool addTo(librevenge::RVNGPropertyList &propList) const;
  /** add the link property to a graph style as bitmap */
  bool addAsFillImageTo(librevenge::RVNGPropertyList &propList) const;
  /** operator<<*/
  friend std::ostream &operator<<(std::ostream &o, STOFFEmbeddedObject const &pict);
  /** a comparison function */
  int cmp(STOFFEmbeddedObject const &pict) const;

  //! the picture content: one data by representation
  std::vector<librevenge::RVNGBinaryData> m_dataList;
  //! the picture type: one type by representation
  std::vector<std::string> m_typeList;
  //! a picture link
  librevenge::RVNGString m_filenameLink;
};

// forward declarations of basic classes and smart pointers
class STOFFFont;
class STOFFFrameStyle;
class STOFFGraphicShape;
class STOFFGraphicStyle;
class STOFFList;
class STOFFParagraph;
class STOFFSection;
class STOFFPageSpan;

class STOFFEntry;
class STOFFHeader;
class STOFFParser;
class STOFFPosition;

class STOFFGraphicListener;
class STOFFInputStream;
class STOFFListener;
class STOFFListManager;
class STOFFParserState;
class STOFFSpreadsheetListener;
class STOFFSubDocument;
class STOFFTextListener;
//! a smart pointer of STOFFGraphicListener
typedef std::shared_ptr<STOFFGraphicListener> STOFFGraphicListenerPtr;
//! a smart pointer of STOFFInputStream
typedef std::shared_ptr<STOFFInputStream> STOFFInputStreamPtr;
//! a smart pointer of STOFFListener
typedef std::shared_ptr<STOFFListener> STOFFListenerPtr;
//! a smart pointer of STOFFListManager
typedef std::shared_ptr<STOFFListManager> STOFFListManagerPtr;
//! a smart pointer of STOFFParserState
typedef std::shared_ptr<STOFFParserState> STOFFParserStatePtr;
//! a smart pointer of STOFFSpreadsheetListener
typedef std::shared_ptr<STOFFSpreadsheetListener> STOFFSpreadsheetListenerPtr;
//! a smart pointer of STOFFSubDocument
typedef std::shared_ptr<STOFFSubDocument> STOFFSubDocumentPtr;
//! a smart pointer of STOFFTextListener
typedef std::shared_ptr<STOFFTextListener> STOFFTextListenerPtr;

/** a generic variable template: value + flag to know if the variable is set

\note the variable is considered set as soon a new value is set or
when its content is acceded by a function which returns a not-const
reference... You can use the function setSet to unset it.
*/
template <class T> struct STOFFVariable {
  //! constructor
  STOFFVariable() : m_data(), m_set(false) {}
  //! constructor with a default value
  explicit STOFFVariable(T const &def) : m_data(def), m_set(false) {}
  //! copy constructor
  STOFFVariable(STOFFVariable const &orig) : m_data(orig.m_data), m_set(orig.m_set) {}
  //! copy operator
  STOFFVariable &operator=(STOFFVariable const &orig)
  {
    if (this != &orig) {
      m_data = orig.m_data;
      m_set = orig.m_set;
    }
    return std::forward<STOFFVariable &>(*this);
  }
  //! set a value
  STOFFVariable &operator=(T const &val)
  {
    m_data = val;
    m_set = true;
    return std::forward<STOFFVariable &>(*this);
  }
  //! update the current value if orig is set
  void insert(STOFFVariable const &orig)
  {
    if (orig.m_set) {
      m_data = orig.m_data;
      m_set = orig.m_set;
    }
  }
  //! operator*
  T const *operator->() const
  {
    return &m_data;
  }
  /** operator* */
  T *operator->()
  {
    m_set = true;
    return &m_data;
  }
  //! operator*
  T const &operator*() const
  {
    return m_data;
  }
  //! operator*
  T &operator*()
  {
    m_set = true;
    return m_data;
  }
  //! return the current value
  T const &get() const
  {
    return m_data;
  }
  //! return true if the variable is set
  bool isSet() const
  {
    return m_set;
  }
  //! define if the variable is set
  void setSet(bool newVal)
  {
    m_set=newVal;
  }
protected:
  //! the value
  T m_data;
  //! a flag to know if the variable is set or not
  bool m_set;
};

/* ---------- vec2/box2f ------------- */
/*! \class STOFFVec2
 *   \brief small class which defines a vector with 2 elements
 */
template <class T> class STOFFVec2
{
public:
  //! constructor
  STOFFVec2(T xx=0,T yy=0) : m_x(xx), m_y(yy) { }
  //! generic copy constructor
  template <class U> STOFFVec2(STOFFVec2<U> const &p) : m_x(T(p.x())), m_y(T(p.y())) {}

  //! first element
  T x() const
  {
    return m_x;
  }
  //! second element
  T y() const
  {
    return m_y;
  }
  //! operator[]
  T operator[](int c) const
  {
    if (c<0 || c>1) throw libstoff::GenericException();
    return (c==0) ? m_x : m_y;
  }
  //! operator[]
  T &operator[](int c)
  {
    if (c<0 || c>1) throw libstoff::GenericException();
    return (c==0) ? m_x : m_y;
  }

  //! resets the two elements
  void set(T xx, T yy)
  {
    m_x = xx;
    m_y = yy;
  }
  //! resets the first element
  void setX(T xx)
  {
    m_x = xx;
  }
  //! resets the second element
  void setY(T yy)
  {
    m_y = yy;
  }

  //! increases the actuals values by \a dx and \a dy
  void add(T dx, T dy)
  {
    if (libstoff::checkAddOverflow(m_x, dx) || libstoff::checkAddOverflow(m_y, dy))
      throw libstoff::GenericException();
    m_x += dx;
    m_y += dy;
  }

  //! operator+=
  STOFFVec2<T> &operator+=(STOFFVec2<T> const &p)
  {
    add(p.m_x, p.m_y);
    return *this;
  }
  //! operator-=
  STOFFVec2<T> &operator-=(STOFFVec2<T> const &p)
  {
    // check if negation of either of the coords will cause overflow
    const T diff = std::numeric_limits<T>::min() + std::numeric_limits<T>::max();
    if (libstoff::checkAddOverflow(p.m_x, diff) || libstoff::checkAddOverflow(p.m_y, diff))
      throw libstoff::GenericException();
    add(-p.m_x, -p.m_y);
    return *this;
  }
  //! generic operator*=
  template <class U>
  STOFFVec2<T> &operator*=(U scale)
  {
    m_x = T(m_x*scale);
    m_y = T(m_y*scale);
    return *this;
  }

  //! operator+
  friend STOFFVec2<T> operator+(STOFFVec2<T> const &p1, STOFFVec2<T> const &p2)
  {
    STOFFVec2<T> p(p1);
    return p+=p2;
  }
  //! operator-
  friend STOFFVec2<T> operator-(STOFFVec2<T> const &p1, STOFFVec2<T> const &p2)
  {
    STOFFVec2<T> p(p1);
    return p-=p2;
  }
  //! generic operator*
  template <class U>
  friend STOFFVec2<T> operator*(U scale, STOFFVec2<T> const &p1)
  {
    STOFFVec2<T> p(p1);
    return p *= scale;
  }

  //! comparison==
  bool operator==(STOFFVec2<T> const &p) const
  {
    return cmpY(p) == 0;
  }
  //! comparison!=
  bool operator!=(STOFFVec2<T> const &p) const
  {
    return cmpY(p) != 0;
  }
  //! comparison<: sort by y
  bool operator<(STOFFVec2<T> const &p) const
  {
    return cmpY(p) < 0;
  }
  //! a comparison function: which first compares x then y
  int cmp(STOFFVec2<T> const &p) const
  {
    if (m_x < p.m_x) return -1;
    if (m_x > p.m_x) return 1;
    if (m_y < p.m_y) return -1;
    if (m_y > p.m_y) return 1;
    return 0;
  }
  //! a comparison function: which first compares y then x
  int cmpY(STOFFVec2<T> const &p) const
  {
    if (m_y < p.m_y) return -1;
    if (m_y > p.m_y) return 1;
    if (m_x < p.m_x) return -1;
    if (m_x > p.m_x) return 1;
    return 0;
  }

  //! operator<<: prints data in form "XxY"
  friend std::ostream &operator<< (std::ostream &o, STOFFVec2<T> const &f)
  {
    o << f.m_x << "x" << f.m_y;
    return o;
  }

  /*! \struct PosSizeLtX
   * \brief internal struct used to create sorted map, sorted by X
   */
  struct PosSizeLtX {
    //! comparaison function
    bool operator()(STOFFVec2<T> const &s1, STOFFVec2<T> const &s2) const
    {
      return s1.cmp(s2) < 0;
    }
  };
  /*! \typedef MapX
   *  \brief map of STOFFVec2
   */
  typedef std::map<STOFFVec2<T>, T,struct PosSizeLtX> MapX;

  /*! \struct PosSizeLtY
   * \brief internal struct used to create sorted map, sorted by Y
   */
  struct PosSizeLtY {
    //! comparaison function
    bool operator()(STOFFVec2<T> const &s1, STOFFVec2<T> const &s2) const
    {
      return s1.cmpY(s2) < 0;
    }
  };
  /*! \typedef MapY
   *  \brief map of STOFFVec2
   */
  typedef std::map<STOFFVec2<T>, T,struct PosSizeLtY> MapY;
protected:
  T m_x/*! \brief first element */, m_y/*! \brief second element */;
};

/*! \brief STOFFVec2 of bool */
typedef STOFFVec2<bool> STOFFVec2b;
/*! \brief STOFFVec2 of int */
typedef STOFFVec2<int> STOFFVec2i;
/*! \brief STOFFVec2 of long */
typedef STOFFVec2<long> STOFFVec2l;
/*! \brief STOFFVec2 of float */
typedef STOFFVec2<float> STOFFVec2f;

/*! \class STOFFVec3
 *   \brief small class which defines a vector with 3 elements
 */
template <class T> class STOFFVec3
{
public:
  //! constructor
  STOFFVec3(T xx=0,T yy=0,T zz=0)
  {
    m_val[0] = xx;
    m_val[1] = yy;
    m_val[2] = zz;
  }
  //! generic copy constructor
  template <class U> STOFFVec3(STOFFVec3<U> const &p)
  {
    for (int c = 0; c < 3; c++) m_val[c] = T(p[c]);
  }

  //! first element
  T x() const
  {
    return m_val[0];
  }
  //! second element
  T y() const
  {
    return m_val[1];
  }
  //! third element
  T z() const
  {
    return m_val[2];
  }
  //! operator[]
  T operator[](int c) const
  {
    if (c<0 || c>2) throw libstoff::GenericException();
    return m_val[c];
  }
  //! operator[]
  T &operator[](int c)
  {
    if (c<0 || c>2) throw libstoff::GenericException();
    return m_val[c];
  }

  //! resets the three elements
  void set(T xx, T yy, T zz)
  {
    m_val[0] = xx;
    m_val[1] = yy;
    m_val[2] = zz;
  }
  //! resets the first element
  void setX(T xx)
  {
    m_val[0] = xx;
  }
  //! resets the second element
  void setY(T yy)
  {
    m_val[1] = yy;
  }
  //! resets the third element
  void setZ(T zz)
  {
    m_val[2] = zz;
  }

  //! increases the actuals values by \a dx, \a dy, \a dz
  void add(T dx, T dy, T dz)
  {
    m_val[0] += dx;
    m_val[1] += dy;
    m_val[2] += dz;
  }

  //! operator+=
  STOFFVec3<T> &operator+=(STOFFVec3<T> const &p)
  {
    for (int c = 0; c < 3; c++) m_val[c] = T(m_val[c]+p.m_val[c]);
    return *this;
  }
  //! operator-=
  STOFFVec3<T> &operator-=(STOFFVec3<T> const &p)
  {
    for (int c = 0; c < 3; c++) m_val[c] = T(m_val[c]-p.m_val[c]);
    return *this;
  }
  //! generic operator*=
  template <class U>
  STOFFVec3<T> &operator*=(U scale)
  {
    for (auto &c : m_val) c = T(c*scale);
    return *this;
  }

  //! operator+
  friend STOFFVec3<T> operator+(STOFFVec3<T> const &p1, STOFFVec3<T> const &p2)
  {
    STOFFVec3<T> p(p1);
    return p+=p2;
  }
  //! operator-
  friend STOFFVec3<T> operator-(STOFFVec3<T> const &p1, STOFFVec3<T> const &p2)
  {
    STOFFVec3<T> p(p1);
    return p-=p2;
  }
  //! generic operator*
  template <class U>
  friend STOFFVec3<T> operator*(U scale, STOFFVec3<T> const &p1)
  {
    STOFFVec3<T> p(p1);
    return p *= scale;
  }

  //! comparison==
  bool operator==(STOFFVec3<T> const &p) const
  {
    return cmp(p) == 0;
  }
  //! comparison!=
  bool operator!=(STOFFVec3<T> const &p) const
  {
    return cmp(p) != 0;
  }
  //! comparison<: which first compares x values, then y values then z values.
  bool operator<(STOFFVec3<T> const &p) const
  {
    return cmp(p) < 0;
  }
  //! a comparison function: which first compares x values, then y values then z values.
  int cmp(STOFFVec3<T> const &p) const
  {
    for (int c = 0; c < 3; c++) {
      if (m_val[c]<p.m_val[c]) return -1;
      if (m_val[c]>p.m_val[c]) return 1;
    }
    return 0;
  }

  //! operator<<: prints data in form "XxYxZ"
  friend std::ostream &operator<< (std::ostream &o, STOFFVec3<T> const &f)
  {
    o << f.m_val[0] << "x" << f.m_val[1] << "x" << f.m_val[2];
    return o;
  }

  /*! \struct PosSizeLt
   * \brief internal struct used to create sorted map, sorted by X, Y, Z
   */
  struct PosSizeLt {
    //! comparaison function
    bool operator()(STOFFVec3<T> const &s1, STOFFVec3<T> const &s2) const
    {
      return s1.cmp(s2) < 0;
    }
  };
  /*! \typedef Map
   *  \brief map of STOFFVec3
   */
  typedef std::map<STOFFVec3<T>, T,struct PosSizeLt> Map;

protected:
  //! the values
  T m_val[3];
};

/*! \brief STOFFVec3 of bool */
typedef STOFFVec3<bool> STOFFVec3b;
/*! \brief STOFFVec3 of unsigned char */
typedef STOFFVec3<unsigned char> STOFFVec3uc;
/*! \brief STOFFVec3 of int */
typedef STOFFVec3<int> STOFFVec3i;
/*! \brief STOFFVec3 of float */
typedef STOFFVec3<float> STOFFVec3f;

/*! \class STOFFBox2
 *   \brief small class which defines a 2D Box
 */
template <class T> class STOFFBox2
{
public:
  //! constructor
  STOFFBox2(STOFFVec2<T> minPt=STOFFVec2<T>(), STOFFVec2<T> maxPt=STOFFVec2<T>())
  {
    m_pt[0] = minPt;
    m_pt[1] = maxPt;
  }
  //! generic constructor
  template <class U> STOFFBox2(STOFFBox2<U> const &p)
  {
    for (int c=0; c < 2; c++) m_pt[c] = p[c];
  }

  //! the minimum 2D point (in x and in y)
  STOFFVec2<T> const &min() const
  {
    return m_pt[0];
  }
  //! the maximum 2D point (in x and in y)
  STOFFVec2<T> const &max() const
  {
    return m_pt[1];
  }
  //! the minimum 2D point (in x and in y)
  STOFFVec2<T> &min()
  {
    return m_pt[0];
  }
  //! the maximum 2D point (in x and in y)
  STOFFVec2<T> &max()
  {
    return m_pt[1];
  }
  /*! \brief the two extremum points which defined the box
   * \param c 0 means the minimum and 1 the maximum
   */
  STOFFVec2<T> const &operator[](int c) const
  {
    if (c<0 || c>1) throw libstoff::GenericException();
    return m_pt[c];
  }
  //! the box size
  STOFFVec2<T> size() const
  {
    return m_pt[1]-m_pt[0];
  }
  //! the box center
  STOFFVec2<T> center() const
  {
    return STOFFVec2<T>((m_pt[0].x()+m_pt[1].x())/2,
                        (m_pt[0].y()+m_pt[1].y())/2);
  }

  //! resets the data to minimum \a x and maximum \a y
  void set(STOFFVec2<T> const &x, STOFFVec2<T> const &y)
  {
    m_pt[0] = x;
    m_pt[1] = y;
  }
  //! resets the minimum point
  void setMin(STOFFVec2<T> const &x)
  {
    m_pt[0] = x;
  }
  //! resets the maximum point
  void setMax(STOFFVec2<T> const &y)
  {
    m_pt[1] = y;
  }

  //!  resize the box keeping the minimum
  void resizeFromMin(STOFFVec2<T> const &sz)
  {
    m_pt[1] = m_pt[0]+sz;
  }
  //!  resize the box keeping the maximum
  void resizeFromMax(STOFFVec2<T> const &sz)
  {
    m_pt[0] = m_pt[1]-sz;
  }
  //!  resize the box keeping the center
  void resizeFromCenter(STOFFVec2<T> const &sz)
  {
    STOFFVec2<T> centerPt = 0.5*(m_pt[0]+m_pt[1]);
    m_pt[0] = centerPt - 0.5*sz;
    m_pt[1] = centerPt + (sz - 0.5*sz);
  }

  //! scales all points of the box by \a factor
  template <class U> void scale(U factor)
  {
    m_pt[0] *= factor;
    m_pt[1] *= factor;
  }

  //! extends the bdbox by (\a val, \a val) keeping the center
  void extend(T val)
  {
    m_pt[0] -= STOFFVec2<T>(val/2,val/2);
    m_pt[1] += STOFFVec2<T>(val-(val/2),val-(val/2));
  }

  //! returns the union between this and box
  STOFFBox2<T> getUnion(STOFFBox2<T> const &box) const
  {
    STOFFBox2<T> res;
    res.m_pt[0]=STOFFVec2<T>(m_pt[0][0]<box.m_pt[0][0]?m_pt[0][0] : box.m_pt[0][0],
                             m_pt[0][1]<box.m_pt[0][1]?m_pt[0][1] : box.m_pt[0][1]);
    res.m_pt[1]=STOFFVec2<T>(m_pt[1][0]>box.m_pt[1][0]?m_pt[1][0] : box.m_pt[1][0],
                             m_pt[1][1]>box.m_pt[1][1]?m_pt[1][1] : box.m_pt[1][1]);
    return res;
  }
  //! returns the intersection between this and box
  STOFFBox2<T> getIntersection(STOFFBox2<T> const &box) const
  {
    STOFFBox2<T> res;
    res.m_pt[0]=STOFFVec2<T>(m_pt[0][0]>box.m_pt[0][0]?m_pt[0][0] : box.m_pt[0][0],
                             m_pt[0][1]>box.m_pt[0][1]?m_pt[0][1] : box.m_pt[0][1]);
    res.m_pt[1]=STOFFVec2<T>(m_pt[1][0]<box.m_pt[1][0]?m_pt[1][0] : box.m_pt[1][0],
                             m_pt[1][1]<box.m_pt[1][1]?m_pt[1][1] : box.m_pt[1][1]);
    return res;
  }
  //! comparison operator==
  bool operator==(STOFFBox2<T> const &p) const
  {
    return cmp(p) == 0;
  }
  //! comparison operator!=
  bool operator!=(STOFFBox2<T> const &p) const
  {
    return cmp(p) != 0;
  }
  //! comparison operator< : fist sorts min by Y,X values then max extremity
  bool operator<(STOFFBox2<T> const &p) const
  {
    return cmp(p) < 0;
  }

  //! comparison function : fist sorts min by Y,X values then max extremity
  int cmp(STOFFBox2<T> const &p) const
  {
    int diff  = m_pt[0].cmpY(p.m_pt[0]);
    if (diff) return diff;
    diff  = m_pt[1].cmpY(p.m_pt[1]);
    if (diff) return diff;
    return 0;
  }

  //! print data in form X0xY0<->X1xY1
  friend std::ostream &operator<< (std::ostream &o, STOFFBox2<T> const &f)
  {
    o << "(" << f.m_pt[0] << "<->" << f.m_pt[1] << ")";
    return o;
  }

  /*! \struct PosSizeLt
   * \brief internal struct used to create sorted map, sorted first min then max
   */
  struct PosSizeLt {
    //! comparaison function
    bool operator()(STOFFBox2<T> const &s1, STOFFBox2<T> const &s2) const
    {
      return s1.cmp(s2) < 0;
    }
  };
  /*! \typedef Map
   *  \brief map of STOFFBox2
   */
  typedef std::map<STOFFBox2<T>, T,struct PosSizeLt> Map;

protected:
  //! the two extremities
  STOFFVec2<T> m_pt[2];
};

/*! \brief STOFFBox2 of int */
typedef STOFFBox2<int> STOFFBox2i;
/*! \brief STOFFBox2 of float */
typedef STOFFBox2<float> STOFFBox2f;
/*! \brief STOFFBox2 of long */
typedef STOFFBox2<long> STOFFBox2l;

namespace libstoff
{
//! convert a date/time in a date time format
bool convertToDateTime(uint32_t date, uint32_t time, std::string &dateTime);
//! split a string in two. If the delimiter is not present, string1=string
void splitString(librevenge::RVNGString const &string, librevenge::RVNGString const &delim,
                 librevenge::RVNGString &string1, librevenge::RVNGString &string2);
/** returns a simplify version of a string.

 \note this function is mainly used to try to test for searching a string when some encoding problem has happens*/
librevenge::RVNGString simplifyString(librevenge::RVNGString const &s);
//! returns the cell name corresponding to a cell's position
std::string getCellName(STOFFVec2i const &cellPos, STOFFVec2b const &relative=STOFFVec2b(true,true));
// some geometrical function
//! factor to convert from one unit to other
float getScaleFactor(librevenge::RVNGUnit orig, librevenge::RVNGUnit dest);
//! rotate a point around center, angle is given in degree
STOFFVec2f rotatePointAroundCenter(STOFFVec2f const &point, STOFFVec2f const &center, float angle);
//! rotate a bdox and returns the final bdbox, angle is given in degree
STOFFBox2f rotateBoxFromCenter(STOFFBox2f const &box, float angle);
}
#endif /* LIBSTAROFFICE_INTERNAL_H */
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
