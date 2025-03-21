// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Switches events based on the currently selected window
 * (principally the listview and treeview).
 */

#include "stdafx.h"
#include "Explorer++.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellTreeView/ShellTreeView.h"
#include "TabContainerImpl.h"

void Explorerplusplus::OnCopyItemPath() const
{
	HWND hFocus;

	hFocus = GetFocus();

	if (hFocus == m_hActiveListView)
	{
		OnListViewCopyItemPath();
	}
	else if (hFocus == m_shellTreeView->GetHWND())
	{
		OnTreeViewCopyItemPath();
	}
}

void Explorerplusplus::OnCopyUniversalPaths() const
{
	HWND hFocus;

	hFocus = GetFocus();

	if (hFocus == m_hActiveListView)
	{
		OnListViewCopyUniversalPaths();
	}
	else if (hFocus == m_shellTreeView->GetHWND())
	{
		OnTreeViewCopyUniversalPaths();
	}
}

void Explorerplusplus::OnCopy(BOOL bCopy)
{
	HWND hFocus = GetFocus();

	if (hFocus == m_hActiveListView)
	{
		Tab &selectedTab = GetActivePane()->GetTabContainerImpl()->GetSelectedTab();
		selectedTab.GetShellBrowserImpl()->CopySelectedItemsToClipboard(bCopy);
	}
	else if (hFocus == m_shellTreeView->GetHWND())
	{
		m_shellTreeView->CopySelectedItemToClipboard(bCopy);
	}
}

void Explorerplusplus::OnFileRename()
{
	HWND hFocus = GetFocus();

	if (hFocus == m_hActiveListView)
	{
		Tab &selectedTab = GetActivePane()->GetTabContainerImpl()->GetSelectedTab();
		selectedTab.GetShellBrowserImpl()->StartRenamingSelectedItems();
	}
	else if (hFocus == m_shellTreeView->GetHWND())
	{
		m_shellTreeView->StartRenamingSelectedItem();
	}
}

void Explorerplusplus::OnFileDelete(bool permanent)
{
	HWND hFocus;

	hFocus = GetFocus();

	if (hFocus == m_hActiveListView)
	{
		Tab &tab = GetActivePane()->GetTabContainerImpl()->GetSelectedTab();
		tab.GetShellBrowserImpl()->DeleteSelectedItems(permanent);
	}
	else if (hFocus == m_shellTreeView->GetHWND())
	{
		m_shellTreeView->DeleteSelectedItem(permanent);
	}
}

void Explorerplusplus::OnSetFileAttributes() const
{
	HWND hFocus;

	hFocus = GetFocus();

	if (hFocus == m_hActiveListView)
	{
		OnListViewSetFileAttributes();
	}
	else if (hFocus == m_shellTreeView->GetHWND())
	{
		OnTreeViewSetFileAttributes();
	}
}

void Explorerplusplus::OnShowFileProperties() const
{
	HWND hFocus;

	hFocus = GetFocus();

	if (hFocus == m_hActiveListView)
	{
		const Tab &selectedTab = GetActivePane()->GetTabContainerImpl()->GetSelectedTab();
		selectedTab.GetShellBrowserImpl()->ShowPropertiesForSelectedFiles();
	}
	else if (hFocus == m_shellTreeView->GetHWND())
	{
		m_shellTreeView->ShowPropertiesOfSelectedItem();
	}
}

void Explorerplusplus::OnPaste()
{
	HWND focus = GetFocus();

	if (focus == m_hActiveListView)
	{
		OnListViewPaste();
	}
	else if (focus == m_shellTreeView->GetHWND())
	{
		m_shellTreeView->Paste();
	}
}

void Explorerplusplus::OnPasteShortcut()
{
	HWND focus = GetFocus();

	if (focus == m_hActiveListView)
	{
		GetActiveShellBrowserImpl()->PasteShortcut();
	}
	else if (focus == m_shellTreeView->GetHWND())
	{
		m_shellTreeView->PasteShortcut();
	}
}
