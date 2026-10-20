#include "engine/mesh.h"

#include <catch_amalgamated.hpp>

namespace {
    void setupMesh(dx::UMesh& mesh)
    {
        mesh.frameVerts = 1;
        mesh.animFrames = 4;
        mesh.scale = { 1, 1, 1 };
        mesh.verts = { { 0, 0, 0 }, { 10, 0, 0 }, { 100, 0, 0 }, { 200, 0, 0 } };
        dx::AnimSequence first;
        first.name = dx::Name("TweenTestA");
        first.startFrame = 0;
        first.numFrames = 2;
        first.rate = 10;
        dx::AnimSequence second;
        second.name = dx::Name("TweenTestB");
        second.startFrame = 2;
        second.numFrames = 2;
        second.rate = 10;
        mesh.sequences = { first, second };
    }
}

TEST_CASE("tweening starts from the last drawn pose and moves toward the new sequence")
{
    dx::UMesh mesh;
    setupMesh(mesh);
    dx::MeshPoseCache cache;
    mesh.tweenFrame(dx::Name("TweenTestA"), 0.25f, cache, 1);
    REQUIRE(cache.verts.size() == 1);
    CHECK(cache.verts[0].x == 5.0f);
    mesh.tweenFrame(dx::Name("TweenTestB"), -0.5f, cache, 1);
    CHECK(cache.verts[0].x == 5.0f);
    mesh.tweenFrame(dx::Name("TweenTestB"), -0.25f, cache, 1);
    CHECK(cache.verts[0].x == 52.5f);
    mesh.tweenFrame(dx::Name("TweenTestB"), -0.125f, cache, 1);
    CHECK(cache.verts[0].x == 76.25f);
    mesh.tweenFrame(dx::Name("TweenTestB"), 0.0f, cache, 1);
    CHECK(cache.verts[0].x == 100.0f);
}

TEST_CASE("blend channels add their offset from the first mesh frame")
{
    dx::UMesh mesh;
    setupMesh(mesh);
    dx::MeshAnimState state;
    state.sequence = dx::Name("TweenTestA");
    state.frame = 0.5f;
    state.blendSequences[0] = dx::Name("TweenTestB");
    state.blendFrames[0] = 0.0f;
    dx::MeshPoseCaches caches;
    std::vector<dx::Vector> pose;
    mesh.animatedPose(state, caches, pose, 1);
    REQUIRE(pose.size() == 1);
    CHECK(pose[0].x == 110.0f);
    CHECK(pose[0].y == 0.0f);
    CHECK(pose[0].z == 0.0f);
}

TEST_CASE("LOD mesh tweening stops once the tween indicator passes 0.97")
{
    dx::ULodMesh mesh;
    setupMesh(mesh);
    dx::MeshPoseCache cache;
    mesh.tweenFrame(dx::Name("TweenTestA"), 0.25f, cache, 1);
    mesh.tweenFrame(dx::Name("TweenTestB"), -0.5f, cache, 1);
    const float frames[] = { -0.25f, -0.125f, -0.0625f, -0.03125f, -0.015625f };
    for (float frame : frames) {
        mesh.tweenFrame(dx::Name("TweenTestB"), frame, cache, 1);
    }
    CHECK(cache.verts[0].x == 97.03125f);
    mesh.tweenFrame(dx::Name("TweenTestB"), -0.0078125f, cache, 1);
    CHECK(cache.verts[0].x == 97.03125f);
    CHECK(cache.tweenIndicator > 0.97f);
}

TEST_CASE("LOD mesh tweening cannot return more vertices than the cached pose had")
{
    dx::ULodMesh mesh;
    setupMesh(mesh);
    dx::MeshPoseCache cache;
    CHECK(mesh.tweenFrame(dx::Name("TweenTestA"), 0.25f, cache, 0) == 0);
    CHECK(mesh.tweenFrame(dx::Name("TweenTestB"), -0.5f, cache, 1) == 0);
    CHECK(mesh.tweenFrame(dx::Name("TweenTestA"), 0.5f, cache, 1) == 1);
}
