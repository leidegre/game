#include "entity-query.hh"

#include "archetype.hh"

#include "../common/hash.hh"

using namespace game;

namespace {
// Both query and archetype are sorted
// Look for all the query component types in the archetype and if all of them are found return true otherwise false
bool MergeJoinAll(const ComponentTypeId* query, i32 query_len, const ComponentTypeId* archetype, i32 archetype_len) {
  int i = 0, j = 0, k = 0;
  for (; i < query_len && j < archetype_len;) {
    if (query[i] < archetype[j]) {
      i++;
      continue;
    }
    if (archetype[j] < query[i]) {
      j++;
      continue;
    }
    if (query[i] == archetype[j]) {
      i++;
      j++;
      k++;
      continue;
    }
    return false; // all not found in archetype
  }
  return k == query_len;
}

bool MergeJoinAny(const ComponentTypeId* query, i32 query_len, const ComponentTypeId* archetype, i32 archetype_len) {
  int i = 0, j = 0;
  for (; i < query_len && j < archetype_len;) {
    if (query[i] < archetype[j]) {
      i++;
      continue;
    }
    if (archetype[j] < query[i]) {
      j++;
      continue;
    }
    if (query[i] == archetype[j]) {
      return true;
    }
  }
  return false;
}
} // namespace

bool EntityQuery::IsMatch(Archetype* archetype) {
  bool all = MergeJoinAll(all_, all_len_, archetype->types_, archetype->types_len_);
  bool any = MergeJoinAny(any_, any_len_, archetype->types_, archetype->types_len_);
  // todo: exclude
  return all && any;
}

u32 EntityQuery::HashCode() {
  Hash32 h;

  h.Init();

  h.Update(All());
  h.Update(AllAccessMode());

  h.Update(Any());
  h.Update(AnyAccessMode());

  h.Update(None());
  h.Update(NoneAccessMode());

  return h.Digest();
}

bool EntityQuery::Equals(const EntityQuery& other) {
  if ((this->all_len_ == other.all_len_) & (this->any_len_ == other.any_len_) & (this->none_len_ == other.none_len_)) {
    if (memcmp(this->all_, other.all_, size_t(all_len_) * sizeof(ComponentTypeId)) != 0) {
      return false;
    }
    if (memcmp(this->all_access_mode_, other.all_access_mode_, size_t(all_len_)) != 0) {
      return false;
    }
    if (memcmp(this->any_, other.any_, size_t(any_len_) * sizeof(ComponentTypeId)) != 0) {
      return false;
    }
    if (memcmp(this->any_access_mode_, other.any_access_mode_, size_t(any_len_)) != 0) {
      return false;
    }
    if (memcmp(this->none_, other.none_, size_t(none_len_) * sizeof(ComponentTypeId)) != 0) {
      return false;
    }
    if (memcmp(this->none_access_mode_, other.none_access_mode_, size_t(none_len_)) != 0) {
      return false;
    }
    return true;
  }
  return false;
}
