/* -*- Mode: C++; c-default-style: "k&r"; indent-tabs-mode: nil; tab-width: 2; c-basic-offset: 2 -*- */
/* libstaroffice
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2002 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2002 Marc Maurer (uwog@uwog.net)
 *
 * For minor contributions see the git repository.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU Lesser General Public License Version 2.1 or later
 * (LGPLv2.1+), in which case the provisions of the LGPLv2.1+ are
 * applicable instead of those above.
 *
 * For further information visit http://libstoff.sourceforge.net
 */

#ifndef LIBSTAROFFICE_HXX
#define LIBSTAROFFICE_HXX

/** Defines the chart possible conversion:
    - 1: try to retrieve the graphic representation of the chart when
         a chart is in some defined document(from libstoff-0.0.6)
 */
#define STOFF_CHART_VERSION 1
/** Defines the database possible conversion (actually none) */
#define STOFF_DATABASE_VERSION 0
/** Defines the vector graphic possible conversion
    - 1: begin to convert some .sda and some .sdg file +
         add STOFF_K_GRAPHIC enum(from libstoff-0.0.1)
    - 2: can generate some graphic OLE(from libstoff-0.0.2)
*/
#define STOFF_GRAPHIC_VERSION 2
/** Defines the math possible conversion :
    - 1: try to retrieve the graphic representation of the chart when
         a chart is in some defined document(from libstoff-0.0.6)*/
#define STOFF_MATH_VERSION 1
/** Defines the presentation possible conversion
    - 1: begin to convert the files created by StarImpress
         in presentation(from libstoff-0.0.3)
 */
#define STOFF_PRESENTATION_VERSION 1
/** Defines the spreadsheet processing possible conversion:
    - 1: can convert some basic document(from libstoff-0.0.0)
    - 2: can generate some spreadsheet OLE(from libstoff-0.0.2)
 */
#define STOFF_SPREADSHEET_VERSION 2
/** Defines the word processing possible conversion:
    - 1: can convert some sdw files(from libstoff-0.0.2) */
#define STOFF_TEXT_VERSION 1

#include "STOFFDocument.hxx"

#endif /* LIBSTAROFFICE_HXX */
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
