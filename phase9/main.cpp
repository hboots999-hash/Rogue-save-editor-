/* PKSM Rogue Phase 9 experimental native UI: offline READ ONLY.
 * GPL-3.0-or-later. Never mounts extdata or installed game-save archives.
 * Double confirmation is a fail-closed UI rehearsal, NOT an editor release.
 */
#include <3ds.h>
#include <cstdio>
#include <cstdint>
#include <memory>
#include <new>
#include "rogue_bridge.h"
#include "ui_state.hpp"

namespace {
constexpr const char* importPath = "sdmc:/3ds/PKSM-Rogue/imports/00000001.sav";
struct Closer { void operator()(FILE* f) const { if (f) fclose(f); } };
using File = std::unique_ptr<FILE, Closer>;

std::unique_ptr<uint8_t[]> readExact() {
    File input(fopen(importPath, "rb"));
    if (!input) return {};
    std::unique_ptr<uint8_t[]> data(new(std::nothrow) uint8_t[ROGUE_RAW_SAVE_SIZE]);
    if (!data || fread(data.get(), 1, ROGUE_RAW_SAVE_SIZE, input.get()) != ROGUE_RAW_SAVE_SIZE
        || fgetc(input.get()) != EOF || ferror(input.get())) return {};
    return data;
}

void show(const rogue_ui::State& s, const rogue_snapshot* snapshot,
          const rogue_preview& p, bool inputPresent) {
    consoleClear();
    puts("PKSM ROGUE - 3DS RUNTIME TEST");
    puts("==============================");
    puts("READ ONLY. NO LIVE SAVE ACCESS.");
    printf("PAGE %u/3 - INPUT %s\n\n",
           static_cast<unsigned>(s.page) + 1u, inputPresent ? "PRESENT" : "MISSING");

    if (!snapshot) {
        puts("No supported SD copy loaded.");
        puts("/3ds/PKSM-Rogue/imports/");
        puts("00000001.sav (128 KiB)");
    } else if (s.mode == rogue_ui::Mode::ConfirmOne
               || s.mode == rogue_ui::Mode::ConfirmTwo) {
        puts("OFFLINE EDIT REQUEST (REHEARSAL)");
        puts("No data has been modified.");
        puts("ROM layout is not verified.");
        printf("Confirm %u/2 with A, B cancels.\n",
               s.mode == rogue_ui::Mode::ConfirmOne ? 1u : 2u);
        puts("Any confirmed edit will be BLOCKED.");
    } else if (s.mode == rogue_ui::Mode::Blocked) {
        puts("EDIT BLOCKED - NO SAVE WRITTEN");
        puts("Exact Rogue ROM mapping unknown.");
        puts("Imported SD copy remains untouched.");
        puts("B: return to read-only viewer.");
    } else if (s.mode == rogue_ui::Mode::Detail) {
        if (s.page == rogue_ui::Page::Summary) {
            printf("MONEY: %lu\n", static_cast<unsigned long>(p.money));
            printf("Counter: %lu\n", static_cast<unsigned long>(p.save_counter));
            printf("Profile candidate: %u\n", static_cast<unsigned>(p.profile));
            puts("Money edits disabled until ROM check.");
        } else if (s.page == rogue_ui::Page::Party) {
            rogue_party_candidate mon{};
            if (rogue_snapshot_party(snapshot, s.selected, &mon)) {
                printf("PARTY %u | Species ID %u\n", s.selected + 1u, mon.species_id);
                printf("Item %u | Nature %u | Ability %u\n",
                       mon.held_item_id, mon.nature_index, mon.ability_slot);
                printf("IVs: %u %u %u %u %u %u\n",
                       mon.ivs[0],mon.ivs[1],mon.ivs[2],mon.ivs[3],mon.ivs[4],mon.ivs[5]);
                printf("EVs: %u %u %u %u %u %u\n",
                       mon.evs[0],mon.evs[1],mon.evs[2],mon.evs[3],mon.evs[4],mon.evs[5]);
                printf("Moves: %u %u %u %u\n",
                       mon.moves[0],mon.moves[1],mon.moves[2],mon.moves[3]);
            } else puts("Party slot could not be decoded.");
        } else {
            rogue_item_candidate item{};
            if (rogue_snapshot_item(snapshot, s.selected, &item))
                printf("BAG SLOT %u\nItem ID %u\nQuantity %u\n",
                       item.slot_index, item.item_id, item.quantity);
            else puts("Bag slot could not be decoded.");
        }
        puts("\nB: back   X: test blocked confirm");
    } else if (s.page == rogue_ui::Page::Summary) {
        printf("Marker %u, counter %lu\n", p.serialized_marker,
               static_cast<unsigned long>(p.save_counter));
        printf("Money %lu\nParty %u\nBag %u\n",
               static_cast<unsigned long>(p.money), p.party_count, p.occupied_bag_slots);
        puts("ROM compatibility NOT verified.");
    } else if (s.page == rogue_ui::Page::Party) {
        puts("PARTY - numerical IDs only");
        if (!p.party_count) puts("(empty)");
        const unsigned count = rogue_ui::visibleCount(s, p.party_count, p.occupied_bag_slots);
        for (unsigned i = 0; i < count; ++i) {
            rogue_party_candidate mon{};
            if (rogue_snapshot_party(snapshot, i, &mon))
                printf("%c %u: species %u, HP IV %u\n",
                       s.selected == i ? '>' : ' ', i + 1u, mon.species_id, mon.ivs[0]);
        }
    } else {
        puts("BAG - numerical IDs only");
        if (!p.occupied_bag_slots) puts("(empty)");
        const unsigned count = rogue_ui::visibleCount(s, p.party_count, p.occupied_bag_slots);
        const unsigned first = (s.selected / 7u) * 7u;
        for (unsigned i = first; i < count && i < first + 7u; ++i) {
            rogue_item_candidate item{};
            if (rogue_snapshot_item(snapshot, i, &item))
                printf("%c slot %u: ID %u x%u\n",
                       s.selected == i ? '>' : ' ', item.slot_index,item.item_id,item.quantity);
        }
    }
    puts("\nL/R pages, Up/Down select");
    puts("A details, B cancel/back, X test");
    puts("START safe exit. NO EDITS/RESTORE.");
}
} // namespace

int main() {
    gfxInitDefault();
    consoleInit(GFX_TOP, nullptr);
    std::unique_ptr<uint8_t[]> data = readExact();
    std::unique_ptr<rogue_snapshot, decltype(&rogue_snapshot_close)> snap(
        data ? rogue_snapshot_open(data.get(), ROGUE_RAW_SAVE_SIZE) : nullptr,
        rogue_snapshot_close);
    rogue_preview info{};
    if (snap && !rogue_snapshot_summary(snap.get(), &info)) snap.reset();

    rogue_ui::State state;
    bool dirty = true;
    while (aptMainLoop() && !state.exitRequested) {
        hidScanInput();
        const u32 keys = hidKeysDown();
        const bool supportedCopy = snap != nullptr;
        const auto pressKey = [&](rogue_ui::Key key) {
            rogue_ui::press(state, key, info.party_count, info.occupied_bag_slots, supportedCopy);
            dirty = true;
        };
        if (keys & KEY_START) pressKey(rogue_ui::Key::Exit);
        else if (keys & KEY_B) pressKey(rogue_ui::Key::Back);
        else if (keys & KEY_A) pressKey(rogue_ui::Key::Open);
        else if (keys & KEY_X) pressKey(rogue_ui::Key::Stage);
        else if (keys & KEY_R) pressKey(rogue_ui::Key::NextPage);
        else if (keys & KEY_L) pressKey(rogue_ui::Key::PrevPage);
        else if (keys & KEY_DOWN) pressKey(rogue_ui::Key::Down);
        else if (keys & KEY_UP) pressKey(rogue_ui::Key::Up);
        if (dirty && !state.exitRequested) {
            show(state, snap.get(), info, data != nullptr);
            dirty = false;
        }
        gspWaitForVBlank();
    }
    snap.reset();
    data.reset();
    gfxExit();
    return 0;
}
