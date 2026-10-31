// SPDX-License-Identifier: GPL-3.0-or-later
// Build/link probe only; produces no installable application or writable handle.
#include "native_journal.hpp"
static_assert(!phase14::LiveWritesEnabled,"Native writes must stay disabled");
bool phase14_native_authentication_probe() {
    phase14::NativePxi pxi;
    if(!pxi.open())return false;
    phase14::Bytes container;
    phase14::Layout layout;
    const bool ok=pxi.read(container)&&phase14::authenticate(container,pxi,layout);
    return pxi.close()&&ok;
}

phase14::Status phase14_native_restore_probe(const phase14::Plan& plan) {
    phase14::NativeBackend backend;
    return phase14::execute(backend,backend,plan); // always Blocked
}
bool phase14_native_journal_load_probe(phase14::Plan& plan) {
    phase14::NativeJournal journal;return journal.load(plan);
}
