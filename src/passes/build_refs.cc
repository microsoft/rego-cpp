#include "lang.h"
#include "passes.h"

namespace
{
  using namespace rego;

  const inline auto RefHeadToken = T(Var) / T(Array) / T(Object) / T(Set) /
    T(ArrayCompr) / T(ObjectCompr) / T(SetCompr) / T(ExprCall);
}

namespace rego
{
  PassDef build_refs()
  {
    return {
      In(Group) * (RefHeadToken[RefHead] * T(Dot) * T(Var)[Rhs]) >>
        [](Match& _) {
          return Ref << (RefHead << _(RefHead))
                     << (RefArgSeq << (RefArgDot << _(Rhs)));
        },

      In(Group) * (RefHeadToken[RefHead] * T(Array)[Array]) >>
        [](Match& _) {
          return Ref << (RefHead << _(RefHead))
                     << (RefArgSeq << (RefArgBrack << *_[Array]));
        },

      In(Group) *
          ((T(Ref) << (T(RefHead)[RefHead] * T(RefArgSeq)[RefArgSeq])) *
           T(Dot) * T(Var)[Rhs]) >>
        [](Match& _) {
          return Ref << _(RefHead)
                     << (RefArgSeq << *_[RefArgSeq] << (RefArgDot << _(Rhs)));
        },

      In(Group) *
          ((T(Ref) << (T(RefHead)[RefHead] * T(RefArgSeq)[RefArgSeq])) *
           T(Array)[Array]) >>
        [](Match& _) {
          return Ref << _(RefHead)
                     << (RefArgSeq << *_[RefArgSeq]
                                   << (RefArgBrack << *_[Array]));
        },

      // errors
      In(RefArgBrack) * (T(Group) * T(Group)[Group]) >>
        [](Match& _) {
          return err(
            _(Group), "Multi-dimensional array references are not supported");
        },

      In(RefArgSeq) * (T(RefArgBrack)[RefArgBrack] << End) >>
        [](Match& _) {
          return err(_(RefArgBrack), "Must provide an index argument");
        },
    };
  }
}