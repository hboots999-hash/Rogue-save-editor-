#include "rogue_bridge.h"
#include "candidate_profile.hpp"
#include "observed_profile.hpp"
#include <algorithm>
#include <new>
#include <utility>
#include <vector>

struct rogue_snapshot {
    rogue_preview preview{};
    std::vector<rogue_party_candidate> party;
    std::vector<rogue_item_candidate> bag;
};

namespace {
rogue_profile classify(const uint8_t* bytes, size_t n, const rogue::ObservedView& view) {
    if(view.status != rogue::ObservedStatus::Candidate) return ROGUE_UNSUPPORTED;
    if(view.saveVersion == 6 && rogue::inspectCandidate212(bytes,n).status == rogue::CandidateStatus::CandidateOnly)
        return ROGUE_CANDIDATE_EX_212;
    if(view.saveVersion == 8 && rogue::inspectCandidate(bytes,n).status == rogue::CandidateStatus::CandidateOnly)
        return ROGUE_CANDIDATE_EX_221;
    return ROGUE_UNSUPPORTED;
}
rogue_party_candidate convert(const rogue::ObservedMon& source) {
    rogue_party_candidate out{};
    out.species_id = source.species;
    out.held_item_id = source.heldItem;
    out.nature_index = source.nature;
    out.ability_slot = source.abilitySlot;
    std::copy(source.iv.begin(), source.iv.end(), out.ivs);
    std::copy(source.ev.begin(), source.ev.end(), out.evs);
    std::copy(source.moves.begin(), source.moves.end(), out.moves);
    std::copy(source.pp.begin(), source.pp.end(), out.move_pp);
    return out;
}
} // namespace

extern "C" rogue_snapshot* rogue_snapshot_open(const uint8_t* bytes, size_t n) {
    if(!bytes || n != ROGUE_RAW_SAVE_SIZE) return nullptr;
    // Catch exceptions at the C ABI boundary so PKSM's C callers cannot
    // accidentally unwind through C frames on malformed/untrusted input.
    try {
        const auto view = rogue::inspectObserved(bytes, n);
        const rogue_profile profile = classify(bytes, n, view);
        if(profile == ROGUE_UNSUPPORTED) return nullptr;
        auto* result = new (std::nothrow) rogue_snapshot;
        if(!result) return nullptr;
        try {
            result->preview.profile = profile;
            result->preview.serialized_marker = view.saveVersion;
            result->preview.has_unverified_pokemon_layout = 1;
            result->preview.money = view.money;
            result->preview.save_counter = view.counter;
            result->preview.party_count = static_cast<uint16_t>(view.party.size());
            result->preview.occupied_bag_slots = static_cast<uint16_t>(view.items.size());
            result->party.reserve(view.party.size());
            result->bag.reserve(view.items.size());
            for(const auto& pokemon : view.party) result->party.push_back(convert(pokemon));
            for(const auto& item : view.items) result->bag.push_back({item.slot, item.id, item.quantity});
        } catch(...) {
            delete result;
            return nullptr;
        }
        return result;
    } catch(...) {
        return nullptr;
    }
}

extern "C" void rogue_snapshot_close(rogue_snapshot* snapshot) { delete snapshot; }

extern "C" int rogue_snapshot_summary(const rogue_snapshot* snapshot, rogue_preview* output) {
    if(!output) return 0;
    *output = rogue_preview{};
    if(!snapshot) return 0;
    *output = snapshot->preview;
    return 1;
}
extern "C" int rogue_snapshot_party(const rogue_snapshot* snapshot, size_t index, rogue_party_candidate* output) {
    if(!output) return 0;
    *output = rogue_party_candidate{};
    if(!snapshot || index >= snapshot->party.size()) return 0;
    *output = snapshot->party[index];
    return 1;
}
extern "C" int rogue_snapshot_item(const rogue_snapshot* snapshot, size_t index, rogue_item_candidate* output) {
    if(!output) return 0;
    *output = rogue_item_candidate{};
    if(!snapshot || index >= snapshot->bag.size()) return 0;
    *output = snapshot->bag[index];
    return 1;
}

extern "C" int rogue_preview_copy(const uint8_t* bytes, size_t n, rogue_preview* output) {
    if(!output) return 0;
    *output = rogue_preview{};
    rogue_snapshot* snapshot = rogue_snapshot_open(bytes,n);
    const int ok = rogue_snapshot_summary(snapshot,output);
    rogue_snapshot_close(snapshot);
    return ok;
}
extern "C" int rogue_get_party_candidate(const uint8_t* bytes, size_t n, size_t index, rogue_party_candidate* output) {
    if(!output) return 0;
    *output = rogue_party_candidate{};
    rogue_snapshot* snapshot = rogue_snapshot_open(bytes,n);
    const int ok = rogue_snapshot_party(snapshot,index,output);
    rogue_snapshot_close(snapshot);
    return ok;
}
extern "C" int rogue_get_item_candidate(const uint8_t* bytes, size_t n, size_t index, rogue_item_candidate* output) {
    if(!output) return 0;
    *output = rogue_item_candidate{};
    rogue_snapshot* snapshot = rogue_snapshot_open(bytes,n);
    const int ok = rogue_snapshot_item(snapshot,index,output);
    rogue_snapshot_close(snapshot);
    return ok;
}
