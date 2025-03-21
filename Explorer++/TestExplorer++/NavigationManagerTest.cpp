// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellBrowser/NavigationManager.h"
#include "GeneratorTestHelper.h"
#include "NavigationRequestTestHelper.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellEnumeratorFake.h"
#include "ShellTestHelper.h"
#include "../Helper/UniqueThreadId.h"
#include <gtest/gtest.h>
#include <future>

using namespace testing;

class NavigationManagerTest : public Test
{
protected:
	NavigationManagerTest() :
		m_shellEnumerator(std::make_shared<ShellEnumeratorFake>()),
		m_manualExecutorBackground(std::make_shared<concurrencpp::manual_executor>()),
		m_manualExecutorCurrent(std::make_shared<concurrencpp::manual_executor>()),
		m_navigationManager(std::make_unique<NavigationManager>(nullptr, &m_navigationEvents,
			m_shellEnumerator, m_manualExecutorBackground, m_manualExecutorCurrent))
	{
	}

	~NavigationManagerTest()
	{
		m_manualExecutorBackground->shutdown();
		m_manualExecutorCurrent->shutdown();
	}

	// Starts and then completes a navigation before returning.
	void CompleteNavigation(const NavigateParams &navigateParams)
	{
		m_navigationManager->StartNavigation(navigateParams);
		RunExecutors();
	}

	void RunExecutors()
	{
		m_manualExecutorBackground->loop(std::numeric_limits<size_t>::max());
		m_manualExecutorCurrent->loop(std::numeric_limits<size_t>::max());
	}

	NavigationEvents m_navigationEvents;
	const std::shared_ptr<ShellEnumeratorFake> m_shellEnumerator;
	const std::shared_ptr<concurrencpp::manual_executor> m_manualExecutorBackground;
	const std::shared_ptr<concurrencpp::manual_executor> m_manualExecutorCurrent;
	std::unique_ptr<NavigationManager> m_navigationManager;
};

// The mocks here are all strict, which means that this should only be used as a base class when
// testing that the appropriate mock methods are called.
class NavigationManagerSignalTest : public NavigationManagerTest
{
protected:
	NavigationManagerSignalTest()
	{
		m_navigationEvents.AddStartedObserver(m_navigationStartedCallback.AsStdFunction(),
			NavigationEventScope::Global());

		m_navigationEvents.AddWillCommitObserver(m_navigationWillCommitCallback.AsStdFunction(),
			NavigationEventScope::Global());
		m_navigationEvents.AddCommittedObserver(m_navigationCommittedCallback.AsStdFunction(),
			NavigationEventScope::Global());

		m_navigationEvents.AddFailedObserver(m_navigationFailedCallback.AsStdFunction(),
			NavigationEventScope::Global());

		m_navigationEvents.AddCancelledObserver(m_navigationCancelledCallback.AsStdFunction(),
			NavigationEventScope::Global());
	}

	StrictMock<MockFunction<void(const NavigationRequest *request)>> m_navigationStartedCallback;

	StrictMock<MockFunction<void(const NavigationRequest *request)>> m_navigationWillCommitCallback;
	StrictMock<MockFunction<void(const NavigationRequest *request)>> m_navigationCommittedCallback;

	StrictMock<MockFunction<void(const NavigationRequest *request)>> m_navigationFailedCallback;

	StrictMock<MockFunction<void(const NavigationRequest *request)>> m_navigationCancelledCallback;
};

TEST_F(NavigationManagerSignalTest, SuccessfulNavigation)
{
	PidlAbsolute pidl = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams = NavigateParams::Normal(pidl.Raw());

	{
		InSequence seq;

		EXPECT_CALL(m_navigationStartedCallback, Call(NavigateParamsMatch(navigateParams)));
		EXPECT_CALL(m_navigationWillCommitCallback, Call(NavigateParamsMatch(navigateParams)));
		EXPECT_CALL(m_navigationCommittedCallback, Call(NavigateParamsMatch(navigateParams)));
	}

	CompleteNavigation(navigateParams);
}

TEST_F(NavigationManagerSignalTest, FailedInitialNavigation)
{
	PidlAbsolute pidl = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams = NavigateParams::Normal(pidl.Raw());

	{
		InSequence seq;

		// Even though the navigation failed, it should still be committed, since it was the first
		// navigation.
		EXPECT_CALL(m_navigationStartedCallback, Call(NavigateParamsMatch(navigateParams)));
		EXPECT_CALL(m_navigationWillCommitCallback, Call(NavigateParamsMatch(navigateParams)));
		EXPECT_CALL(m_navigationCommittedCallback, Call(NavigateParamsMatch(navigateParams)));
	}

	m_shellEnumerator->SetShouldSucceed(false);
	CompleteNavigation(navigateParams);
}

TEST_F(NavigationManagerSignalTest, CancelledInitialNavigation)
{
	PidlAbsolute pidl = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams = NavigateParams::Normal(pidl.Raw());

	{
		InSequence seq;

		EXPECT_CALL(m_navigationStartedCallback, Call(NavigateParamsMatch(navigateParams)));
		EXPECT_CALL(m_navigationWillCommitCallback, Call(NavigateParamsMatch(navigateParams)));
		EXPECT_CALL(m_navigationCommittedCallback, Call(NavigateParamsMatch(navigateParams)));
	}

	m_navigationManager->StartNavigation(navigateParams);

	m_navigationManager->StopLoading();
	RunExecutors();
}

TEST_F(NavigationManagerSignalTest, FailedSubsequentNavigation)
{
	PidlAbsolute pidlSuccess = CreateSimplePidlForTest(L"c:\\");
	auto navigateParamsSuccess = NavigateParams::Normal(pidlSuccess.Raw());

	PidlAbsolute pidlFail = CreateSimplePidlForTest(L"d:\\");
	auto navigateParamsFail = NavigateParams::Normal(pidlFail.Raw());

	{
		InSequence seq;

		EXPECT_CALL(m_navigationStartedCallback, Call(NavigateParamsMatch(navigateParamsSuccess)));
		EXPECT_CALL(m_navigationWillCommitCallback,
			Call(NavigateParamsMatch(navigateParamsSuccess)));
		EXPECT_CALL(m_navigationCommittedCallback,
			Call(NavigateParamsMatch(navigateParamsSuccess)));
		EXPECT_CALL(m_navigationStartedCallback, Call(NavigateParamsMatch(navigateParamsFail)));
		EXPECT_CALL(m_navigationFailedCallback, Call(NavigateParamsMatch(navigateParamsFail)));
	}

	CompleteNavigation(navigateParamsSuccess);

	m_shellEnumerator->SetShouldSucceed(false);
	CompleteNavigation(navigateParamsFail);
}

TEST_F(NavigationManagerSignalTest, CancelledSubsequentNavigation)
{
	PidlAbsolute pidlSuccess = CreateSimplePidlForTest(L"c:\\");
	auto navigateParamsSuccess = NavigateParams::Normal(pidlSuccess.Raw());

	PidlAbsolute pidlCancel = CreateSimplePidlForTest(L"d:\\");
	auto navigateParamsCancel = NavigateParams::Normal(pidlCancel.Raw());

	{
		InSequence seq;

		EXPECT_CALL(m_navigationStartedCallback, Call(NavigateParamsMatch(navigateParamsSuccess)));
		EXPECT_CALL(m_navigationWillCommitCallback,
			Call(NavigateParamsMatch(navigateParamsSuccess)));
		EXPECT_CALL(m_navigationCommittedCallback,
			Call(NavigateParamsMatch(navigateParamsSuccess)));
		EXPECT_CALL(m_navigationStartedCallback, Call(NavigateParamsMatch(navigateParamsCancel)));
		EXPECT_CALL(m_navigationCancelledCallback, Call(NavigateParamsMatch(navigateParamsCancel)));
	}

	CompleteNavigation(navigateParamsSuccess);

	m_navigationManager->StartNavigation(navigateParamsCancel);
	m_navigationManager->StopLoading();
	RunExecutors();
}

TEST_F(NavigationManagerSignalTest, CommitWithPendingNavigation)
{
	PidlAbsolute pidl1 = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams1 = NavigateParams::Normal(pidl1.Raw());

	PidlAbsolute pidl2 = CreateSimplePidlForTest(L"d:\\");
	auto navigateParams2 = NavigateParams::Normal(pidl2.Raw());

	{
		InSequence seq;

		EXPECT_CALL(m_navigationStartedCallback, Call(NavigateParamsMatch(navigateParams1)));
		EXPECT_CALL(m_navigationStartedCallback, Call(NavigateParamsMatch(navigateParams2)));
		EXPECT_CALL(m_navigationWillCommitCallback, Call(NavigateParamsMatch(navigateParams1)));
		EXPECT_CALL(m_navigationCommittedCallback, Call(NavigateParamsMatch(navigateParams1)));
		EXPECT_CALL(m_navigationCancelledCallback, Call(NavigateParamsMatch(navigateParams2)));
	}

	m_navigationManager->StartNavigation(navigateParams1);
	m_navigationManager->StartNavigation(navigateParams2);

	// This should allow the first navigation to proceed, allowing it to commit. That will then
	// cause the second navigation to be cancelled.
	RunExecutors();
}

TEST_F(NavigationManagerSignalTest, ManagerDestroyedWithPendingNavigation)
{
	PidlAbsolute pidl = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams = NavigateParams::Normal(pidl.Raw());

	{
		InSequence seq;

		// Navigations are always synchronously started, so this signal should always be emitted.
		EXPECT_CALL(m_navigationStartedCallback, Call(NavigateParamsMatch(navigateParams)));
	}

	m_navigationManager->StartNavigation(navigateParams);

	m_navigationManager.reset();

	// This will resume the asynchronous navigation. However, because the NavigationManager instance
	// was destroyed, no signals should be emitted. This should be a safe operation.
	RunExecutors();
}

class NavigationManagerSignalThreadTest : public NavigationManagerSignalTest
{
protected:
	NavigationManagerSignalThreadTest()
	{
		auto originalThreadId = UniqueThreadId::GetForCurrentThread();

		// The callbacks should all be invoked on the original thread, even if the enumeration is
		// done on a background thread.
		ON_CALL(m_navigationStartedCallback, Call)
			.WillByDefault([originalThreadId](const NavigationRequest *)
				{ EXPECT_EQ(UniqueThreadId::GetForCurrentThread(), originalThreadId); });

		ON_CALL(m_navigationWillCommitCallback, Call)
			.WillByDefault([originalThreadId](const NavigationRequest *)
				{ EXPECT_EQ(UniqueThreadId::GetForCurrentThread(), originalThreadId); });
		ON_CALL(m_navigationCommittedCallback, Call)
			.WillByDefault([originalThreadId](const NavigationRequest *)
				{ EXPECT_EQ(UniqueThreadId::GetForCurrentThread(), originalThreadId); });

		ON_CALL(m_navigationFailedCallback, Call)
			.WillByDefault([originalThreadId](const NavigationRequest *)
				{ EXPECT_EQ(UniqueThreadId::GetForCurrentThread(), originalThreadId); });

		ON_CALL(m_navigationCancelledCallback, Call)
			.WillByDefault([originalThreadId](const NavigationRequest *)
				{ EXPECT_EQ(UniqueThreadId::GetForCurrentThread(), originalThreadId); });
	}
};

TEST_F(NavigationManagerSignalThreadTest, CheckThread)
{
	PidlAbsolute pidl1 = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams1 = NavigateParams::Normal(pidl1.Raw());

	PidlAbsolute pidl2 = CreateSimplePidlForTest(L"d:\\");
	auto navigateParams2 = NavigateParams::Normal(pidl2.Raw());

	PidlAbsolute pidl3 = CreateSimplePidlForTest(L"e:\\");
	auto navigateParams3 = NavigateParams::Normal(pidl3.Raw());

	{
		InSequence seq;

		EXPECT_CALL(m_navigationStartedCallback, Call(NavigateParamsMatch(navigateParams1)));
		EXPECT_CALL(m_navigationStartedCallback, Call(NavigateParamsMatch(navigateParams2)));
		EXPECT_CALL(m_navigationWillCommitCallback, Call(NavigateParamsMatch(navigateParams1)));
		EXPECT_CALL(m_navigationCommittedCallback, Call(NavigateParamsMatch(navigateParams1)));
		EXPECT_CALL(m_navigationCancelledCallback, Call(NavigateParamsMatch(navigateParams2)));

		EXPECT_CALL(m_navigationStartedCallback, Call(NavigateParamsMatch(navigateParams3)));
		EXPECT_CALL(m_navigationFailedCallback, Call(NavigateParamsMatch(navigateParams3)));
	}

	m_navigationManager->StartNavigation(navigateParams1);
	m_navigationManager->StartNavigation(navigateParams2);

	// This will complete the background potion of navigations 1 and 2.
	std::async(std::launch::async,
		[this] { m_manualExecutorBackground->loop(std::numeric_limits<size_t>::max()); })
		.wait();

	// This will cause navigations 1 and 2 to be finalized.
	m_manualExecutorCurrent->loop(std::numeric_limits<size_t>::max());

	m_shellEnumerator->SetShouldSucceed(false);
	m_navigationManager->StartNavigation(navigateParams3);

	// This will complete the background potion of navigation 3.
	std::async(std::launch::async,
		[this] { m_manualExecutorBackground->loop(std::numeric_limits<size_t>::max()); })
		.wait();

	// This will cause navigation 3 to be finalized.
	m_manualExecutorCurrent->loop(std::numeric_limits<size_t>::max());
}

TEST_F(NavigationManagerTest, PendingNavigations)
{
	EXPECT_THAT(GeneratorToVector(m_navigationManager->GetPendingNavigations()), IsEmpty());
	EXPECT_EQ(m_navigationManager->MaybeGetLatestPendingNavigation(), nullptr);
	EXPECT_EQ(m_navigationManager->GetNumPendingNavigations(), 0u);
	EXPECT_FALSE(m_navigationManager->HasAnyPendingNavigations());

	PidlAbsolute pidl1 = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams1 = NavigateParams::Normal(pidl1.Raw());
	m_navigationManager->StartNavigation(navigateParams1);
	EXPECT_THAT(GeneratorToVector(m_navigationManager->GetPendingNavigations()),
		ElementsAre(NavigateParamsMatch(navigateParams1)));
	ASSERT_NE(m_navigationManager->MaybeGetLatestPendingNavigation(), nullptr);
	EXPECT_THAT(m_navigationManager->MaybeGetLatestPendingNavigation(),
		NavigateParamsMatch(navigateParams1));
	EXPECT_EQ(m_navigationManager->GetNumPendingNavigations(), 1u);
	EXPECT_TRUE(m_navigationManager->HasAnyPendingNavigations());

	PidlAbsolute pidl2 = CreateSimplePidlForTest(L"d:\\");
	auto navigateParams2 = NavigateParams::Normal(pidl2.Raw());
	m_navigationManager->StartNavigation(navigateParams2);
	EXPECT_THAT(GeneratorToVector(m_navigationManager->GetPendingNavigations()),
		ElementsAre(NavigateParamsMatch(navigateParams1), NavigateParamsMatch(navigateParams2)));
	ASSERT_NE(m_navigationManager->MaybeGetLatestPendingNavigation(), nullptr);
	EXPECT_THAT(m_navigationManager->MaybeGetLatestPendingNavigation(),
		NavigateParamsMatch(navigateParams2));
	EXPECT_EQ(m_navigationManager->GetNumPendingNavigations(), 2u);
	EXPECT_TRUE(m_navigationManager->HasAnyPendingNavigations());

	// This should allow navigation 1 to complete.
	m_manualExecutorBackground->loop(1);
	m_manualExecutorCurrent->loop(1);
	EXPECT_THAT(GeneratorToVector(m_navigationManager->GetPendingNavigations()),
		ElementsAre(NavigateParamsMatch(navigateParams2)));
	ASSERT_NE(m_navigationManager->MaybeGetLatestPendingNavigation(), nullptr);
	EXPECT_THAT(m_navigationManager->MaybeGetLatestPendingNavigation(),
		NavigateParamsMatch(navigateParams2));
	EXPECT_EQ(m_navigationManager->GetNumPendingNavigations(), 1u);
	EXPECT_TRUE(m_navigationManager->HasAnyPendingNavigations());

	// This should allow navigation 2 to complete.
	m_manualExecutorBackground->loop(1);
	m_manualExecutorCurrent->loop(1);
	EXPECT_THAT(GeneratorToVector(m_navigationManager->GetPendingNavigations()), IsEmpty());
	EXPECT_EQ(m_navigationManager->MaybeGetLatestPendingNavigation(), nullptr);
	EXPECT_EQ(m_navigationManager->GetNumPendingNavigations(), 0u);
	EXPECT_FALSE(m_navigationManager->HasAnyPendingNavigations());
}

TEST_F(NavigationManagerTest, ActiveNavigations)
{
	EXPECT_THAT(GeneratorToVector(m_navigationManager->GetActiveNavigations()), IsEmpty());
	EXPECT_EQ(m_navigationManager->MaybeGetLatestActiveNavigation(), nullptr);
	EXPECT_EQ(m_navigationManager->GetNumActiveNavigations(), 0u);
	EXPECT_FALSE(m_navigationManager->HasAnyActiveNavigations());

	PidlAbsolute pidl1 = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams1 = NavigateParams::Normal(pidl1.Raw());
	m_navigationManager->StartNavigation(navigateParams1);
	EXPECT_THAT(GeneratorToVector(m_navigationManager->GetActiveNavigations()),
		ElementsAre(NavigateParamsMatch(navigateParams1)));
	ASSERT_NE(m_navigationManager->MaybeGetLatestActiveNavigation(), nullptr);
	EXPECT_THAT(m_navigationManager->MaybeGetLatestActiveNavigation(),
		NavigateParamsMatch(navigateParams1));
	EXPECT_EQ(m_navigationManager->GetNumActiveNavigations(), 1u);
	EXPECT_TRUE(m_navigationManager->HasAnyActiveNavigations());

	PidlAbsolute pidl2 = CreateSimplePidlForTest(L"d:\\");
	auto navigateParams2 = NavigateParams::Normal(pidl2.Raw());
	m_navigationManager->StartNavigation(navigateParams2);
	EXPECT_THAT(GeneratorToVector(m_navigationManager->GetActiveNavigations()),
		ElementsAre(NavigateParamsMatch(navigateParams1), NavigateParamsMatch(navigateParams2)));
	ASSERT_NE(m_navigationManager->MaybeGetLatestActiveNavigation(), nullptr);
	EXPECT_THAT(m_navigationManager->MaybeGetLatestActiveNavigation(),
		NavigateParamsMatch(navigateParams2));
	EXPECT_EQ(m_navigationManager->GetNumActiveNavigations(), 2u);
	EXPECT_TRUE(m_navigationManager->HasAnyActiveNavigations());

	// This should allow navigation 1 to complete, which should result in navigation 2 being
	// cancelled. That should leave no active navigations.
	m_manualExecutorBackground->loop(1);
	m_manualExecutorCurrent->loop(1);
	EXPECT_THAT(GeneratorToVector(m_navigationManager->GetActiveNavigations()), IsEmpty());
	EXPECT_EQ(m_navigationManager->MaybeGetLatestActiveNavigation(), nullptr);
	EXPECT_EQ(m_navigationManager->GetNumActiveNavigations(), 0u);
	EXPECT_FALSE(m_navigationManager->HasAnyActiveNavigations());

	// This navigation has been started after navigation 1 finished, so it can commit.
	PidlAbsolute pidl3 = CreateSimplePidlForTest(L"e:\\");
	auto navigateParams3 = NavigateParams::Normal(pidl3.Raw());
	m_navigationManager->StartNavigation(navigateParams3);
	EXPECT_THAT(GeneratorToVector(m_navigationManager->GetActiveNavigations()),
		ElementsAre(NavigateParamsMatch(navigateParams3)));
	ASSERT_NE(m_navigationManager->MaybeGetLatestActiveNavigation(), nullptr);
	EXPECT_THAT(m_navigationManager->MaybeGetLatestActiveNavigation(),
		NavigateParamsMatch(navigateParams3));
	EXPECT_EQ(m_navigationManager->GetNumActiveNavigations(), 1u);
	EXPECT_TRUE(m_navigationManager->HasAnyActiveNavigations());

	// This should allow navigation 2 to complete, resulting in its cancellation being finalized.
	// Navigation 3 should still be active, however.
	m_manualExecutorBackground->loop(1);
	m_manualExecutorCurrent->loop(1);
	EXPECT_THAT(GeneratorToVector(m_navigationManager->GetActiveNavigations()),
		ElementsAre(NavigateParamsMatch(navigateParams3)));
	ASSERT_NE(m_navigationManager->MaybeGetLatestActiveNavigation(), nullptr);
	EXPECT_THAT(m_navigationManager->MaybeGetLatestActiveNavigation(),
		NavigateParamsMatch(navigateParams3));
	EXPECT_EQ(m_navigationManager->GetNumActiveNavigations(), 1u);
	EXPECT_TRUE(m_navigationManager->HasAnyActiveNavigations());

	// This will complete navigation 3, leaving no active navigations.
	m_manualExecutorBackground->loop(1);
	m_manualExecutorCurrent->loop(1);
	EXPECT_THAT(GeneratorToVector(m_navigationManager->GetActiveNavigations()), IsEmpty());
	EXPECT_EQ(m_navigationManager->MaybeGetLatestActiveNavigation(), nullptr);
	EXPECT_EQ(m_navigationManager->GetNumActiveNavigations(), 0u);
	EXPECT_FALSE(m_navigationManager->HasAnyActiveNavigations());
}

TEST_F(NavigationManagerTest, InitialNavigationActiveWhenStopped)
{
	// This navigation has been stopped. Ordinarily, that would result in it being cancelled, but
	// since it's the initial navigation, it should be committed instead. That also means that the
	// navigation should continue to appear in the active list.
	//
	// In other words, because the navigation can still commit (in spite of being stopped), it's
	// considered active.
	PidlAbsolute pidl = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams = NavigateParams::Normal(pidl.Raw());
	m_navigationManager->StartNavigation(navigateParams);
	m_navigationManager->StopLoading();
	EXPECT_THAT(GeneratorToVector(m_navigationManager->GetActiveNavigations()),
		ElementsAre(NavigateParamsMatch(navigateParams)));
	ASSERT_NE(m_navigationManager->MaybeGetLatestActiveNavigation(), nullptr);
	EXPECT_THAT(m_navigationManager->MaybeGetLatestActiveNavigation(),
		NavigateParamsMatch(navigateParams));
	EXPECT_EQ(m_navigationManager->GetNumActiveNavigations(), 1u);
	EXPECT_TRUE(m_navigationManager->HasAnyActiveNavigations());
}

class NavigationManagerLatestNavigationLifetimeTest : public NavigationManagerTest
{
protected:
	NavigationManagerLatestNavigationLifetimeTest()
	{
		// The behavior of the initial navigation is different to subsequent navigations (e.g. the
		// initial navigation will be committed if it fails). So, an initial navigation is made here
		// first. That way, a failed navigation will result in a failed event, rather than a
		// committed event.
		PidlAbsolute pidl = CreateSimplePidlForTest(L"c:\\");
		auto navigateParams = NavigateParams::Normal(pidl.Raw());
		CompleteNavigation(navigateParams);

		// It's expected that a navigation will be registered for the duration of its lifetime. That
		// is, that it will be added to the list of pending navigations before the navigation
		// started event and removed after the completion event. Which then means that if there's a
		// single navigation, it should be returned as the latest in both the start event and the
		// completion event.
		ON_CALL(m_navigationStartedCallback, Call)
			.WillByDefault(std::bind_front(
				&NavigationManagerLatestNavigationLifetimeTest::CheckIsLatestNavigation, this));
		ON_CALL(m_navigationCommittedCallback, Call)
			.WillByDefault(std::bind_front(
				&NavigationManagerLatestNavigationLifetimeTest::CheckIsLatestNavigation, this));
		ON_CALL(m_navigationFailedCallback, Call)
			.WillByDefault(std::bind_front(
				&NavigationManagerLatestNavigationLifetimeTest::CheckIsLatestNavigation, this));
		ON_CALL(m_navigationCancelledCallback, Call)
			.WillByDefault(std::bind_front(
				&NavigationManagerLatestNavigationLifetimeTest::CheckIsLatestNavigation, this));

		m_navigationEvents.AddStartedObserver(m_navigationStartedCallback.AsStdFunction(),
			NavigationEventScope::Global());
		m_navigationEvents.AddCommittedObserver(m_navigationCommittedCallback.AsStdFunction(),
			NavigationEventScope::Global());
		m_navigationEvents.AddFailedObserver(m_navigationFailedCallback.AsStdFunction(),
			NavigationEventScope::Global());
		m_navigationEvents.AddCancelledObserver(m_navigationCancelledCallback.AsStdFunction(),
			NavigationEventScope::Global());
	}

	StrictMock<MockFunction<void(const NavigationRequest *request)>> m_navigationStartedCallback;
	StrictMock<MockFunction<void(const NavigationRequest *request)>> m_navigationCommittedCallback;
	StrictMock<MockFunction<void(const NavigationRequest *request)>> m_navigationFailedCallback;
	StrictMock<MockFunction<void(const NavigationRequest *request)>> m_navigationCancelledCallback;

private:
	void CheckIsLatestNavigation(const NavigationRequest *request)
	{
		EXPECT_EQ(m_navigationManager->MaybeGetLatestPendingNavigation(), request);
	}
};

TEST_F(NavigationManagerLatestNavigationLifetimeTest, SuccessfulNavigation)
{
	PidlAbsolute pidl = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams = NavigateParams::Normal(pidl.Raw());

	{
		InSequence seq;

		EXPECT_CALL(m_navigationStartedCallback, Call(NavigateParamsMatch(navigateParams)));
		EXPECT_CALL(m_navigationCommittedCallback, Call(NavigateParamsMatch(navigateParams)));
	}

	CompleteNavigation(navigateParams);
}

TEST_F(NavigationManagerLatestNavigationLifetimeTest, FailedNavigation)
{
	PidlAbsolute pidl = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams = NavigateParams::Normal(pidl.Raw());

	{
		InSequence seq;

		EXPECT_CALL(m_navigationStartedCallback, Call(NavigateParamsMatch(navigateParams)));
		EXPECT_CALL(m_navigationFailedCallback, Call(NavigateParamsMatch(navigateParams)));
	}

	m_shellEnumerator->SetShouldSucceed(false);
	CompleteNavigation(navigateParams);
}

TEST_F(NavigationManagerLatestNavigationLifetimeTest, CancelledNavigation)
{
	PidlAbsolute pidl = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams = NavigateParams::Normal(pidl.Raw());

	{
		InSequence seq;

		EXPECT_CALL(m_navigationStartedCallback, Call(NavigateParamsMatch(navigateParams)));
		EXPECT_CALL(m_navigationCancelledCallback, Call(NavigateParamsMatch(navigateParams)));
	}

	m_navigationManager->StartNavigation(navigateParams);
	m_navigationManager->StopLoading();
	RunExecutors();
}
