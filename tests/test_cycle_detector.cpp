#include <gtest/gtest.h>
#include "../src/cycle_detector.hpp"

using namespace permuto;

class CycleDetectorTest : public ::testing::Test {
protected:
    CycleDetector detector;
    
    void SetUp() override {
        detector.clear();
    }
};

TEST_F(CycleDetectorTest, NoCycle) {
    EXPECT_FALSE(detector.would_create_cycle("/path1"));
    detector.push_path("/path1");
    
    EXPECT_FALSE(detector.would_create_cycle("/path2"));
    detector.push_path("/path2");
    
    EXPECT_FALSE(detector.would_create_cycle("/path3"));
}

TEST_F(CycleDetectorTest, DirectCycle) {
    detector.push_path("/path1");
    EXPECT_TRUE(detector.would_create_cycle("/path1"));
}

TEST_F(CycleDetectorTest, IndirectCycle) {
    detector.push_path("/path1");
    detector.push_path("/path2");
    detector.push_path("/path3");
    
    EXPECT_TRUE(detector.would_create_cycle("/path1"));
    EXPECT_TRUE(detector.would_create_cycle("/path2"));
    EXPECT_FALSE(detector.would_create_cycle("/path4"));
}

TEST_F(CycleDetectorTest, PopPath) {
    detector.push_path("/path1");
    detector.push_path("/path2");
    
    EXPECT_TRUE(detector.would_create_cycle("/path1"));
    
    detector.pop_path(); // Remove /path2
    EXPECT_TRUE(detector.would_create_cycle("/path1"));
    EXPECT_FALSE(detector.would_create_cycle("/path2"));
    
    detector.pop_path(); // Remove /path1
    EXPECT_FALSE(detector.would_create_cycle("/path1"));
    EXPECT_FALSE(detector.would_create_cycle("/path2"));
}

TEST_F(CycleDetectorTest, GetCurrentPath) {
    auto path = detector.get_current_path();
    EXPECT_TRUE(path.empty());
    
    detector.push_path("/path1");
    detector.push_path("/path2");
    detector.push_path("/path3");
    
    path = detector.get_current_path();
    ASSERT_EQ(path.size(), 3);
    EXPECT_EQ(path[0], "/path1");
    EXPECT_EQ(path[1], "/path2");
    EXPECT_EQ(path[2], "/path3");
}

TEST_F(CycleDetectorTest, Clear) {
    detector.push_path("/path1");
    detector.push_path("/path2");
    
    EXPECT_FALSE(detector.get_current_path().empty());
    EXPECT_TRUE(detector.would_create_cycle("/path1"));
    
    detector.clear();
    
    EXPECT_TRUE(detector.get_current_path().empty());
    EXPECT_FALSE(detector.would_create_cycle("/path1"));
    EXPECT_FALSE(detector.would_create_cycle("/path2"));
}

TEST_F(CycleDetectorTest, EmptyPopPath) {
    // Should not crash when popping from empty stack
    detector.pop_path();
    EXPECT_TRUE(detector.get_current_path().empty());
}