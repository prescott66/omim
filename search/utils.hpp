#pragma once

#include "search/token_slice.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/buffer_vector.hpp"
#include "base/levenshtein_dfa.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <functional>
#include <queue>
#include <vector>

namespace search
{
// todo(@m, @y). Unite with the similar function in search/feature_offset_match.hpp.
template <typename TrieIt, typename DFA, typename ToDo>
bool MatchInTrie(TrieIt const & trieStartIt, DFA const & dfa, ToDo && toDo)
{
  using Char = typename TrieIt::Char;
  using DFAIt = typename DFA::Iterator;
  using State = pair<TrieIt, DFAIt>;

  std::queue<State> q;

  {
    auto it = dfa.Begin();
    if (it.Rejects())
      return false;
    q.emplace(trieStartIt, it);
  }

  bool found = false;

  while (!q.empty())
  {
    auto const p = q.front();
    q.pop();

    auto const & trieIt = p.first;
    auto const & dfaIt = p.second;

    if (dfaIt.Accepts())
    {
      trieIt.ForEachInNode(toDo);
      found = true;
    }

    trieIt.ForEachMove([&](Char const & c, TrieIt const & nextTrieIt) {
      auto nextDfaIt = dfaIt;
      nextDfaIt.Move(c);
      if (!nextDfaIt.Rejects())
        q.emplace(nextTrieIt, nextDfaIt);
    });
  }

  return found;
}

using TLocales = buffer_vector<int8_t, 3>;

size_t GetMaxErrorsForToken(strings::UniString const & token);

strings::LevenshteinDFA BuildLevenshteinDFA(strings::UniString const & s);

template <typename ToDo>
void ForEachCategoryType(StringSliceBase const & slice, TLocales const & locales,
                         CategoriesHolder const & categories, ToDo && todo)
{
  for (size_t i = 0; i < slice.Size(); ++i)
  {
    auto const & token = slice.Get(i);
    for (int8_t const locale : locales)
      categories.ForEachTypeByName(locale, token, std::bind<void>(todo, i, std::placeholders::_1));

    // Special case processing of 2 codepoints emoji (e.g. black guy on a bike).
    // Only emoji synonyms can have one codepoint.
    if (token.size() > 1)
    {
      categories.ForEachTypeByName(CategoriesHolder::kEnglishCode, strings::UniString(1, token[0]),
                                   std::bind<void>(todo, i, std::placeholders::_1));
    }
  }
}

// Unlike ForEachCategoryType which extracts types by a token
// from |slice| by exactly matching it to a token in the name
// of a category, in the worst case this function has to loop through the tokens
// in all category synonyms in all |locales| in order to find a token
// whose edit distance is close enough to the required token from |slice|.
template <typename ToDo>
void ForEachCategoryTypeFuzzy(StringSliceBase const & slice, TLocales const & locales,
                              CategoriesHolder const & categories, ToDo && todo)
{
  using Trie = my::MemTrie<strings::UniString, my::VectorValues<uint32_t>>;

  auto const & trie = categories.GetNameToTypesTrie();
  auto const & trieRootIt = trie.GetRootIterator();
  vector<int8_t> sortedLocales(locales.begin(), locales.end());
  my::SortUnique(sortedLocales);

  for (size_t i = 0; i < slice.Size(); ++i)
  {
    auto const & token = slice.Get(i);
    // todo(@m, @y). We build dfa twice for each token: here and in geocoder.cpp.
    // A possible optimization is to build each dfa once and save it. Note that
    // dfas for the prefix tokens differ, i.e. we ignore slice.IsPrefix(i) here.
    strings::LevenshteinDFA const dfa(BuildLevenshteinDFA(token));

    trieRootIt.ForEachMove([&](Trie::Char const & c, Trie::Iterator const & trieStartIt) {
      if (std::binary_search(sortedLocales.begin(), sortedLocales.end(), static_cast<int8_t>(c)))
        MatchInTrie(trieStartIt, dfa, std::bind<void>(todo, i, std::placeholders::_1));
    });
  }
}
}  // namespace search
