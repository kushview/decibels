/// A MIDI buffer.
// Designed for real time performance, therefore does virtually no type
// checking in method calls.
// @classmod kv.MidiBuffer
// @pragma nostrip

#include "kv/lua/midi_buffer.hpp"
#include "packed.h"
#define LKV_MT_MIDI_BUFFER_TYPE "kv.MidiBufferClass"

using MidiBuffer    = juce::MidiBuffer;
using Iterator      = juce::MidiBufferIterator;
using MidiMessage   = juce::MidiMessage;
using Impl          = kv::lua::MidiBufferImpl;

/// Create an empty MIDI Buffer
// @function MidiBuffer.new
// @return A new midi buffer
// @within Constructors

/// Create a new MIDI Buffer
// @param size Size in bytes
// @function MidiBuffer.new
// @return A new MIDI Buffer
// @within Constructors
static int midibuffer_new (lua_State* L) {
    auto** impl = kv::lua::new_midibuffer (L);
    if (lua_gettop (L) > 1) {
        if (lua_isinteger (L, 2)) {
            (**impl).buffer.ensureSize (static_cast<size_t> (lua_tointeger (L, 2)));
        }
    }
    return 1;
}

static int midibuffer_free (lua_State* L) {
    auto** impl = (Impl**) lua_touserdata (L, 1);
    if (nullptr != *impl) {
        (*impl)->free (L);
        delete (*impl);
        *impl = nullptr;
    }
    return 0;
}

static int midibuffer_reserve (lua_State* L) {
    if (auto* impl = *(Impl**) lua_touserdata (L, 1)) {
        auto size = lua_tointeger (L, 2);
        (*impl).buffer.ensureSize (static_cast<size_t> (size));
        lua_pushinteger (L, size);
    } else {
        lua_pushboolean (L, false);
    }
    return 1;
}

//==============================================================================
static int midibuffer_events_closure (lua_State* L) {
    auto* impl = (Impl*) lua_touserdata (L, lua_upvalueindex (1));
    
    if (impl->iter == impl->buffer.end()) {
        lua_pushnil (L);
        return 1;
    }
    
    const auto& ref = (*(*impl).iter);
    lua_pushlightuserdata (L, (void*) ref.data);
    lua_pushinteger (L, ref.numBytes);
    lua_pushinteger (L, ref.samplePosition + 1);
    ++impl->iter;

    return 3;
}

static int midibuffer_events (lua_State* L) {
    auto* impl = *(Impl**) lua_touserdata (L, 1);
    impl->reset_iter();
    lua_pushlightuserdata (L, impl);
    lua_pushcclosure (L, midibuffer_events_closure, 1);
    return 1;
}

//==============================================================================
static int midibuffer_messages_closure (lua_State* L) {
    auto* impl = (Impl*) lua_touserdata (L, lua_upvalueindex (1));
    if (impl->iter == impl->buffer.end()) {
        lua_pushnil (L);
        return 1;
    }

    const auto& ref = (*(*impl).iter);
    **impl->message = ref.getMessage();
    lua_rawgeti (L, LUA_REGISTRYINDEX, impl->msgref);
    lua_pushinteger (L, ref.samplePosition + 1);
    ++impl->iter;

    return 2;
}

static int midibuffer_messages (lua_State* L) {
    auto* impl = *(Impl**) lua_touserdata (L, 1);
    impl->reset_iter();
    lua_pushlightuserdata (L, impl);
    lua_pushcclosure (L, midibuffer_messages_closure, 1);
    return 1;
}

//==============================================================================
static int midibuffer_addmessage (lua_State* L) {
    auto* impl = *(Impl**) lua_touserdata (L, 1);
    impl->buffer.addEvent (
        **(MidiMessage**) lua_touserdata (L, 2), 
        static_cast<int> (lua_tointeger (L, 3) + 1));
    return 0;
}

static int midibuffer_addevent (lua_State* L) {
    auto* impl = *(Impl**) lua_touserdata (L, 1);
    impl->buffer.addEvent ((juce::uint8*) lua_touserdata (L, 2),
                            static_cast<int> (lua_tointeger (L, 3)),
                            static_cast<int> (lua_tointeger (L, 4) - 1));
    return 0;
}

static int midibuffer_swap (lua_State* L) {
    auto* impl = *(Impl**) lua_touserdata (L, 1);
    impl->buffer.swapWith (**(MidiBuffer**) lua_touserdata (L, 2));
    return 0;
}

static int midibuffer_clear (lua_State* L) {
    auto* impl = *(Impl**) lua_touserdata (L, 1);
    
    switch (lua_gettop (L)) {
        case 1: {
            (*impl).buffer.clear();
            break;
        }

        case 3: {
            (*impl).buffer.clear (static_cast<int> (lua_tointeger (L, 2) - 1),
                                  static_cast<int> (lua_tointeger (L, 3)));
            break;
        }
    }

    return 0;
}

#if 0
static int midibuffer_clear_range (lua_State* L) {
    auto* impl = *(Impl**) lua_touserdata (L, 1);
    (*impl).buffer.clear (static_cast<int> (lua_tointeger (L, 2)),
                          static_cast<int> (lua_tointeger (L, 3)));
    return 0;
}
#endif

static int midibuffer_size (lua_State* L) {
    auto* impl = *(Impl**) lua_touserdata (L, 1);
    lua_pushinteger (L, (*impl).buffer.getNumEvents());
    return 1;
}

static int midibuffer_addbuffer (lua_State* L) {
    auto* impl = *(Impl**) lua_touserdata (L, 1);
    if (lua_gettop (L) >= 5) {
        impl->buffer.addEvents (
            **(MidiBuffer**) lua_touserdata (L, 2),
            static_cast<int> (lua_tointeger (L, 3) - 1),
            static_cast<int> (lua_tointeger (L, 4)),
            static_cast<int> (lua_tointeger (L, 5)));
    } else {
        lua_error (L);
    }
    return 0;
}

static int midibuffer_insert (lua_State* L) {
    auto* impl = *(Impl**)lua_touserdata (L, 1);
    kv_packed_t pack;
    pack.packed = lua_tointeger (L, 2);

    impl->buffer.addEvent ((uint8_t*) pack.data, 4,
                           static_cast<int> (lua_tointeger (L, 3) - 1));
    return 0;
}

//==============================================================================

/// Methods.
// @section methods
static const luaL_Reg buffer_methods[] = {
    { "__gc",               midibuffer_free },
    // { "__tostring",         midibuffer_tostring },

    /// Removes all events from the buffer.
    // @function MidiBuffer:clear

    // Removes all events between two times from the buffer.
    // All events for which (start <= event position < start + numSamples) will
    // be removed.
    // @function MidiBuffer:clear
    // @int start
    // @int last
    { "clear",              midibuffer_clear },

    // { "clear_range",        midibuffer_clear_range },

    /// Number of events in the buffer.
    // @function MidiBuffer:size
    { "size",               midibuffer_size },

    /// Reserve an amount of space.
    // @param size Size in bytes to reserve
    // @function MidiBuffer:reserve
    // @return Size reserved in bytes or false
    { "reserve",            midibuffer_reserve },

    /// Exchanges the contents of this buffer with another one.
    // This is a quick operation, because no memory allocating or copying is done, it
    // just swaps the internal state of the two buffers.
    // @function MidiBuffer:swap
    // @tparam kv.MidiBuffer other Buffer to swap with
    { "swap",               midibuffer_swap },

    /// Insert some MIDI in the buffer.
    // @function MidiBuffer:insert
    // @int data Packed integer data
    // @int frame Sample index to insert at
    { "insert",             midibuffer_insert },

    /// Iterate over MIDI data.  
    // Iterate over midi data in this buffer
    // @function MidiBuffer:events
    // @return Event data iterator
    // @usage
    // -- @data    Raw bite array
    // -- @size    Size in bytes
    // -- @frame   Audio frame index in buffer
    // for data, size, frame in buffer:events() do
    //     -- do something with midi data
    // end
    { "events",             midibuffer_events },

    /// Add a raw MIDI Event
    // @function MidiBuffer:addevent
    // @param data Raw event data to add
    // @int size Size of data in bytes
    // @int frame Insert index
    { "addevent",           midibuffer_addevent },

    /// Iterate over MIDI Messages.  
    // Iterate over messages (kv.MidiMessage) in the buffer
    // @function MidiBuffer:messages
    // @return message iterator
    // @usage
    // -- @msg     A kv.MidiMessage
    // -- @frame   Audio frame index in buffer
    // for msg, frame in buffer:messages() do
    //     -- do something with midi data
    // end
    { "messages",           midibuffer_messages },

    /// Add a message to the buffer.
    // @function MidiBuffer:addmessage
    // @tparam kv.MidiMessage msg Message to add
    // @int frame Insert index
    { "addmessage",        midibuffer_addmessage },

    /// Add messages from another buffer.
    // @function MidiBuffer:addbuffer
    // @tparam kv.MidiBuffer buf Buffer to copy from
    { "addbuffer",         midibuffer_addbuffer },

    { NULL, NULL }
};

//==============================================================================
LUAMOD_API
int luaopen_kv_MidiBuffer (lua_State* L) {
    if (luaL_newmetatable (L, LKV_MT_MIDI_BUFFER)) {
        lua_pushvalue (L, -1);               /* duplicate the metatable */
        lua_setfield (L, -2, "__index");     /* mt.__index = mt */
        luaL_setfuncs (L, buffer_methods, 0);
        lua_pop (L, 1);
    }

    if (luaL_newmetatable (L, LKV_MT_MIDI_BUFFER_TYPE)) {
        lua_pushcfunction (L, midibuffer_new);   /* push audio_new function */
        lua_setfield (L, -2, "__call");     /* mt.__call = audio_new */
        lua_pop (L, 1);
    }

    lua_newtable (L);
    luaL_setmetatable (L, LKV_MT_MIDI_BUFFER_TYPE);
    lua_pushcfunction (L, midibuffer_new);
    lua_setfield (L, -2, "new");
    return 1;
}
