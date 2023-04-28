#include "passes.h"

namespace rego
{
  PassDef structure()
  {
    return {
      In(Input) * (T(Brace) << T(KeyValueList)[KeyValueList]) >>
        [](Match& _) { return TopKeyValueList << *_[KeyValueList]; },

      In(Data) * (T(Brace) << T(KeyValueList)[KeyValueList]) >>
        [](Match& _) { return TopKeyValueList << *_[KeyValueList]; },

      T(Assign)
          << ((T(Group) << T(Ident)[Id]) * (T(Group)[Group]) *
              (T(Group) << (T(Brace) << T(BraceList)[BraceList]))) >>
        [](Match& _) {
          return Rule << _(Id) << (RuleValue << _(Group))
                      << (RuleBody << *_[BraceList]);
        },

      T(Assign)
          << ((T(Group) << T(Ident)[Id]) *
              (T(Group) << (T(Brace) << T(BraceList)[BraceList]))) >>
        [](Match& _) {
          return Rule << _(Id) << (RuleValue << (Bool ^ "true"))
                      << (RuleBody << *_[BraceList]);
        },

      T(Assign) << ((T(Group) << T(Ident)[Id]) * (T(Group)[Group])) >>
        [](Match& _) {
          auto empty_body = RuleBody << (Bool ^ "true");
          return Rule << _(Id) << (RuleValue << _(Group)) << empty_body;
        },

      In(RuleValue) * T(Group)[Group] >>
        [](Match& _) { return Expression << *_[Group]; },

      In(RuleBody) * T(Group)[Group] >>
        [](Match& _) { return Expression << *_[Group]; },

      In(Query) * T(Group)[Group] >>
        [](Match& _) { return Expression << *_[Group]; },

      In(Array) * T(Group)[Group] >>
        [](Match& _) { return Expression << *_[Group]; },

      In(Expression) * (T(Paren) << T(Group)[Group]) >>
        [](Match& _) { return Expression << *_[Group]; },

      T(KeyValue) << ((T(Group) << (T(String)[Key] * End)) * T(Group)[Group]) >>
        [](Match& _) {
          std::string key(_(Key)->location().view());
          key = key.substr(1, key.size() - 2);
          return KeyValue << (Key ^ key) << (Expression << *_[Group]);
        },

      In(TopKeyValueList) *
          (T(KeyValue) << (T(Key)[Key] * T(Expression)[Value])) >>
        [](Match& _) { return TopKeyValue << _(Key) << _(Value); },

      In(Expression) * (LookupLhs[Lhs] * T(Dot) * T(Ident)[Rhs]) >>
        [](Match& _) { return Lookup << _(Lhs) << _(Rhs); },

      In(Expression) *
          (LookupLhs[Lhs] * (T(Square) << T(SquareList)[SquareList])) >>
        [](Match& _) { return Lookup << _(Lhs) << (Index << *_[SquareList]); },

      In(Index) * (T(Group)[Group]) >>
        [](Match& _) { return Expression << *_[Group]; },

      In(Expression) * T(Lookup)[Lookup] >>
        [](Match& _) { return Ref << _(Lookup); },

      In(Expression) * T(Ident)[Id] >>
        [](Match& _) {
          std::string local(_(Id)->location().view());
          return Ref << (Local ^ local);
        },

      T(Expression) << (NotANumber[Value] * End) >>
        [](Match& _) { return _(Value); },

      T(Square) << T(SquareList)[SquareList] >>
        [](Match& _) { return Array << *_[SquareList]; },

      T(Brace) << T(KeyValueList)[KeyValueList] >>
        [](Match& _) { return Object << *_[KeyValueList]; },

      // errors

      T(Brace)[Brace] >>
        [](Match& _) { return err(_(Brace), "invalid brace"); },

      In(RuleSeq) * T(Assign)[Assign] >>
        [](Match& _) { return err(_(Assign), "invalid assign"); },

      In(RuleBody) * T(Assign)[Assign] >>
        [](Match& _) { return err(_(Assign), "invalid assign"); },

      In(KeyValue) * (!T(Key)[Lhs] * Any) >>
        [](Match& _) { return err(_(Lhs), "invalid key"); },

      In(KeyValue) * (T(Key) * T(Error)[Error]) >>
        [](Match& _) { return _(Error); },

      T(KeyValue) << T(Error)[Error] >> [](Match& _) { return _(Error); },

      In(Expression) * (NotANumber[Value] * Any) >>
        [](Match& _) {
          return err(_(Value), "invalid value for an expression");
        },

      In(Expression) * (Any * NotANumber[Value]) >>
        [](Match& _) {
          return err(_(Value), "invalid value for an expression");
        },

      In(Expression) * T(Dot)[Dot] >>
        [](Match& _) { return err(_(Dot), "cannot index this value"); },

      In(Expression) * (Any * T(Error)[Error]) >>
        [](Match& _) { return _(Error); },

      T(Expression) << T(Error)[Error] >> [](Match& _) { return _(Error); },

      T(Index)[Index] << End >>
        [](Match& _) { return err(_(Index), "Empty index"); },
    };
  }
}