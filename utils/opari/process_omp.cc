/*************************************************************************/
/* OPARI Version 1.1                                                     */
/* Copyright (c) 2001-2005                                                    */
/* Forschungszentrum Juelich, Zentralinstitut fuer Angewandte Mathematik */
/*************************************************************************/

#include <iostream>
#ifdef EBUG
   using std::cerr;

#  include <iomanip>
   using std::setw;
# endif

#include "opari.h"
#include "handler.h"

void process_pragma(OMPragma* p, ostream& os, bool* hasEnd, bool* isFor) {
# ifdef EBUG
  for (unsigned i=0; i<p->lines.size(); ++i)
    cerr << setw(3) << p->lineno+i << ":O" << (i?"+":" ")
         << ": " << p->lines[i] << "\n";
# endif
  p->find_name();

  if ( do_transform || p->name == "instrument" ) {
    if ( hasEnd )
      *hasEnd = (p->name != "barrier" && p->name != "noinstrument" &&
                 p->name != "flush"   && p->name != "threadprivate" &&
#if defined(__GNUC__) && (__GNUC__ < 3)
                 p->name.substr(0, 4) != "inst");
#else
                 p->name.compare(0, 4, "inst") != 0);
#endif
    if ( isFor  )
      *isFor  = (p->name == "for" || p->name == "parallelfor");
    phandler_t handler = find_handler(p->name);
    handler(p, os);
  } else {
    for (unsigned i=0; i<p->lines.size(); ++i) os << p->lines[i] << "\n";
  }
  delete p;
}
