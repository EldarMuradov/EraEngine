#include <gtest/gtest.h>

#include <ecs/observable_member.h>

#include <string>

namespace
{
    using namespace era_engine;
}

TEST(ECS_ObservableMember, DefaultIsNotChanged)
{
    ObservableMember<float> member;

    EXPECT_FALSE(member.is_changed());
}

TEST(ECS_ObservableMember, AssignDifferentValueMarksChanged)
{
    ObservableMember<float> member(1.0f);

    member = 2.0f;

    EXPECT_TRUE(member.is_changed());
    EXPECT_FLOAT_EQ(member.get(), 2.0f);

    member.sync_changes();

    EXPECT_FALSE(member.is_changed());
}

TEST(ECS_ObservableMember, AssignSameValueKeepsUnchanged)
{
    ObservableMember<int32> member(5);

    member = 5;

    EXPECT_FALSE(member.is_changed());

    const int32 same = 5;
    member = same;

    EXPECT_FALSE(member.is_changed());
}

TEST(ECS_ObservableMember, WriteAccess)
{
    ObservableMember<vec3> member(vec3(0.0f));

    member.get_silent_for_write().x = 1.0f;

    EXPECT_FALSE(member.is_changed());
    EXPECT_FLOAT_EQ(member.get().x, 1.0f);

    member.get_for_write().y = 2.0f;

    EXPECT_TRUE(member.is_changed());
    EXPECT_FLOAT_EQ(member->y, 2.0f);
    EXPECT_FLOAT_EQ((*member).x, 1.0f);
}

TEST(ECS_ObservableMember, NotifyAndSync)
{
    ObservableMember<int32> member(0);

    member.notify();
    member.notify();

    EXPECT_TRUE(member.is_changed());

    member.sync_changes();

    EXPECT_FALSE(member.is_changed());

    member.notify();

    EXPECT_TRUE(member.is_changed());
}

TEST(ECS_ObservableMember, CopyDoesNotInheritChanges)
{
    ObservableMember<std::string> source(std::string("first"));
    source = std::string("second");

    ObservableMember<std::string> copy(source);

    EXPECT_EQ(copy.get(), "second");
    EXPECT_FALSE(copy.is_changed());
    EXPECT_TRUE(source.is_changed());

    ObservableMember<std::string> moved(std::move(copy));

    EXPECT_EQ(moved.get(), "second");
    EXPECT_FALSE(moved.is_changed());
}

TEST(ECS_ObservableMember, AssignFromObservable)
{
    ObservableMember<int32> source(3);
    ObservableMember<int32> target(1);

    target = source;

    EXPECT_EQ(target.get(), 3);
    EXPECT_TRUE(target.is_changed());

    target.sync_changes();
    target = source;

    EXPECT_FALSE(target.is_changed());

    const int32& value = target;
    EXPECT_EQ(value, 3);
}
