// SPDX-License-Identifier: GPL-3.0-or-later
// Host contract tests for experimental opt-in UI. No game or ROM fixtures.
#include "phase20/move_draft_v1.hpp"
#include <cassert>
#include <cstring>
int main() {
    static phase20::LocalMoveProvider catalog{};
    static phase19::Demo demo{};
    phase19::populateMock(demo);
    phase19::MoveBrowser browser{};
    phase20::CopyObservation observation{};
    phase20::MoveDraftFlow flow{};
    assert(flow.stage==phase20::MoveDraftStage::Idle);
    assert(!rogue18::move_v3::MayWriteInstalledGame);
    assert(!rogue18::move_queue_v4::mayWriteInstalledGame(
        rogue18::move_queue_v4::Queue{}));
    // A synthetic party can NEVER create even an experimental offline draft.
    assert(!phase20::beginExperimentalMoveDraft(flow,browser,demo,
                                                observation,catalog));
    assert(flow.stage==phase20::MoveDraftStage::Result);
    assert(!flow.succeeded);
    phase20::moveDraftInput(flow,false,true,demo,observation,catalog);
    assert(flow.stage==phase20::MoveDraftStage::Idle);
    // Without an authenticated SD import + exact-ROM catalogue, fail closed.
    demo.synthetic=false;
    assert(!phase20::beginExperimentalMoveDraft(flow,browser,demo,
                                                observation,catalog));
    assert(flow.stage==phase20::MoveDraftStage::Result);
    assert(!flow.succeeded);
    phase20::moveDraftInput(flow,true,false,demo,observation,catalog);
    assert(flow.stage==phase20::MoveDraftStage::Idle);
    return 0;
}
