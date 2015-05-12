// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/search_result.h"

#include <map>

#include "ui/app_list/app_list_constants.h"
#include "ui/app_list/search/tokenized_string.h"
#include "ui/app_list/search/tokenized_string_match.h"
#include "ui/app_list/search_result_observer.h"

namespace app_list {

SearchResult::Action::Action(const gfx::ImageSkia& base_image,
                             const gfx::ImageSkia& hover_image,
                             const gfx::ImageSkia& pressed_image,
                             const base::string16& tooltip_text)
    : base_image(base_image),
      hover_image(hover_image),
      pressed_image(pressed_image),
      tooltip_text(tooltip_text) {}

SearchResult::Action::Action(const base::string16& label_text,
                             const base::string16& tooltip_text)
    : tooltip_text(tooltip_text),
      label_text(label_text) {}

SearchResult::Action::~Action() {}

SearchResult::SearchResult()
    : relevance_(0),
      display_type_(DISPLAY_LIST),
      distance_from_origin_(-1),
      voice_result_(false),
      is_installing_(false),
      percent_downloaded_(0) {
}

SearchResult::~SearchResult() {
  FOR_EACH_OBSERVER(SearchResultObserver, observers_, OnResultDestroying());
}

void SearchResult::SetIcon(const gfx::ImageSkia& icon) {
  icon_ = icon;
  FOR_EACH_OBSERVER(SearchResultObserver,
                    observers_,
                    OnIconChanged());
}

void SearchResult::SetActions(const Actions& sets) {
  actions_ = sets;
  FOR_EACH_OBSERVER(SearchResultObserver,
                    observers_,
                    OnActionsChanged());
}

void SearchResult::SetIsInstalling(bool is_installing) {
  if (is_installing_ == is_installing)
    return;

  is_installing_ = is_installing;
  FOR_EACH_OBSERVER(SearchResultObserver,
                    observers_,
                    OnIsInstallingChanged());
}

void SearchResult::SetPercentDownloaded(int percent_downloaded) {
  if (percent_downloaded_ == percent_downloaded)
    return;

  percent_downloaded_ = percent_downloaded;
  FOR_EACH_OBSERVER(SearchResultObserver,
                    observers_,
                    OnPercentDownloadedChanged());
}

int SearchResult::GetPreferredIconDimension() const {
  switch (display_type_) {
    case DISPLAY_RECOMMENDATION:  // Falls through.
    case DISPLAY_TILE:
      return kTileIconSize;
    case DISPLAY_LIST:
      return kListIconSize;
    case DISPLAY_NONE:
      return 0;
    case DISPLAY_TYPE_LAST:
      break;
  }
  NOTREACHED();
  return 0;
}

void SearchResult::NotifyItemInstalled() {
  FOR_EACH_OBSERVER(SearchResultObserver, observers_, OnItemInstalled());
}

void SearchResult::AddObserver(SearchResultObserver* observer) {
  observers_.AddObserver(observer);
}

void SearchResult::RemoveObserver(SearchResultObserver* observer) {
  observers_.RemoveObserver(observer);
}

void SearchResult::UpdateFromMatch(const TokenizedString& title,
                                   const TokenizedStringMatch& match) {
  const TokenizedStringMatch::Hits& hits = match.hits();

  Tags tags;
  tags.reserve(hits.size());
  for (size_t i = 0; i < hits.size(); ++i)
    tags.push_back(Tag(Tag::MATCH, hits[i].start(), hits[i].end()));

  set_title(title.text());
  set_title_tags(tags);
  set_relevance(match.relevance());
}

void SearchResult::Open(int event_flags) {
}

void SearchResult::InvokeAction(int action_index, int event_flags) {
}

ui::MenuModel* SearchResult::GetContextMenuModel() {
  return NULL;
}

// static
std::string SearchResult::TagsDebugString(const std::string& text,
                                          const Tags& tags) {
  std::string result = text;

  // Build a table of delimiters to insert.
  std::map<size_t, std::string> inserts;
  for (const auto& tag : tags) {
    if (tag.styles & Tag::URL)
      inserts[tag.range.start()].push_back('{');
    if (tag.styles & Tag::MATCH)
      inserts[tag.range.start()].push_back('[');
    if (tag.styles & Tag::DIM) {
      inserts[tag.range.start()].push_back('<');
      inserts[tag.range.end()].push_back('>');
    }
    if (tag.styles & Tag::MATCH)
      inserts[tag.range.end()].push_back(']');
    if (tag.styles & Tag::URL)
      inserts[tag.range.end()].push_back('}');
  }

  // Insert the delimiters (in reverse order, to preserve indices).
  for (auto it = inserts.rbegin(); it != inserts.rend(); ++it)
    result.insert(it->first, it->second);

  return result;
}

}  // namespace app_list
