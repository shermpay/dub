#ifndef DUB_FORM_FORMAT_H
#define DUB_FORM_FORMAT_H

#include "src/form.h"

#include "llvm/Support/FormatVariadicDetails.h"

namespace llvm {

template <> struct format_provider<dub::Nil> {
  static void format(const dub::Nil &_, raw_ostream &stream, StringRef style) {
    (void)style;
    stream << "nil";
  }
};

template <> struct format_provider<dub::Form> {
  static void format(const dub::Form &form, raw_ostream &stream,
                     StringRef style);
};



template <> struct format_provider<dub::List> {
  static void format(const dub::List &list, raw_ostream &stream,
                     StringRef style);
};


template <> struct format_provider<dub::Vector> {
  static void format(const dub::Vector &vec, raw_ostream &stream,
                     StringRef style);
};


} // namespace llvm

#endif  // DUB_FORM_FORMAT_H
