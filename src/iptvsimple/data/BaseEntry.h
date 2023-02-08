/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "EpgGenre.h"
#include "../InstanceSettings.h"

#include <memory>
#include <string>
#include <vector>

#include <kodi/addon-instance/pvr/EPG.h>
#include <pugixml.hpp>

namespace iptvsimple
{
  namespace data
  {
    class BaseEntry
    {
    public:
      int GetGenreType() const { return m_genreType; }
      void SetGenreType(int value) { m_genreType = value; }

      int GetGenreSubType() const { return m_genreSubType; }
      void SetGenreSubType(int value) { m_genreSubType = value; }

      int GetYear() const { return m_year; }
      void SetYear(int value) { m_year = value; }

      int GetEpisodeNumber() const { return m_episodeNumber; }
      void SetEpisodeNumber(int value) { m_episodeNumber = value; }

      int GetEpisodePartNumber() const { return m_episodePartNumber; }
      void SetEpisodePartNumber(int value) { m_episodePartNumber = value; }

      int GetSeasonNumber() const { return m_seasonNumber; }
      void SetSeasonNumber(int value) { m_seasonNumber = value; }

      const std::string& GetFirstAired() const { return m_firstAired; }
      void SetFirstAired(const std::string& value) { m_firstAired = value; }

      const std::string& GetTitle() const { return m_title; }
      void SetTitle(const std::string& value) { m_title = value; }

      const std::string& GetEpisodeName() const { return m_episodeName; }
      void SetEpisodeName(const std::string& value) { m_episodeName = value; }

      const std::string& GetPlotOutline() const { return m_plotOutline; }
      void SetPlotOutline(const std::string& value) { m_plotOutline = value; }

      const std::string& GetPlot() const { return m_plot; }
      void SetPlot(const std::string& value) { m_plot = value; }

      const std::string& GetIconPath() const { return m_iconPath; }
      void SetIconPath(const std::string& value) { m_iconPath = value; }

      const std::string& GetGenreString() const { return m_genreString; }
      void SetGenreString(const std::string& value) { m_genreString = value; }

      const std::string& GetCast() const { return m_cast; }
      void SetCast(const std::string& value) { m_cast = value; }

      const std::string& GetDirector() const { return m_director; }
      void SetDirector(const std::string& value) { m_director = value; }

      const std::string& GetWriter() const { return m_writer; }
      void SetWriter(const std::string& value) { m_writer = value; }

      const std::string& GetParentalRating() const { return m_parentalRating; }
      void SetParentalRating(const std::string& value) { m_parentalRating = value; }

      const std::string& GetParentalRatingSystem() const { return m_parentalRatingSystem; }
      void SetParentalRatingSystem(const std::string& value) { m_parentalRatingSystem = value; }

      const std::string& GetParentalRatingIconPath() const { return m_parentalRatingIconPath; }
      void SetParentalRatingIconPath(const std::string& value) { m_parentalRatingIconPath = value; }

      int GetStarRating() const { return m_starRating; }
      void SetStarRating(int value) { m_starRating = value; }

      bool IsNew() const { return m_new; }
      void SetNew(int value) { m_new = value; }

      bool IsPremiere() const { return m_premiere; }
      void SetPremiere(int value) { m_premiere = value; }

    protected:
      int m_genreType = 0;
      int m_genreSubType = 0;
      int m_year = 0;
      int m_episodeNumber = EPG_TAG_INVALID_SERIES_EPISODE;
      int m_episodePartNumber = EPG_TAG_INVALID_SERIES_EPISODE;
      int m_seasonNumber = EPG_TAG_INVALID_SERIES_EPISODE;

      std::string m_firstAired;
      std::string m_title;
      std::string m_episodeName;
      std::string m_plotOutline;
      std::string m_plot;
      std::string m_iconPath;
      std::string m_genreString;

      std::string m_cast;
      std::string m_director;
      std::string m_writer;

      std::string m_parentalRating;
      std::string m_parentalRatingSystem;
      std::string m_parentalRatingIconPath;
      int m_starRating;

      bool m_new = false;
      bool m_premiere = false;

      std::shared_ptr<iptvsimple::InstanceSettings> m_settings;
    };
  } //namespace data
} //namespace iptvsimple
