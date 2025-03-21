// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <glog/logging.h>
#include <memory>
#include <vector>

template <typename HistoryEntryType>
class NavigationController
{
public:
	NavigationController() : m_currentEntry(-1)
	{
	}

	NavigationController(std::vector<std::unique_ptr<HistoryEntryType>> &&entries,
		int currentEntry) :
		m_entries(std::move(entries))
	{
		// This constructor should only be called with at least one entry. Passing in an empty set
		// of entries isn't valid. It's necessary to validate this before initializing
		// `m_currentEntry` below, since the `std::clamp` call won't be valid if the set of entries
		// is empty (lo will be greater than hi, which will cause undefined behavior).
		CHECK(!m_entries.empty());

		m_currentEntry = std::clamp(currentEntry, 0, static_cast<int>(m_entries.size() - 1));
	}

	virtual ~NavigationController() = default;

	int GetNumHistoryEntries() const
	{
		return static_cast<int>(m_entries.size());
	}

	HistoryEntryType *GetCurrentEntry() const
	{
		return GetEntryAtIndex(GetCurrentIndex());
	}

	int GetCurrentIndex() const
	{
		return m_currentEntry;
	}

	HistoryEntryType *GetEntry(int offset) const
	{
		int index = m_currentEntry + offset;

		if (index < 0 || index >= GetNumHistoryEntries())
		{
			return nullptr;
		}

		return m_entries[index].get();
	}

	HistoryEntryType *GetEntryAtIndex(int index) const
	{
		if (index < 0 || index >= GetNumHistoryEntries())
		{
			return nullptr;
		}

		return m_entries[index].get();
	}

	std::optional<int> GetIndexOfEntry(const HistoryEntryType *entry) const
	{
		auto itr = std::find_if(m_entries.begin(), m_entries.end(),
			[entry](auto &currentEntry) { return currentEntry.get() == entry; });

		if (itr == m_entries.end())
		{
			return std::nullopt;
		}

		return static_cast<int>(itr - m_entries.begin());
	}

	bool CanGoBack() const
	{
		if (m_currentEntry == -1)
		{
			return false;
		}

		return m_currentEntry > 0;
	}

	bool CanGoForward() const
	{
		if (m_currentEntry == -1)
		{
			return false;
		}

		return (GetNumHistoryEntries() - m_currentEntry - 1) > 0;
	}

	std::vector<HistoryEntryType *> GetBackHistory() const
	{
		std::vector<HistoryEntryType *> history;

		if (m_currentEntry == -1)
		{
			return history;
		}

		for (int i = m_currentEntry - 1; i >= 0; i--)
		{
			history.push_back(m_entries[i].get());
		}

		return history;
	}

	std::vector<HistoryEntryType *> GetForwardHistory() const
	{
		std::vector<HistoryEntryType *> history;

		if (m_currentEntry == -1)
		{
			return history;
		}

		for (int i = m_currentEntry + 1; i < GetNumHistoryEntries(); i++)
		{
			history.push_back(m_entries[i].get());
		}

		return history;
	}

	void GoBack()
	{
		GoToOffset(-1);
	}

	void GoForward()
	{
		GoToOffset(1);
	}

	void GoToOffset(int offset)
	{
		auto entry = GetEntry(offset);

		if (!entry)
		{
			return;
		}

		Navigate(entry);
	}

protected:
	virtual void Navigate(const HistoryEntryType *entry) = 0;

	int AddEntry(std::unique_ptr<HistoryEntryType> entry)
	{
		// This will implicitly remove all "forward" entries.
		m_entries.resize(m_currentEntry + 1);

		m_entries.push_back(std::move(entry));
		m_currentEntry++;

		return m_currentEntry;
	}

	int ReplaceCurrentEntry(std::unique_ptr<HistoryEntryType> entry)
	{
		if (m_currentEntry == -1)
		{
			// Shouldn't be attempting to replace the current entry when there is no current entry.
			DCHECK(false);

			return AddEntry(std::move(entry));
		}

		m_entries[m_currentEntry] = std::move(entry);

		return m_currentEntry;
	}

	void SetCurrentIndex(int index)
	{
		CHECK(index >= 0 && index < GetNumHistoryEntries());

		m_currentEntry = index;
	}

private:
	std::vector<std::unique_ptr<HistoryEntryType>> m_entries;
	int m_currentEntry;
};
