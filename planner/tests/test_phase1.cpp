#include "action.hpp"
#include "simulated_environment.hpp"
#include "state.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace autopilot {
namespace {

VirtualPath path(std::string_view value) {
    return VirtualPath::parse(value);
}

State emptyState() {
    return State({path("/workspace/Downloads")});
}

FileRecord fileRecord(std::uint64_t size = 3, char hash_character = 'a') {
    return {size, std::string(64, hash_character)};
}

TEST(VirtualPathTests, RejectsRelativeAndTraversalPaths) {
    EXPECT_THROW(VirtualPath::parse("Downloads/file.txt"), std::invalid_argument);
    EXPECT_THROW(VirtualPath::parse("/workspace/Downloads/../secret.txt"), std::invalid_argument);
    EXPECT_THROW(VirtualPath::parse("/workspace\\Downloads\\file.txt"), std::invalid_argument);
    EXPECT_THROW(VirtualPath::parse(std::string("/workspace/Downloads/line\nbreak")), std::invalid_argument);
}

TEST(StateTests, AllowlistChecksPathBoundaries) {
    const auto state = emptyState();
    EXPECT_TRUE(state.isAllowed(path("/workspace/Downloads")));
    EXPECT_TRUE(state.isAllowed(path("/workspace/Downloads/file.txt")));
    EXPECT_FALSE(state.isAllowed(path("/workspace/Downloads-old/file.txt")));
    EXPECT_FALSE(state.isAllowed(path("/outside/file.txt")));
}

TEST(StateTests, RejectsOutOfScopeAndOverwritingSeedFiles) {
    auto state = emptyState();
    EXPECT_THROW(state.addFile(path("/outside/file.txt"), fileRecord()), std::invalid_argument);
    state.addFile(path("/workspace/Downloads/file.txt"), fileRecord());
    EXPECT_THROW(state.addFile(path("/workspace/Downloads/file.txt"), fileRecord()), std::invalid_argument);
    EXPECT_THROW(state.addFile(path("/workspace/Downloads/bad.txt"), {3, "not-a-sha256"}), std::invalid_argument);
}

TEST(StateTests, RequiresNarrowAllowlistedRoots) {
    EXPECT_THROW(State({path("/")}), std::invalid_argument);
    EXPECT_THROW(State({}), std::invalid_argument);
}

TEST(ActionRegistryTests, DescribesOnlyTypedSupportedActions) {
    const ActionRegistry registry;
    ASSERT_EQ(registry.definitions().size(), 2U);

    const auto* move = registry.find("MoveFile");
    ASSERT_NE(move, nullptr);
    EXPECT_EQ(move->parameters.size(), 2U);
    EXPECT_EQ(move->parameters[0].type, ParameterType::Path);
    EXPECT_FALSE(move->verifier.empty());
    EXPECT_TRUE(move->reversible);

    EXPECT_EQ(registry.find("DeleteFile"), nullptr);
    EXPECT_EQ(definitionFor(registry, Action{CreateDirectory{path("/workspace/Downloads/Sorted")}}).name,
              "CreateDirectory");
}

TEST(SimulatedEnvironmentTests, CreatesAndVerifiesDirectory) {
    SimulatedEnvironment environment(emptyState());
    const auto result = environment.execute(CreateDirectory{path("/workspace/Downloads/Images")});

    EXPECT_TRUE(result.applied);
    EXPECT_TRUE(result.verified);
    EXPECT_TRUE(environment.observe().directoryExists(path("/workspace/Downloads/Images")));
}

TEST(SimulatedEnvironmentTests, MovePreservesHashAndSizeWithoutOverwriting) {
    auto initial = emptyState();
    initial.addDirectory(path("/workspace/Downloads/Sorted"));
    const auto original = fileRecord(42, 'B');
    initial.addFile(path("/workspace/Downloads/image.png"), original);
    SimulatedEnvironment environment(std::move(initial));

    const auto result = environment.execute(MoveFile{
        path("/workspace/Downloads/image.png"),
        path("/workspace/Downloads/Sorted/image.png")
    });

    ASSERT_TRUE(result.applied);
    ASSERT_TRUE(result.verified);
    EXPECT_FALSE(environment.observe().fileExists(path("/workspace/Downloads/image.png")));
    ASSERT_NE(environment.observe().file(path("/workspace/Downloads/Sorted/image.png")), nullptr);
    EXPECT_EQ(*environment.observe().file(path("/workspace/Downloads/Sorted/image.png")),
              (FileRecord{42, std::string(64, 'b')}));
}

TEST(SimulatedEnvironmentTests, RefusesExistingDestinationAndKeepsState) {
    auto initial = emptyState();
    initial.addDirectory(path("/workspace/Downloads/Sorted"));
    initial.addFile(path("/workspace/Downloads/source.txt"), fileRecord());
    initial.addFile(path("/workspace/Downloads/Sorted/source.txt"), fileRecord(4, 'b'));
    SimulatedEnvironment environment(std::move(initial));

    const auto result = environment.execute(MoveFile{
        path("/workspace/Downloads/source.txt"),
        path("/workspace/Downloads/Sorted/source.txt")
    });

    EXPECT_FALSE(result.applied);
    EXPECT_FALSE(result.verified);
    EXPECT_TRUE(environment.observe().fileExists(path("/workspace/Downloads/source.txt")));
    EXPECT_EQ(environment.observe().file(path("/workspace/Downloads/Sorted/source.txt"))->size, 4U);
}

TEST(SimulatedEnvironmentTests, RefusesOutOfScopeAndMissingParentActions) {
    SimulatedEnvironment environment(emptyState());

    const auto outside = environment.execute(CreateDirectory{path("/outside/Images")});
    const auto missing_parent = environment.execute(CreateDirectory{path("/workspace/Downloads/Missing/Images")});

    EXPECT_FALSE(outside.applied);
    EXPECT_FALSE(outside.verified);
    EXPECT_FALSE(missing_parent.applied);
    EXPECT_FALSE(missing_parent.verified);
}

} // namespace
} // namespace autopilot
