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

/*
 * Parser to convert a SdrModel zone/OLE in a StarOffice document
 *
 */
#ifndef STAR_OBJECT_MODEL
#  define STAR_OBJECT_MODEL

#include <ostream>
#include <set>
#include <vector>

#include "libstaroffice_internal.hxx"
#include "StarObject.hxx"

namespace StarObjectModelInternal
{
class Layer;
class LayerSet;
class Page;
struct State;
}

class StarState;
class StarZone;

/** \brief the main class to read a SdrModel zone
 *
 *
 *
 */
class StarObjectModel : public StarObject
{
public:
  //! constructor
  StarObjectModel(StarObject const &orig, bool duplicateState);
  //! destructor
  ~StarObjectModel() override;
  //! try to read a SdrModel zone: "DrMd"
  bool read(StarZone &zone);
  /** try to update the object id of page 0

      \note this is used to retrieve an object in a .sdw's DrawingLawer stream
   */
  void updateObjectIds(std::set<long> &unusedId);
  /** try to update the page span (to create draw document)*/
  bool updatePageSpans(std::vector<STOFFPageSpan> &pageSpan, int &numPages, bool usePage0=false) const;
  //! try to send the master pages
  bool sendMasterPages(STOFFGraphicListenerPtr listener);
  //! try to send the different page
  bool sendPages(STOFFListenerPtr listener);
  //! try to send a page content
  bool sendPage(int pageId, STOFFListenerPtr listener, bool masterPage=false);
  //! try to send an object
  bool sendObject(int id, STOFFListenerPtr listener, StarState const &state);

  //! small operator<< to print the content of the model
  friend std::ostream &operator<<(std::ostream &o, StarObjectModel const &model);
protected:
  //
  // low level
  //
  //! try to read a SdrLayer zone: "DrLy'
  bool readSdrLayer(StarZone &zone, StarObjectModelInternal::Layer &layer);
  //! try to read a SdrLayerSet zone: "DrLS'
  bool readSdrLayerSet(StarZone &zone, StarObjectModelInternal::LayerSet &layers);
  //! try to read a Page/MasterPage zone: "DrPg'
  std::shared_ptr<StarObjectModelInternal::Page> readSdrPage(StarZone &zone);
  /* try to read a Master Page descriptor zone: "DrMP' and add it the master page descriptor */
  bool readSdrMPageDesc(StarZone &zone, StarObjectModelInternal::Page &page);
  /* try to read a list of Master Page zone: "DrML' and add them in the master page descriptors */
  bool readSdrMPageDescList(StarZone &zone,  StarObjectModelInternal::Page &page);
  //! try to read a zone which appear at end of a zone: "DrPg'
  bool readSdrPageUnknownZone1(StarZone &zone, long lastPos);


protected:
  //
  // data
  //

  //! the state
  std::shared_ptr<StarObjectModelInternal::State> m_modelState;
private:
  StarObjectModel &operator=(StarObjectModel const &orig) = delete;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
