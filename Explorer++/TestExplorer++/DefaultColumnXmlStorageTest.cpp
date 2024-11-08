// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "DefaultColumnXmlStorage.h"
#include "ColumnStorageTestHelper.h"
#include "ResourceTestHelper.h"
#include "ShellBrowser/FolderSettings.h"
#include "XmlStorageTestHelper.h"
#include <gtest/gtest.h>

class DefaultColumnXmlStorageTest : public XmlStorageTest
{
};

TEST_F(DefaultColumnXmlStorageTest, Load)
{
	auto referenceColumns = BuildFolderColumnsLoadSaveReference();

	std::wstring xmlFilePath = GetResourcePath(L"default-columns-config.xml");
	auto xmlDocument = LoadXmlDocument(xmlFilePath);
	ASSERT_TRUE(xmlDocument);

	FolderColumns loadedColumns;
	DefaultColumnXmlStorage::Load(xmlDocument.get(), loadedColumns);

	EXPECT_EQ(loadedColumns, referenceColumns);
}

TEST_F(DefaultColumnXmlStorageTest, Save)
{
	auto referenceColumns = BuildFolderColumnsLoadSaveReference();

	auto xmlDocumentData = CreateXmlDocument();
	ASSERT_TRUE(xmlDocumentData);

	DefaultColumnXmlStorage::Save(xmlDocumentData->xmlDocument.get(), xmlDocumentData->root.get(),
		referenceColumns);

	FolderColumns loadedColumns;
	DefaultColumnXmlStorage::Load(xmlDocumentData->xmlDocument.get(), loadedColumns);

	EXPECT_EQ(loadedColumns, referenceColumns);
}